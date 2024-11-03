#include <arpa/inet.h>
#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../include/auth.h"
#include "../include/queue.h"
#include "../include/server.h"
#include "../include/threadpool.h"

struct tpool *tp	   = NULL;
struct queue *q		   = NULL;
pthread_mutex_t EnqMut = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg __attribute__((unused))) {

	for (;;) {
		/* TODO: synchronisation; only pick up task if available */
		// ---
		int cfd = *(int *)top(q);
		dequeue(q);
		// ---

		char *buf = NULL, udir[PATH_MAX] = "\0";
		ssize_t n;
		int status = 0;
		int64_t uid;

		if ((buf = malloc(BUFSIZE)) == NULL) {
			perror("malloc() in handle_client()");
			continue;
		}

		/* get uid, mkdir if needed, and send success/failure */
		if ((uid = authenticate_and_get_uid(cfd, buf)) < 0 ||
			(snprintf(udir, sizeof(udir), "%s/%ld", HOSTDIR, uid)) < 0 ||
			!ensure_dir_exists(udir)) {
			if (send(cfd, FAILURE_MSG, sizeof(FAILURE_MSG), 0) == -1)
				perror("send() #1 in handle_client()");
			goto cleanup;
		} else {
			if (send(cfd, SUCCESS_MSG, sizeof(SUCCESS_MSG), 0) == -1) {
				perror("send() #2 in handle_client()");
				goto cleanup;
			}
		}

		while (status == 0) {
			if ((n = recv(cfd, buf, BUFSIZE, 0)) == -1) {
				perror("recv() in handle_client()");
				goto cleanup;
			}

			if (n == 0) {
				printf("Client has closed the socket!\n");
				close(cfd);
				goto cleanup;
			}

			enum REQ_TYPE rt = identify_request(buf);

			/* TODO: Set global session info */
			if (rt != INVALID) {
				;
			}

			switch (rt) {
			case VIEW:
				status = server_wrap_view(cfd, udir);
				break;
			case UPLOAD:
				/*  TODO: If this user has another UPLOAD, block thread */
				status = server_wrap_download(cfd, buf, udir);
				break;
			case DOWNLOAD:
				status = server_wrap_upload(cfd, buf, udir);
				break;
			case INVALID:
				if ((send(cfd, FAILURE_MSG, sizeof(FAILURE_MSG), 0)) == -1) {
					perror("send() #3 in handle_client()");
					goto cleanup;
				}
				break;
			}

			/* TODO: Unset global session info */
			if (rt != INVALID) {
				;
			}

			if (status != 0)
				fprintf(stderr, "Error ocurred in threaded operation\n");
		}

	cleanup:
		free(buf);
		if (close(cfd) == -1)
			perror("close(cfd) in handle_client");
	}

	return NULL;
}

int main() {
	int sfd, cfd, ret;
	struct sockaddr_in saddr, caddr;
	socklen_t len = sizeof(caddr);

	if ((ret = init(&sfd, &saddr)) != 0)
		goto cleanup;

	printf("Listening on port %d...\n", SERVER_PORT);

	for (;;) {
		if ((cfd = accept(sfd, (struct sockaddr *)&caddr, &len)) == -1) {
			perror("accept() in main()");
			goto cleanup;
		}

		pthread_mutex_lock(&EnqMut);
		enqueue(q, (void *)(&cfd));
		pthread_mutex_unlock(&EnqMut);
	}

cleanup:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
	switch (ret) {
	case 0:
		delete_queue(q);
	case 4:
		delete_threadpool(tp);
	case 3:
		if ((close(sfd)) == -1) {
			perror("close(sfd)");
		}
	case 2:
		/* is this right... ? */
		while (close_db() != 0)
			usleep(100000);
	case 1:
		break;
	}
#pragma GCC diagnostic pop

	pthread_mutex_destroy(&EnqMut);
	return ret;
}

int init(int *sfd, struct sockaddr_in *saddr) {
	if (ensure_dir_exists(HOSTDIR) == false || init_db() != 0)
		return 1;

	if ((*sfd = init_server_socket(saddr)) < 0)
		return 2;

	tp = create_threadpool(MAXCLIENTS, handle_client);
	if (tp == NULL)
		return 3;

	q = create_queue(sizeof(int));
	if (q == NULL)
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
int server_wrap_upload(const int cfd, const char *buf, char *udir) {
	size_t fsize;
	char const *fname;
	char fpath[PATH_MAX];
	int n;
	struct stat st;

	fname = buf + 10;
	n	  = snprintf(fpath, sizeof(fpath), "%s/%s", udir, fname);

	if (n < 0)
		return -1;

	if (stat(fpath, &st) != 0) {
		if (send(cfd, DLOAD_FAILURE_MSG, sizeof(DLOAD_FAILURE_MSG), 0) == -1) {
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
int server_wrap_download(const int cfd, const char *buf, char *udir) {
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

	if (err) {
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

int server_wrap_view(int cfd, char *udir) {
	int status = 0;
	ssize_t idx;
	char *ret;

	if ((ret = malloc(BUFSIZE)) == NULL) {
		perror("malloc() in server_wrap_view()");
		return -1;
	}

	/* get list of entries + size and send latter */
	if ((idx = view(ret, BUFSIZE, udir)) < 0) {
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
		if ((send(cfd, ret + (i << 10), min(BUFSIZE, idx), 0)) == -1) {
			perror("send() #2 in server_wrap_view()");
			status = -4;
			goto cleanup;
		}
	}

cleanup:
	free(ret);
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
	int64_t uid = -1;

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
