#include <arpa/inet.h>
#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../include/server.h"
#include "../include/utils.h"

int handle_authorisation(int cfd, const char *buf) {
	/* recv in buffer */

	/* if login, query DB */

	/* if register, query DB */

	/* send success */

	return 0;
}

int main() {
	if (ensure_srv_dir_exists() != 0) {
		fprintf(
			stderr, COL_RED
			"Unable to find or generate server directory, exiting\n" COL_RESET);
		return 1;
	}

	int sfd, cfd, ret = 0;
	struct sockaddr_in saddr, caddr;
	socklen_t addrSize = sizeof(caddr);
	pthread_t clientThread;

	printf("Listening on port %d...\n", SERVER_PORT);

	if ((sfd = init_server_socket(&saddr)) < 0)
		return 2;

	for (;;) {
		if ((cfd = accept(sfd, (struct sockaddr *)&caddr, &addrSize)) == -1) {
			perror("accept()");
			ret = 3;
			goto cleanup;
		}

		if (pthread_create(&clientThread, NULL, handle_client, &cfd) != 0) {
			perror("pthread_create()");
			ret = 4;
			goto cleanup;
		}

		if (pthread_detach(clientThread) != 0) {
			perror("pthread_detach()");
			ret = 5;
			goto cleanup;
		}
	}

cleanup:
	if ((close(sfd)) == -1) {
		perror("close(sfd)");
		ret = 6;
	}

	return ret;
}

void *handle_client(void *arg) {
	int cfd, status = 0;
	enum REQUEST reqType;
	ssize_t bytesRead;
	char *buf;
	unsigned long uid;

	if ((buf = malloc(BUFSIZE)) == NULL) {
		perror("malloc()");
		pthread_exit(NULL);
	}

	cfd = *(int *)arg;
	if ((uid = handle_authorisation(cfd, buf)) != 0)
		goto cleanup;

	while (status == 0) {
		if ((bytesRead = recv(cfd, buf, BUFSIZE, 0)) == -1) {
			perror("recv()");
			goto cleanup;
		}

		if (bytesRead == 0) {
			printf("Client has closed the socket!\n");
			goto cleanup;
		}

		reqType = identify_request(buf);

		switch (reqType) {
		case VIEW:
			status = serv_wrap_view(cfd, uid);
			break;
		case DOWNLOAD:
			status = serv_wrap_upload(cfd, buf, uid);
			break;
		case UPLOAD:
			status = serv_wrap_download(cfd, buf, uid);
			break;
		default:
			if ((send(cfd, FAILURE_MSG, sizeof(FAILURE_MSG), 0)) == -1) {
				perror("send()");
				goto cleanup;
			}
		}

		memset(buf, 0, BUFSIZE);

		if (status != 0)
			fprintf(stderr, "Internal error occured in threaded operation!\n");
	}

cleanup:
	free(buf);
	if ((close(cfd)) == -1)
		perror("close(cfd)");
	pthread_exit(NULL);
}

int init_server_socket(struct sockaddr_in *saddr) {
	int sfd, reuse = 1;

	if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket()");
		return -1;
	}

	if ((setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) ==
		-1) {
		perror("setsockopt(SO_REUSEADDR)");
		return -2;
	}

	saddr->sin_family	   = AF_INET;
	saddr->sin_port		   = htons(SERVER_PORT);
	saddr->sin_addr.s_addr = INADDR_ANY;
	memset(saddr->sin_zero, '\0', sizeof(saddr->sin_zero));

	if ((bind(sfd, (struct sockaddr *)saddr, sizeof(*saddr))) == -1) {
		perror("bind()");
		return -3;
	}

	if (listen(sfd, MAXCLIENTS) == -1) {
		perror("listen()");
		return -4;
	}

	return sfd;
}

/**
 * @details This function is called when the user opts to DOWNLOAD a file.
 * 	- Check if the file exists inside HOSTDIR
 * 	- If not, send() back $FAILURE$FILE_NOT_FOUND$, else send $SUCCESS$
 * 	- recv() acknowledgement
 * 	- Else, send() back it's size
 * 	- Call upload function to send() file until nothing remains
 *
 * @param[buf] the buffer containing the request $DOWNLOAD$<fname>$
 */
