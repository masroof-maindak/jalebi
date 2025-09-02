#include <arpa/inet.h>
#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "../include/auth.h"
#include "../include/hashmap.h"
#include "../include/server.h"
#include "../include/threadpool.h"

struct tpool *commTp = NULL; /* Communication threads to auth/recv. tasks */
struct tpool *workTp = NULL; /* internal threads to complete tasks */

struct task_status_map *uuidToStatus = NULL;
struct user_tasks_map *uidToTasks	 = NULL;
pthread_mutex_t statusMapMut		 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t tasksMapMut			 = PTHREAD_MUTEX_INITIALIZER;

/**
 * @param arg a struct work_task holding details regarding a client's request
 */
void worker_thread(void *arg) {
	worker_task *wt	  = arg;
	task_list *uTasks = get_user_tasks(uidToTasks, wt->uid);
	enum STATUS st	  = FAILURE;

	wt->load.rt = identify_req_type(wt->load.buf);
	if (wt->load.rt == INVALID) {
		if ((send(wt->cfd, FAILURE_MSG, sizeof(FAILURE_MSG), 0)) == -1)
			perror("send() in worker_thread()");
		goto write_answer;
	}

	/* Set global session info */
	pthread_mutex_lock(&tasksMapMut);

	while (uTasks != NULL && !is_conflicting(&wt->load, uTasks))
		pthread_cond_wait(uTasks->userCond, &tasksMapMut);

	if (uTasks == NULL) {
		if (!add_new_user(&uidToTasks, wt->uid)) {
			/* TODO: we're cooked so try to gracefully terminate/restart(?) */
			perror("malloc() in add_new_user()");
		}

		uTasks = get_user_tasks(uidToTasks, wt->uid);
	}

	pthread_mutex_unlock(&tasksMapMut);

	/* add task to this user's list */
	if (!append_task(&wt->load, uTasks)) {
		if ((send(wt->cfd, OVERLOAD_MSG, sizeof(OVERLOAD_MSG), 0)) == -1)
			perror("send() in worker_thread()");
		goto write_answer;
	}

	/* perform task */
	switch (wt->load.rt) {
	case VIEW:
		st = server_wrap_view(wt->cfd, wt->load.udir);
		break;
	case UPLOAD:
		st = server_wrap_download(wt->cfd, wt->load.buf, wt->load.udir);
		break;
	case DOWNLOAD:
		st = server_wrap_upload(wt->cfd, wt->load.buf, wt->load.udir);
		break;
	default:
		break;
	}

	/* Unset global session info */
	pthread_mutex_lock(&tasksMapMut);
	if (--uTasks->count == 0) {
		delete_from_user_map(&uidToTasks, wt->uid);
	} else {
		remove_task_from_list(wt->load.uuid, uTasks);
		pthread_cond_signal(uTasks->userCond);
	}
	pthread_mutex_unlock(&tasksMapMut);

write_answer:
	pthread_mutex_lock(&statusMapMut);
	if (!add_new_status(&uuidToStatus, wt->load.uuid, st))
		/* TODO: we're cooked so try to gracefully terminate/restart(?) */
		perror("malloc() in add_to_answer_map()");
	pthread_mutex_unlock(&statusMapMut);

	pthread_cond_signal(wt->statCond);

	free(arg);
}

/**
 * @brief this thread will handle authentication and receiving a client's tasks
 */
