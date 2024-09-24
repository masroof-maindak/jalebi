#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../include/client.h"
#include "../include/utils.h"

int handle_input(char userInput[]);
int init_client_socket(struct sockaddr_in *saddr);
uint8_t validate_user_input(const char *input, int reqType);

int client_wrap_view(int sfd);
int client_wrap_upload(int sfd, char *buf);

int main() {
	signal(SIGINT, SIG_IGN);
	int sfd, reqType, status = 0;
	struct sockaddr_in saddr;
	char userInput[BUFSIZE];

	if ((sfd = init_client_socket(&saddr)) < 0)
		return 1;

	printf("%sSuccesfully connected to server -- %s:%d%s\n", COL_GREEN,
		   SERVER_IP, SERVER_PORT, COL_RESET);

	/* TODO: login/registration... */

	while (status == 0) {
		if ((reqType = handle_input(userInput)) < 0)
			continue;

		switch (reqType) {
		case 1: /* $VIEW$\n */
			status = client_wrap_view(sfd);
			break;
		case 2: /* $DOWNLOAD$<filename>$\n */
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
	}

	close(sfd);
	return 0;
}

int client_wrap_upload(int sfd, char *buf) {
	char *filepath, *filename;
	size_t fsize = 0, written;
	char msg[BUFSIZE];
	struct stat fstat;

	filepath = buf + 8;
	/* TODO: extract file name from path; just one strtok? */
	filename = filepath;

	/* verify existence */
	if (stat(filepath, &fstat) != 0) {
		if (errno == EBADF) {
			fprintf(stderr, "%sError: file does not exist\n%s", COL_RED,
					COL_RESET);
			return 0;
		} else {
			perror("stat()");
			return -2;
		}
	}

	/* send upload message */
	written = snprintf(msg, sizeof(msg), "$UPLOAD$%s$", filename);
	if (send(sfd, msg, written, 0) == -1) {
		perror("send()");
		return -1;
	}

	/* send fsize */
	fsize = fstat.st_size;
	if (send(sfd, &fsize, sizeof(fsize), 0) == -1) {
		perror("send()");
		return -3;
	}

	printf("here1\n\n");

	/* recv response */
	if (recv(sfd, msg, sizeof(msg), 0) == -1) {
		perror("recv()");
		return -4;
	}

	printf("here2\n\n");

	/* check response */

	if (!(strncmp(msg, SUCCESS_MSG, sizeof(SUCCESS_MSG)) == 0)) {
		fprintf(stderr, "Error: Not enough space available!\n");
		return -5;
	}

	puts("here3\n\n");

	/* upload file */
	// FIXME
	if (upload(filename, fsize, sfd) < 0) {
		perror("upload()");
		return -6;
	}
	puts("here3");

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

/* FIXME */
uint8_t validate_user_input(const char *input, int reqType) {
	return 0;

	/*
	 * Validate input depending on what type of request it is
	 * - e.g for $VIEW$, nothing should lie after the second '$'
	 * - For download and upload, there must be a string between
	 *   the second and third '$'s
	 */

	size_t len = strlen(input);
	switch (reqType) {
	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	}

	if (len < 7)
		return 0;

	if (!strncmp(input, "$VIEW$", 6) && input[6] == '\n')
		return 1;

	if (strncmp(input, "$DOWNLOAD$", 10) == 0) {
		if (len > 11 && input[len - 2] != '$')
			return 0;
		return 1;
	}

	if (strncmp(input, "$UPLOAD$", 8) == 0) {
		if (len > 9 && input[len - 2] != '$')
			return 0;
		return 1;
	}

	return 0;
}

int handle_input(char userInput[]) {
	int reqType = -1;

	printf("namak-paare > ");

	if (!fgets(userInput, USER_INPUT_LIMIT, stdin)) {
		puts("");
		return 4;
	}

	userInput[strcspn(userInput, "\n")] = 0;

	if (strcmp(userInput, "exit") == 0)
		return 4;
	else if (strlen(userInput) == 0)
		return -1;

	if ((reqType = identify_request(userInput)) == -1) {
		fprintf(stderr, "%sError: invalid command!%s\n", COL_RED, COL_RESET);
		return -2;
	}

	if (validate_user_input(userInput, reqType) != 0) {
		fprintf(stderr, "%sError: invalid format!%s\n", COL_RED, COL_RESET);
		return -3;
	}

	return reqType;
}
