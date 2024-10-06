#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <readline/readline.h>

#include "../include/client.h"
#include "../include/utils.h"

int main() {
	signal(SIGINT, SIG_IGN);
	int sfd, reqType, status = 0;
	struct sockaddr_in saddr;
	char *userInput = NULL;

	if ((sfd = init_client_socket(&saddr)) < 0)
		return 1;

	printf("%sSuccesfully connected to server -- %s:%d%s\n", COL_GREEN,
		   SERVER_IP, SERVER_PORT, COL_RESET);

	/* TODO: login/registration... */

	while (status == 0) {
		userInput = readline("namak-paare > ");

		if ((reqType = handle_input(userInput)) < 0)
			continue;

		switch (reqType) {
		case 1: /* $VIEW$\n */
			status = client_wrap_view(sfd);
			break;
		case 2: /* $DOWNLOAD$<filename>$\n */
			status = client_wrap_download(sfd, userInput);
			break;
		case 3: /* $UPLOAD$<filename>$\n */
			status = client_wrap_upload(sfd, userInput);
			break;
		case 4:
			status = -1;
			break;
		default:
			break;
		}

		free(userInput);
		userInput = NULL;
	}

	close(sfd);
	return 0;
}

int client_wrap_download(int sfd, char *buf) {
	char *filename;
	size_t fsize = 0;

	filename = buf + 10;

	/* send download message */
	if (send(sfd, buf, strlen(buf), 0) == -1) {
		perror("send()");
		return -1;
	}

	/* either it wasn't there or we get back the filesize */
	if ((recv_success(sfd, "File not available on server, try $VIEW$")) < 0)
		return -2;

	/* tell server we're ready to accept size */
	if (send(sfd, SUCCESS_MSG, sizeof(SUCCESS_MSG), 0) == -1) {
		perror("send()");
		return -3;
	}

	/* recv size */
	if (recv(sfd, &fsize, sizeof(fsize), 0) == -1) {
		perror("recv()");
		return -4;
	}

	/* download file */
	if (download(filename, fsize, sfd) != 0) {
		fprintf(stderr, "download()\n");
		return -5;
	}

	return 0;
}

int client_wrap_upload(int sfd, char *buf) {
	char *filepath, *filename, *saveFilename = NULL, msg[BUFSIZE];
	struct stat fstat;
	size_t fsize = 0, written;

	filepath = buf + 8;
	if (stat(filepath, &fstat) != 0) {
		if (errno == ENOENT) {
			fprintf(stderr, "Error: file does not exist\n");
			return 0;
		} else {
			perror("stat()");
			return -2;
		}
	}

	/* FIXME */
	filename = strtok(filepath, "/");
	if ((filename = strtok(filepath, "/")) != NULL) {
		do {
			saveFilename = filename;
		} while ((filename = strtok(NULL, "/")) != NULL);
	}

	/* send upload message */
	memset(msg, 0, sizeof(msg));
	written = snprintf(msg, sizeof(msg), "$UPLOAD$%s", saveFilename);
	printf("SENDING ULOAD MSG OF SIZE: %ld\n", written);
	printf("%s\n", msg);
	if (send(sfd, msg, written, 0) == -1) {
		perror("send()");
		return -1;
	}

	if ((recv_success(sfd, "Error: something went wrong!")) < 0)
		return -2;

	/* send fsize */
	fsize = fstat.st_size;
	if (send(sfd, &fsize, sizeof(fsize), 0) == -1) {
		perror("send()");
		return -3;
	}

	if ((recv_success(sfd, "Error: Not enough space available!")) < 0)
		return -4;

	if (upload(saveFilename, fsize, sfd) < 0) {
		perror("upload()");
		return -5;
	}

	if ((recv_success(sfd, "Error: something went wrong!")) < 0)
		return -4;

	printf(COL_GREEN "File uploaded to server successfully!\n" COL_RESET);
	return 0;
}

int client_wrap_view(int sfd) {
	int status = 0;
	ssize_t idx;
	char *buf;

	if (send(sfd, "$VIEW$", 6, 0) == -1) {
		perror("send()");
		return -1;
	}

	if (recv(sfd, &idx, sizeof(idx), 0) == -1) {
		perror("recv()");
		return -2;
	}

	if ((buf = malloc(idx + 1)) == NULL) {
		perror("malloc()");
		return -3;
	}

	buf[idx] = '\0';
	for (int i = 0; idx > 0; i++, idx -= BUFSIZE) {
		if ((recv(sfd, buf + (i << 10), min(BUFSIZE, idx), 0)) == -1) {
			perror("recv()");
			free(buf);
			return -4;
		}
	}

	printf("%s", buf);
	free(buf);
	return status;
}

int init_client_socket(struct sockaddr_in *saddr) {
	int sfd;

	if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket()");
		return -1;
	}

	saddr->sin_family = AF_INET;
	saddr->sin_port	  = htons(SERVER_PORT);
	memset(saddr->sin_zero, '\0', sizeof(saddr->sin_zero));

	if (inet_pton(AF_INET, SERVER_IP, &saddr->sin_addr) <= 0) {
		perror("inet_pton()");
		return -2;
	}

	if (connect(sfd, (struct sockaddr *)saddr, sizeof(*saddr)) < 0) {
		perror("connect()");
		close(sfd);
		return -3;
	}

	return sfd;
}

int handle_input(char *userInput) {
	size_t len	= strlen(userInput);
	int reqType = -1;

	if (strcmp(userInput, "exit") == 0)
		return 4;
	else if (strlen(userInput) == 0)
		return -1;

	if ((reqType = identify_request(userInput)) == -1) {
		fprintf(stderr, "%sError: invalid command!%s\n", COL_RED, COL_RESET);
		return -2;
	}

	if (!valid_user_input(userInput, reqType, len)) {
		fprintf(stderr, "%sError: invalid format!%s\n", COL_RED, COL_RESET);
		return -3;
	}

	userInput[len - 1] = '\0';
	return reqType;
}

uint8_t valid_user_input(const char *input, int reqType, size_t len) {
	switch (reqType) {
	case 1:
		return input[6] == '\0';
		break;
	case 2:
		if (input[len - 1] == '$')
			if (len > 11 && input[10] != '$')
				return 1;
		break;
	case 3:
		if (input[len - 1] == '$')
			if (len > 9 && input[8] != '$')
				return 1;
		break;
	}

	return 0;
}