void client_thread(void *arg) {
	int cfd				= *(int *)arg;
	int status			= 0;
	int64_t uid			= -1;
	char buf[BUFSIZE]	= {0};
	char udir[PATH_MAX] = "\0";
	worker_task *wt		= NULL;
	free(arg);

	wt = malloc(sizeof(*wt));
	if (wt == NULL) {
		perror("malloc() in client_thread() - wt");
		goto cleanup;
	}

	wt->statCond = malloc(sizeof(*wt->statCond));
	if (wt->statCond == NULL) {
		perror("malloc() in client_thread() - wt");
		goto cleanup;
	}

	/* get uid, mkdir if needed, and send success/failure */
	if ((uid = authenticate_and_get_uid(cfd, buf)) < 0 ||
		(snprintf(udir, sizeof(udir), "%s/%ld", HOSTDIR, uid)) < 0 ||
		!ensure_dir_exists(udir)) {
		if (send(cfd, FAILURE_MSG, sizeof(FAILURE_MSG), 0) == -1)
			perror("send() #1 in client_thread()");
		goto cleanup;
	} else {
		if (send(cfd, SUCCESS_MSG, sizeof(SUCCESS_MSG), 0) == -1) {
			perror("send() #2 in client_thread()");
			goto cleanup;
		}
	}

	while (status == 0) {
		ssize_t n = recv(cfd, buf, sizeof(buf), 0);
		if (n == -1) {
			perror("recv() in client_thread()");
			goto cleanup;
		} else if (n == 0) {
			printf("Client has closed socket\n");
			goto cleanup;
		}

		/* push task to worker queue, alongside random UUID. */
		wt->cfd = cfd;
		wt->uid = uid;
		pthread_cond_init(wt->statCond, NULL);
		wt->load.buf  = buf;
		wt->load.udir = udir;
		uuid_generate_random(wt->load.uuid);
		add_task(workTp, wt);

		/* watch answer hashmap */
		/* TODO: timedwait + kill thread if no answer */
		enum STATUS *st = NULL;
		pthread_mutex_lock(&statusMapMut);
		while ((st = get_status(uuidToStatus, wt->load.uuid)) == NULL)
			pthread_cond_wait(wt->statCond, &statusMapMut);
		status = *st;
		delete_from_status_map(&uuidToStatus, wt->load.uuid);
		pthread_mutex_unlock(&statusMapMut);

		/* cleanup for next iteration */
		pthread_cond_destroy(wt->statCond);
		memset(buf, 0, sizeof(buf));

		if (status != 0)
			fprintf(stderr, "Error ocurred in threaded operation\n");
	}

cleanup:
	printf(YELLOW "Thread performing cleanup\n");
	free(wt->statCond);
	free(wt);
	if (close(cfd) == -1)
		perror("close(cfd) in client_thread()");
}

int main() {
	int sfd, cfd, status;
	struct sockaddr_in saddr, caddr;
	socklen_t len = sizeof(caddr);

	if ((status = init(&sfd, &saddr)) != 0)
		goto cleanup;

	printf("Listening on port %d...\n", SERVER_PORT);

	for (;;) {
		if (-1 == (cfd = accept(sfd, (struct sockaddr *)&caddr, &len))) {
			perror("accept() in main()");
			goto cleanup;
		}

		add_task(commTp, &cfd);
	}

cleanup:
	switch (status) {
	case 0:
		delete_tpool(workTp);
		/* FALLTHRU */
	case 4:
		delete_tpool(commTp);
		/* FALLTHRU */
	case 3:
		if ((close(sfd)) == -1)
			perror("close(sfd)");
		/* FALLTHRU */
	case 2:
		/* CHECK: is this right... ? */
		while (!close_db())
			usleep(100000);
		/* FALLTHRU */
	case 1:
		break;
	}

	free_status_map(&uuidToStatus);
	free_user_map(&uidToTasks);
	pthread_mutex_destroy(&statusMapMut);
	pthread_mutex_destroy(&tasksMapMut);
	return status;
}

int init(int *sfd, struct sockaddr_in *saddr) {
	if (ensure_dir_exists(HOSTDIR) == false || init_db() != 0)
		return 1;

	*sfd = init_server_socket(saddr);
	if (*sfd < 0)
		return 2;

	commTp = create_tpool(MAXCLIENTS, sizeof(int), client_thread);
	if (commTp == NULL)
		return 3;

	workTp = create_tpool(MAXCLIENTS, sizeof(worker_task), worker_thread);
	if (workTp == NULL)
		return 4;

	return 0;
}

