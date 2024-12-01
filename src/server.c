#include <arpa/inet.h>
#include <dirent.h>
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

struct threadpool *commTp	 = NULL; /* Comm. threads for auth/receiving work */
struct threadpool *workTp	 = NULL; /* internal threads for task-completion */
struct answer_map *uuidToAns = NULL;
struct user_map *uidToReqType = NULL;
pthread_mutex_t answerMapMut  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t uidMapMut	  = PTHREAD_MUTEX_INITIALIZER;

/**
 * @param arg a struct work_task holding details regarding a client's request
 */
void *worker_thread(void *arg) {
	worker_task *wt = arg;
	answer ans;
	struct user_info *ui = get_userinfo(uidToReqType, wt->uid);

	ans.status = 0;
	uuid_copy(ans.uuid, wt->load.uuid);

	enum REQ_TYPE rt = identify_req_type(wt->load.buf);
	if (rt == INVALID) {
		if ((send(wt->cfd, FAILURE_MSG, sizeof(FAILURE_MSG), 0)) == -1)
			perror("send() in worker_thread()");
		goto write_answer;
	}

	/* TODO: Set global session info */
	pthread_mutex_lock(&uidMapMut);
	while (ui && (ui->rt == UPLOAD || (ui->rt == DOWNLOAD && rt == UPLOAD)
				  /* TODO: more robust conflict handling */))
		pthread_cond_wait(&ui->condVar, &uidMapMut);

	/*
	 * If we're here, there is either no entry for this UID in the hashmap
	 * or the existing entry has no conflict with the new request type
	 */

	if (ui != NULL) {
		/* FIXME: append the new request type and the client it came from */
		ui->rt = rt;
		if (update_value_in_user_map(&uidToReqType, wt->uid, *ui) < 0)
			perror("worker_thread() - Failed to update the user map");
	} else {
		struct user_info new;
		new.rt = rt;
		pthread_cond_init(&new.condVar, NULL);
		if (!add_to_user_map(&uidToReqType, wt->uid, new))
			perror("worker_thread() - Failed to add to user map");
	}

	pthread_mutex_unlock(&uidMapMut);
	/* ----------------------------- */

	switch (rt) {
	case VIEW:
		ans.status = server_wrap_view(wt->cfd, wt->load.udir);
		break;
	case UPLOAD:
		ans.status = server_wrap_download(wt->cfd, wt->load.buf, wt->load.udir);
		break;
	case DOWNLOAD:
		ans.status = server_wrap_upload(wt->cfd, wt->load.buf, wt->load.udir);
		break;
	default:
		break;
	}

	/* TODO: Unset global session info */
	pthread_mutex_lock(&uidMapMut);
	/*
	 * if (--hashmap[uid].activeTaskCount == 0) {
	 * 		delete_entry_from_hashmap(uid);
	 * } else {
	 * 		remove_this_uuids_task(hashmap, entry, uuid);
	 *		pthread_cond_signal(&ui->condVar);
	 * }
	 */
	pthread_mutex_unlock(&uidMapMut);
	/* ------------------------------- */

write_answer:
	/* write answer to hashmap */
	pthread_mutex_lock(&answerMapMut);
	if (add_to_answer_map(&uuidToAns, wt->load.uuid, &ans))
		/* TODO: we're cooked so just kill this thread */
		perror("malloc() in add_to_answer_map()");
	pthread_cond_signal(wt->cond);
	pthread_mutex_unlock(&answerMapMut);

	free(arg);
	return NULL;
}

/**
 * @brief this thread will handle authentication and receiving a client's tasks
 */
void *client_thread(void *arg) {
	int cfd				= *(int *)arg;
	int status			= 0;
	int64_t uid			= -1;
	char buf[BUFSIZE]	= {0};
	worker_task *wt		= NULL;
	char udir[PATH_MAX] = "\0";
	free(arg);

	wt = malloc(sizeof(*wt));
	if (wt == NULL) {
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
			printf("client_thread(): client has closed socket\n");
			goto cleanup;
		}

		/* push task to worker queue, alongside random UUID. */
		wt->cfd = cfd;
		wt->uid = uid;
		pthread_cond_init(wt->cond, NULL);
		wt->load.buf  = buf;
		wt->load.udir = udir;
		uuid_generate_random(wt->load.uuid);
		add_task(workTp, wt);

		/* watch answer hashmap */
		/* TODO: timedwait + kill thread if no answer */
		answer *ans = NULL;
		pthread_mutex_lock(&answerMapMut);
		while ((ans = get_answers(uuidToAns, wt->load.uuid)) == NULL)
			pthread_cond_wait(wt->cond, &answerMapMut);
		status = ans->status;
		delete_from_answer_map(&uuidToAns, wt->load.uuid);
		pthread_mutex_unlock(&answerMapMut);

		/* cleanup for next iteration */
		pthread_cond_destroy(wt->cond);
		memset(wt, 0, sizeof(*wt));
		memset(buf, 0, sizeof(buf));

		if (status != 0)
			fprintf(stderr, "Error ocurred in threaded operation\n");
	}

