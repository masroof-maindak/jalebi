#include <arpa/inet.h>
#include <dirent.h>
#include <netinet/in.h>
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

void *handle_client(void *arg) {
	int cfd, reqType, status;
	ssize_t bytesRead;
	char *buf;

	if ((buf = malloc(BUFSIZE)) == NULL) {
		perror("malloc()");
		pthread_exit(NULL);
	}

	cfd	   = *(int *)arg;
	status = 0;

	while (status == 0) {
		if ((bytesRead = recv(cfd, buf, BUFSIZE, 0)) == -1) {
			perror("recv()");
			goto cleanup;
		}

		printf("CLIENT SENT: %s\n", buf);

		if (bytesRead == 0) {
			printf("Client has closed the socket");
			goto cleanup;
		}

		reqType = identify_request(buf);

		switch (reqType) {
		case 1: /* $VIEW$ */
			status = serv_wrap_view(cfd);
			break;
		case 2: /* $DOWNLOAD$<filename>$ */
			status = serv_wrap_upload(cfd, buf);
			break;
		case 3: /* $UPLOAD$<filename>$ */
			status = serv_wrap_download(cfd, buf);
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

int main() {
	ensure_srv_dir_exists();

	int sfd, cfd, ret = 0;
	struct sockaddr_in saddr, caddr;
	socklen_t addrSize = sizeof(caddr);
	pthread_t clientThread;

	printf("Listening on port %d...\n", SERVER_PORT);

	if ((sfd = init_server_socket(&saddr)) < 0)
		return 1;

	for (;;) {

		if ((cfd = accept(sfd, (struct sockaddr *)&caddr, &addrSize)) == -1) {
			perror("accept()");
			ret = 2;
			goto cleanup;
		}

		if (pthread_create(&clientThread, NULL, handle_client, &cfd) != 0) {
			perror("pthread_create()");
			ret = 3;
			goto cleanup;
		}

		if (pthread_detach(clientThread) != 0) {
			perror("pthread_detach()");
			ret = 4;
			goto cleanup;
		}
	}

cleanup:
	if ((close(sfd)) == -1) {
		perror("close(sfd)");
		ret = 5;
	}

	return ret;
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
 * @brief upload `bytes` bytes, from `filename` file, to the client's socket
 *
 * @note the file's existence must be guaranteed before calling this function
 */
int serv_upload(char *filename, size_t bytes, int cfd) {
	int bytesRead, toWrite, ret = 0;
	FILE *fp;
	char *buf;

	if ((fp = fopen(filename, "r")) == NULL) {
		perror("fopen()");
		return 1;
	}

	if ((buf = malloc(BUFSIZE)) == NULL) {
		perror("malloc()");
		fclose(fp);
		return 2;
	}

	while (bytes > 0) {

		toWrite = min(BUFSIZE, bytes);

		bytesRead = fread(buf, 1, toWrite, fp);
		if (ferror(fp)) {
			perror("fread()");
			ret = 3;
			goto cleanup;
		}

		if (bytesRead != toWrite) {
			fprintf(stderr, "Error: file read mismatch!");
			ret = 4;
			goto cleanup;
		}

		if (send(cfd, buf, bytesRead, 0) == -1) {
			perror("send()");
			ret = 5;
			goto cleanup;
		}

		bytes -= bytesRead;
	}

cleanup:
	free(buf);
	fclose(fp);
	return ret;
}

/**
 * @details This function is called when the user opts to DOWNLOAD a file.
 * 	- we must check if the file exists inside HOSTDIR
 * 	- If not, send back $FAILURE$FILE_NOT_FOUND$
 * 	- Else, query that file for it's size, and send() it back
 * 	- Call serv_upload function to send() file until nothing remains
 *
 * @param[buf] the buffer containing the request $DOWNLOAD$<filename>$
 */
int serv_wrap_upload(int cfd, char *buf) {
	int fsize;
	char *filename, path[BUFSIZE >> 1];
	struct stat st;

	filename = buf + 9;

	/* TODO: verify_upload_input */

	snprintf(path, sizeof(path), HOSTDIR "/%s", filename);

	/* file not found */
	if (stat(path, &st) != 0) {
		if (send(cfd, DLOAD_FAILURE_MSG, sizeof(DLOAD_FAILURE_MSG), 0) == -1) {
			perror("send()");
			return -3;
		}
		return -2;
	}

	/* send fsize */
	fsize = st.st_size;
	if (send(cfd, &fsize, sizeof(int), 0) == -1) {
		perror("send()");
		return -4;
	}

	/* upload file */
	if (serv_upload(filename, fsize, cfd) < 0) {
		perror("upload()");
		return -5;
	}

	return 0;
}

/**
 * @details This function is called when the user opts to UPLOAD a file.
 * 	- Receiving the file's total size from the client
 * 	- Checking if there is enough space in the HOSTDIR to accomodate this file
 * 	- If not, sending back $FAILURE$LOW_SPACE$
 * 	- If yes, sending back $SUCCESS$
 * 	- Calling the main download function (serv_download) using the `bytes`
 * 	  acquired from #2
 * 	- Sending $SUCCESS$ again
 *
 * @param[buf] the buffer containing the request $UPLOAD$<filename>$
 */
int serv_wrap_download(int cfd, char *buf) {
	size_t fsize;
	char *filename;
	__off_t usedSpace;

	printf("ENTERED DOWNLOAD WRAPPER\n");

	filename = buf + 8;

	if (recv(cfd, &fsize, sizeof(fsize), 0) == -1) {
		perror("recv()");
		return -3;
	}

	printf("RECEIVED FILE SIZE\n");

	if ((usedSpace = get_used_space(HOSTDIR)) + fsize > MAX_CLIENT_SPACE) {
		if (send(cfd, ULOAD_FAILURE_MSG, sizeof(ULOAD_FAILURE_MSG), 0) == -1) {
			perror("send()");
			return -5;
		}
		return 4;
	} else if (usedSpace < 0) {
		if (send(cfd, FAILURE_MSG, sizeof(FAILURE_MSG), 0) == -1) {
			perror("send()");
			return -5;
		}
		return -4;
	}

	printf("SPACE AVAILABLE\n");

	if (send(cfd, SUCCESS_MSG, sizeof(SUCCESS_MSG), 0) == -1) {
		perror("send()");
		return -5;
	}

	printf("SPACE SUCCESS MSG SENT\n");

	if (serv_download(filename, fsize, cfd) != 0) {
		fprintf(stderr, "serv_download()\n");
		return -6;
	}

	printf("DOWNLOAD COMPLETE\n");

	if (send(cfd, SUCCESS_MSG, sizeof(SUCCESS_MSG), 0) == -1) {
		perror("send()");
		return -7;
	}

	return 0;
}

/**
 * @brief download a total of 'bytes' bytes from a client, into
 * `filename` file
 */
int serv_download(char *filename, size_t bytes, int cfd) {
	FILE *fp;
	int bytesRead, toRead, ret = 0;
	char *buf, filepath[BUFSIZE << 1];

	snprintf(filepath, sizeof(filepath), HOSTDIR "/%s", filename);

	if ((fp = fopen(filepath, "w")) == NULL) {
		perror("fopen()");
		return 1;
	}

	if ((buf = malloc(BUFSIZE)) == NULL) {
		perror("malloc()");
		ret = 2;
		goto cleanup;
	}

	while (bytes > 0) {

		toRead = min(BUFSIZE, bytes);

		if ((bytesRead = recv(cfd, buf, toRead, 0)) == -1) {
			perror("recv()");
			ret = 3;
			goto cleanup;
		}

		if (bytesRead != toRead) {
			fprintf(stderr, "Error: socket read mismatch!\n");
			ret = 4;
			goto cleanup;
		}

		fwrite(buf, bytesRead, 1, fp);
		if (ferror(fp)) {
			perror("fwrite()");
			ret = 5;
			goto cleanup;
		}

		bytes -= toRead;
	}

cleanup:
	free(buf);
	fclose(fp);
	return ret;
}

int serv_wrap_view(int cfd) {
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

void ensure_srv_dir_exists() {
	struct stat st = {0};
	if (stat(HOSTDIR, &st) == -1) {
		if (mkdir(HOSTDIR, 0700) == -1) {
			perror("mkdir()");
			return;
		}
	}
}

__off_t get_used_space(const char *dir) {
	struct dirent *entry;
	struct stat st;
	char filepath[BUFSIZE];
	int size = 0;
	DIR *d;

	if ((d = opendir(dir)) == NULL) {
		perror("opendir()");
		return -1;
	}

	while ((entry = readdir(d)) != NULL) {
		if (!strcmp(entry->d_name, ".") || strcmp(entry->d_name, "..") == 0)
			continue;

		snprintf(filepath, sizeof(filepath), "%s/%s", dir, entry->d_name);
		if (stat(filepath, &st) == -1) {
			perror("stat()");
			closedir(d);
			return -2;
		}

		size += st.st_size;
	}

	closedir(d);
	return size;
}