int init_server_socket(struct sockaddr_in *saddr) {
	int sfd, reuse = 1;

	if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket() in init_server_socket()");
		return -1;
	}

	if ((setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) ==
		-1) {
		perror("setsockopt(SO_REUSEADDR) in init_server_socket()");
		return -2;
	}

	saddr->sin_family	   = AF_INET;
	saddr->sin_port		   = htons(SERVER_PORT);
	saddr->sin_addr.s_addr = INADDR_ANY;
	memset(saddr->sin_zero, '\0', sizeof(saddr->sin_zero));

	if ((bind(sfd, (struct sockaddr *)saddr, sizeof(*saddr))) == -1) {
		perror("bind() in init_server_socket()");
		return -3;
	}

	if (listen(sfd, MAXCLIENTS) == -1) {
		perror("listen() in init_server_socket()");
		return -4;
	}

	return sfd;
}

/**
 * @details This function is called when the user opts to DOWNLOAD a file.
 * 	- Check if the file exists inside the user's directory
 * 	- If not, send() back $FAILURE$FILE_NOT_FOUND$, else send $SUCCESS$
 * 	- recv() acknowledgement
 * 	- Else, send() back it's size
 * 	- Call upload function to send() file until nothing remains
 *
 * @param buf the buffer containing the request $DOWNLOAD$<fname>$
 */
enum STATUS server_wrap_upload(int cfd, const char *buf, const char *udir) {
	size_t fsize;
	char const *fname;
	char fpath[PATH_MAX];
	struct stat st;

	fname = buf + 10;

	if (snprintf(fpath, sizeof(fpath), "%s/%s", udir, fname) < 0)
		return FAILURE;

	if (stat(fpath, &st) != 0) {
		if (send(cfd, DLOAD_FAIL_MSG, sizeof(DLOAD_FAIL_MSG), 0) == -1) {
			perror("send() #1 in server_wrap_upload()");
			return SEND_FAIL;
		}
		return SUCCESS;
	}

	fsize = st.st_size;

	if (send(cfd, SUCCESS_MSG, sizeof(SUCCESS_MSG), 0) == -1) {
		perror("send() #2 in server_wrap_upload()");
		return SEND_FAIL;
	}

	/* TODO: timeout if no response for a while */
	if ((recv_success(cfd, "Client never acknowledged receive")) < 0)
		return SUCCESS;

	if (send(cfd, &fsize, sizeof(fsize), 0) == -1) {
		perror("send() #3 in server_wrap_upload()");
		return SEND_FAIL;
	}

	if (upload_file(fpath, fsize, cfd) < 0)
		return FAILURE;

	return SUCCESS;
}

/**
 * @details This function is called when the user opts to UPLOAD a file.
 * 	- Sends back SUCCESS after determining the request type
 * 	- Receives the file's total size from the client
 * 	- Checks if there is enough space in the user's dir to accomodate this file
 * 	- If not, sends back $FAILURE$LOW_SPACE$
 * 	- If yes, sends back $SUCCESS$
 * 	- Calls the main download function (download) using the `bytes`
 * 	  acquired from #2
 * 	- Sends $SUCCESS$ again
 *
 * @param buf the buffer containing the request $UPLOAD$<fname>$
 */
enum STATUS server_wrap_download(int cfd, const char *buf, const char *udir) {
	size_t fsize;
	char const *fname;
	char fpath[PATH_MAX], *err = NULL;
	__off_t usedSpace;

	if (send(cfd, SUCCESS_MSG, sizeof(SUCCESS_MSG), 0) == -1) {
		perror("send() #1 in server_wrap_download()");
		return SEND_FAIL;
	}

	fname = buf + 8;

	if (recv(cfd, &fsize, sizeof(fsize), 0) == -1) {
		perror("recv() in server_wrap_download()");
		return RECV_FAIL;
	}

	usedSpace = get_used_space(udir);

	if (usedSpace < 0)
		err = FAILURE_MSG;
	else if (usedSpace + fsize > MAX_CLIENT_SPACE)
		err = ULOAD_FAILURE_MSG;