int serv_wrap_upload(const int cfd, const char *buf) {
	size_t fsize;
	char const *fname;
	char fpath[BUFSIZE >> 1];
	int n;
	struct stat st;

	fname = buf + 10;
	n	  = snprintf(fpath, sizeof(fpath), HOSTDIR "/%s", fname);

	if (n < 0)
		return -1;

	if (stat(fpath, &st) != 0) {
		if (send(cfd, DLOAD_FAILURE_MSG, sizeof(DLOAD_FAILURE_MSG), 0) == -1) {
			perror("send()");
			return -2;
		}
		return 0;
	} else {
		if (send(cfd, SUCCESS_MSG, sizeof(SUCCESS_MSG), 0) == -1) {
			perror("send()");
			return -3;
		}
	}

	/* TODO: timeout if no response for a while */
	if ((recv_success(cfd, "Client never acknowledged receive")) < 0)
		return -4;

	fsize = st.st_size;
	if (send(cfd, &fsize, sizeof(fsize), 0) == -1) {
		perror("send()");
		return -5;
	}

	if (upload(fpath, fsize, cfd) < 0) {
		fprintf(stderr, "upload()\n");
		return -6;
	}

	return 0;
}

/**
 * @details This function is called when the user opts to UPLOAD a file.
 * 	- Sends back SUCCESS after determining the request type
 * 	- Receives the file's total size from the client
 * 	- Checks if there is enough space in the HOSTDIR to accomodate this file
 * 	- If not, sends back $FAILURE$LOW_SPACE$
 * 	- If yes, sends back $SUCCESS$
 * 	- Calls the main download function (download) using the `bytes`
 * 	  acquired from #2
 * 	- Sends $SUCCESS$ again
 *
 * @param[buf] the buffer containing the request $UPLOAD$<fname>$
 */
int serv_wrap_download(const int cfd, const char *buf) {
	size_t fsize;
	int n;
	char const *fname;
	char fpath[BUFSIZE << 1];
	__off_t usedSpace;

	if (send(cfd, SUCCESS_MSG, sizeof(SUCCESS_MSG), 0) == -1) {
		perror("send()");
		return -2;
	}

	fname = buf + 8;

	if (recv(cfd, &fsize, sizeof(fsize), 0) == -1) {
		perror("recv()");
		return -3;
	}

	if ((usedSpace = get_used_space(HOSTDIR)) + fsize > MAX_CLIENT_SPACE) {
		if (send(cfd, ULOAD_FAILURE_MSG, sizeof(ULOAD_FAILURE_MSG), 0) == -1) {
			perror("send()");
			return -5;
		}
		return 4;
	} else if (usedSpace < 0) {
		fprintf(stderr, "Couldn't query available space!\n");
		if (send(cfd, FAILURE_MSG, sizeof(FAILURE_MSG), 0) == -1) {
			perror("send()");
			return -5;
		}
		return -4;
	}

	if (send(cfd, SUCCESS_MSG, sizeof(SUCCESS_MSG), 0) == -1) {
		perror("send()");
		return -5;
	}

	n = snprintf(fpath, sizeof(fpath), HOSTDIR "/%s", fname);
	if (n < 0)
		return -6;

	if (download(fpath, fsize, cfd) != 0) {
		fprintf(stderr, "download()\n");
		return -7;
	}

	if (send(cfd, SUCCESS_MSG, sizeof(SUCCESS_MSG), 0) == -1) {
		perror("send()");
		return -8;
	}

	return 0;
}

int serv_wrap_view(int cfd, int uid) {
	int status = 0;
	ssize_t idx;
	char *ret;

	if ((ret = malloc(BUFSIZE)) == NULL) {
		perror("malloc()");
		return -1;
	}

	/* error while viewing */
	if ((idx = view(ret, BUFSIZE)) < 0) {
		fprintf(stderr, "Internal error occured while `view`ing!\n");
		status = -2;
		goto cleanup;
	}

	/* no files */
	if (idx == 0) {
		if ((send(cfd, VIEW_FAILURE_MSG, sizeof(VIEW_FAILURE_MSG), 0)) == -1) {
			perror("send()");
			status = -3;
		}
		goto cleanup;
	}

	if ((send(cfd, &idx, sizeof(idx), 0)) == -1) {
		perror("send()");
		status = -3;
	}

	/* transfer information */
	for (int i = 0; idx > 0; i++, idx -= BUFSIZE) {
		if ((send(cfd, ret + (i << 10), min(BUFSIZE, idx), 0)) == -1) {
			perror("send()");
			status = -4;
			goto cleanup;
		}
	}

cleanup:
	free(ret);
	return status;
}

int ensure_srv_dir_exists() {
	struct stat st = {0};
	if (stat(HOSTDIR, &st) == -1) {
		if (mkdir(HOSTDIR, 0700) == -1) {
			perror("mkdir()");
			return 1;
		}
	}
	return 0;
}

__off_t get_used_space(const char *dir) {
	struct dirent *entry;
	struct stat st;
	char fpath[BUFSIZE];
	int size = 0;
	DIR *d;

	if ((d = opendir(dir)) == NULL) {
		perror("opendir()");
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
			perror("stat()");
			closedir(d);
			return -3;
		}

		size += st.st_size;
	}

	closedir(d);
	return size;
}
