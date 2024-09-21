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

int init_server_socket(struct sockaddr_in *saddr);
int identify_request(char *buf);
void ensure_srv_dir_exists();

int wrap_view(int cfd);
/* TODO: Upload/Download interface */
int wrap_download(int cfd);
int wrap_upload(int cfd);

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

	char retArr[128];

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
		case 1:
			status = wrap_view(cfd);
			break;
		case 2:
			status = wrap_download(cfd);
			break;
		case 3:
			status = wrap_upload(cfd);
			break;
		default:
			memcpy(retArr, FAILURE_MSG, sizeof(FAILURE_MSG));
			if ((send(cfd, retArr, sizeof(FAILURE_MSG), 0)) == -1) {
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

int wrap_upload(int cfd) {
	/*
	 * This is going to get called if the user opted to DOWNLOAD.
	 * 1. we must check if the file exists inside HOSTDIR
	 * 2. If not, send back $FAILURE$FILE_NOT_FOUND$
	 * 3. Else, query that file for it's size, and send() it back
	 * 4. recv() acknowledgement
	 * 5. Call upload function to send() file until nothing remains
	 */

	return cfd;
}

int wrap_download(int cfd) {
	/*
	 * This is going to get called if the user opted to UPLOAD.
	 * 1. we must recv() their file's total size.
	 * 2. we must query the HOSTDIR and check if we have enough space to
	 * 	  accomdate this new file (HOSTDIR_MAXSIZE > hostdirSize + fsize)
	 * 3. If not, send back $FAILURE$LOW_SPACE$
	 * 4. Else, send back $SUCCESS$
	 * 5. Call the main download function using the `bytes` acquired from #2
	 * 6. Send $SUCCESS$ again
	 */

	return cfd;
}

int wrap_view(int cfd) {
	int status = 0;
	ssize_t idx;
	char *ret;

	if ((ret = malloc(BUFSIZE)) == NULL) {
		perror("malloc()");
		return -1;
	}

	/* error while viewing */
	if ((idx = view(ret, BUFSIZE)) < 0) {
		status = -2;
		goto cleanup;
	}

	/* no files */
	if (idx == 0) {
		memcpy(ret, VIEW_FAILURE_MSG, sizeof(VIEW_FAILURE_MSG));
		if ((send(cfd, ret, sizeof(VIEW_FAILURE_MSG), 0)) == -1) {
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

/*
 * TODO: more robust error checking needed,
 * NOTE: `telnet` seems to be appending two additional bytes,
 * one for the carriage return and one for the new line; we must
 * find out if this is telnet-specific or general to all clients,
 * and modify our treatment of bytesRead accordingly, because we
 * need it to validate `buf` later on
 */

int identify_request(char *buf) {
	if (!strncmp(buf, "$VIEW$", 6))
		return 1;
	else if (!strncmp(buf, "$DOWNLOAD$", 10))
		return 2;
	else if (!strncmp(buf, "$UPLOAD$", 8))
		return 3;
	return -1;
}