	if (err != NULL) {
		if (send(cfd, err, strlen(err), 0) == -1)
			perror("send() #2 in server_wrap_download()");
		return SEND_FAIL;
	}

	if (send(cfd, SUCCESS_MSG, sizeof(SUCCESS_MSG), 0) == -1) {
		perror("send() #3 in server_wrap_download()");
		return SEND_FAIL;
	}

	if (snprintf(fpath, sizeof(fpath), "%s/%s", udir, fname) < 0)
		return FAILURE;

	if (download_file(fpath, fsize, cfd) != 0)
		return FAILURE;

	if (send(cfd, SUCCESS_MSG, sizeof(SUCCESS_MSG), 0) == -1) {
		perror("send() #4 in server_wrap_download()");
		return SEND_FAIL;
	}

	return 0;
}

enum STATUS server_wrap_view(int cfd, const char *udir) {
	enum STATUS st = SUCCESS;
	ssize_t idx;
	char *buf;

	if ((buf = malloc(BUFSIZE)) == NULL) {
		perror("malloc() in server_wrap_view()");
		return MALLOC_FAIL;
	}

	/* get list of entries + size and send latter */
	if ((idx = view(buf, BUFSIZE, udir)) < 0) {
		st = FAILURE;
		goto cleanup;
	}

	if ((send(cfd, &idx, sizeof(idx), 0)) == -1) {
		perror("send() #1 in server_wrap_view()");
		st = SEND_FAIL;
		goto cleanup;
	}

	/* no files */
	if (idx == 0)
		goto cleanup;

	/* transfer information */
	for (int i = 0; idx > 0; i++, idx -= BUFSIZE) {
		if ((send(cfd, buf + (i << 10), min(BUFSIZE, idx), 0)) == -1) {
			perror("send() #2 in server_wrap_view()");
			st = SEND_FAIL;
			goto cleanup;
		}
	}

cleanup:
	free(buf);
	return st;
}

bool_t ensure_dir_exists(char *d) {
	struct stat st = {0};
	if (stat(d, &st) == -1) {
		if (mkdir(d, 0700) == -1) {
			perror("mkdir() in ensure_dir_exists()");
			return false;
		}
	}
	return true;
}

__off_t get_used_space(const char *dir) {
	struct dirent *entry;
	struct stat st;
	char fpath[PATH_MAX];
	__off_t size = 0;
	DIR *d;

	if ((d = opendir(dir)) == NULL) {
		perror("opendir() in get_used_space()");
		return -1;
	}

	while ((entry = readdir(d)) != NULL) {
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			continue;

		if (snprintf(fpath, sizeof(fpath), "%s/%s", dir, entry->d_name) < 0) {
			closedir(d);
			return -2;
		}

		if (stat(fpath, &st) == -1) {
			perror("stat() in get_used_space()");
			closedir(d);
			return -3;
		}

		size += st.st_size;
	}

	closedir(d);
	return size;
}

/**
 * @brief registers/verifies a user's credentials
 * @return UID on success, negative value on failure
 */
int64_t authenticate_and_get_uid(int cfd, char *buf) {
	char mode, un[PW_MAX_LEN + 1], pw[PW_MAX_LEN + 1];
	uint8_t unL, pwL;
	int64_t uid;

	if (recv(cfd, buf, BUFSIZE, 0) == -1) {
		perror("recv() in authenticate_and_get_uid()");
		return -1;
	}

	mode = buf[0], unL = buf[1], pwL = buf[2];
	if ((mode != 'L' && mode != 'R') ||
		(unL < PW_MIN_LEN || unL > PW_MAX_LEN) ||
		(pwL < PW_MIN_LEN || pwL > PW_MAX_LEN))
		return -2;

	memcpy(un, buf + 3, unL);
	memcpy(pw, buf + 3 + unL, pwL);
	un[unL] = pw[pwL] = '\0';
	uid = (mode == 'L') ? verify_user(un, pw) : register_user(un, pw);
	return uid;
}