cleanup:
	printf(YELLOW "Thread performing cleanup\n");
	free(wt);
	if (close(cfd) == -1)
		perror("close(cfd) in client_thread()");

	return NULL;
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
		delete_threadpool(workTp);
		/* FALLTHRU */
	case 4:
		delete_threadpool(commTp);
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

	free_answer_map(&uuidToAns);
	return status;
}

int init(int *sfd, struct sockaddr_in *saddr) {
	if (ensure_dir_exists(HOSTDIR) == false || init_db() != 0)
		return 1;

	*sfd = init_server_socket(saddr);
	if (*sfd < 0)
		return 2;

	commTp = create_threadpool(MAXCLIENTS, sizeof(int), client_thread);
	if (commTp == NULL)
		return 3;

	workTp = create_threadpool(MAXCLIENTS, sizeof(worker_task), worker_thread);
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
int server_wrap_upload(int cfd, const char *buf, const char *udir) {
	size_t fsize;
	char const *fname;
	char fpath[PATH_MAX];
	struct stat st;

	fname = buf + 10;

	if (snprintf(fpath, sizeof(fpath), "%s/%s", udir, fname) < 0)
		return -1;

	if (stat(fpath, &st) != 0) {
		if (send(cfd, DLOAD_FAIL_MSG, sizeof(DLOAD_FAIL_MSG), 0) == -1) {
			perror("send() #1 in server_wrap_upload()");
			return -2;
		}
		return 0;
	}

	fsize = st.st_size;

	if (send(cfd, SUCCESS_MSG, sizeof(SUCCESS_MSG), 0) == -1) {
		perror("send() #2 in server_wrap_upload()");
		return -3;
	}

	/* TODO: timeout if no response for a while */
	if ((recv_success(cfd, "Client never acknowledged receive")) < 0)
		return -4;

	if (send(cfd, &fsize, sizeof(fsize), 0) == -1) {
		perror("send() #3 in server_wrap_upload()");
		return -5;
	}

	if (upload_file(fpath, fsize, cfd) < 0)
		return -6;

	return 0;
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
int server_wrap_download(int cfd, const char *buf, const char *udir) {
	size_t fsize;
	char const *fname;
	char fpath[PATH_MAX], *err = NULL;
	__off_t usedSpace;

	if (send(cfd, SUCCESS_MSG, sizeof(SUCCESS_MSG), 0) == -1) {
		perror("send() #1 in server_wrap_download()");
		return -2;
	}

	fname = buf + 8;

	if (recv(cfd, &fsize, sizeof(fsize), 0) == -1) {
		perror("recv() in server_wrap_download()");
		return -3;
	}

	usedSpace = get_used_space(udir);

	if (usedSpace < 0)
		err = FAILURE_MSG;
	else if (usedSpace + fsize > MAX_CLIENT_SPACE)
		err = ULOAD_FAILURE_MSG;

	if (err != NULL) {
		if (send(cfd, err, strlen(err), 0) == -1)
			perror("send() #2 in server_wrap_download()");
		return -4;
	}

	if (send(cfd, SUCCESS_MSG, sizeof(SUCCESS_MSG), 0) == -1) {
		perror("send() #3 in server_wrap_download()");
		return -5;
	}

	if (snprintf(fpath, sizeof(fpath), "%s/%s", udir, fname) < 0)
		return -6;

	if (download_file(fpath, fsize, cfd) != 0)
		return -7;

	if (send(cfd, SUCCESS_MSG, sizeof(SUCCESS_MSG), 0) == -1) {
		perror("send() #4 in server_wrap_download()");
		return -8;
	}

	return 0;
}

int server_wrap_view(int cfd, const char *udir) {
	int status = 0;
	ssize_t idx;
	char *buf;

	if ((buf = malloc(BUFSIZE)) == NULL) {
		perror("malloc() in server_wrap_view()");
		return -1;
	}

	/* get list of entries + size and send latter */
	if ((idx = view(buf, BUFSIZE, udir)) < 0) {
		status = -2;
		goto cleanup;
	}

	if ((send(cfd, &idx, sizeof(idx), 0)) == -1) {
		perror("send() #1 in server_wrap_view()");
		status = -3;
		goto cleanup;
	}

	/* no files */
	if (idx == 0)
		goto cleanup;

	/* transfer information */
	for (int i = 0; idx > 0; i++, idx -= BUFSIZE) {
		if ((send(cfd, buf + (i << 10), min(BUFSIZE, idx), 0)) == -1) {
			perror("send() #2 in server_wrap_view()");
			status = -4;
			goto cleanup;
		}
	}

cleanup:
	free(buf);
	return status;
}

bool ensure_dir_exists(char *d) {
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
