#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../include/server.h"
#include "../include/utils.h"

int main() {
	ensure_srv_dir_exists();
	int sfd, cfd, reqType, status, ret = 0;
	ssize_t bytesRead;
	char *buf;
	struct sockaddr_in saddr;
	struct sockaddr_storage caddr;
	socklen_t addrSize = sizeof(caddr);

	printf("Listening on port %d...\n", SERVER_PORT);

	if ((sfd = init_server_socket(&saddr)) < 0)
		return 1;

	if ((buf = malloc(BUFSIZE)) == NULL) {
		perror("malloc()");
		return 2;
	}

	for (;;) {
		reqType = -1;
		status	= 0;

		if ((cfd = accept(sfd, (struct sockaddr *)&caddr, &addrSize)) == -1) {
			perror("accept()");
			ret = 3;
			goto cleanup;
		}

		if ((bytesRead = recv(cfd, buf, BUFSIZE, 0)) == -1) {
			perror("recv()");
			ret = 4;
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
				ret = 3;
				goto cleanup;
			}
		}

		if ((close(cfd)) == -1) {
			perror("close(cfd)");
			ret = 4;
			goto cleanup;
		}

		if (status != 0)
			break;
	}

cleanup:
	free(buf);
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
	char *filename, path[BUFSIZE >> 1], fsizeResp[128];
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
	int fsize;
	char *filename;

	filename = buf + 10;

	/* TODO: validate_download_input */
	/* if (validate_download_input(buf) != 0) { */
	/* 	if (send(cfd, FAILURE_MSG, sizeof(FAILURE_MSG), 0) == -1) { */
	/* 		perror("send()"); */
	/* 		return -1; */
	/* 	} */
	/* 	return -2; */
	/* } */

	if (recv(cfd, &fsize, sizeof(int), 0) == -1) {
		perror("recv()");
		return -2;
	}

	/* TODO: check available space here and error out if none */

	if (send(cfd, SUCCESS_MSG, sizeof(SUCCESS_MSG), 0) == -1) {
		perror("send()");
		return -3;
	}

	if (serv_download(filename, fsize, cfd) != 0) {
		perror("download()");
		return -4;
	}

	if (send(cfd, SUCCESS_MSG, sizeof(SUCCESS_MSG), 0) == -1) {
		perror("send()");
		return -5;
	}

	return 0;
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
		fputs("Internal error occured while `view`ing!\n", stderr);
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
