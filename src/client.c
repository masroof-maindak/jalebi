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

#include <readline/readline.h>

#include "../include/client.h"
#include "../include/encode.h"
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

int send_encoded_buffer(int cfd, char *encodedBuf, size_t bufSize) {
	size_t bytesSent = 0;
	int ret			 = 0;

	while (bytesSent < bufSize) {
		size_t chunkSize =
			(bufSize - bytesSent) > BUFSIZE ? BUFSIZE : (bufSize - bytesSent);

		if (send(cfd, &chunkSize, sizeof(chunkSize), 0) == -1) {
			perror("send() chunk size");
			ret = -1;
			break;
		}

		if (send(cfd, encodedBuf + bytesSent, chunkSize, 0) == -1) {
			perror("send() chunk data");
			ret = -1;
			break;
		}
		bytesSent += chunkSize;
	}

	return ret;
}
int client_upload(char *filename, size_t bytes, int cfd) {
	int bytesRead, toWrite, ret = 0;
	FILE *fp;
	char *buf;
	int bufSize;
	char *encodedBuf;

	// TODO: add encoding

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

		if ((encodedBuf = malloc(bytesRead) == NULL)) {
			perror("malloc()");
			ret = 5;
			goto cleanup;
		}
		bufSize = run_length_encode(buf, encodedBuf, bytesRead);
		if (bufSize < 0) {
			fprintf(stderr, "run_lenghth_encode():");
			ret = 6;
			goto cleanup;
		}

		if (send_encoded_buffer(cfd, encodedBuf, bufSize) < 0) {
			fprintf(stderr, "send()");
			ret = 7;
			goto cleanup;
		}

		bytes -= bytesRead;
	}

cleanup:
	if (encodedBuf != NULL) {
		free(encodedBuf);
	}
	free(buf);
	fclose(fp);
	return ret;
}

int recv_success(int sfd, char *errMsg) {
	char msg[BUFSIZE];

	if (recv(sfd, msg, sizeof(msg), 0) == -1) {
		perror("recv()");
		return -4;
	}

	if (!(strncmp(msg, SUCCESS_MSG, sizeof(SUCCESS_MSG)) == 0)) {
		fprintf(stderr, "%s\n", errMsg);
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

	filename = strtok(filepath, "/");
	if ((filename = strtok(filepath, "/")) != NULL) {
		do {
			saveFilename = filename;
		} while ((filename = strtok(NULL, "/")) != NULL);
	}

	/* send upload message */
	memset(msg, 0, sizeof(msg));
	written = snprintf(msg, sizeof(msg), "$UPLOAD$%s", saveFilename);
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

	if ((recv_success(sfd, "Error: Not enough space available!")) < 0)
		return -4;

	if (client_upload(saveFilename, fsize, sfd) < 0) {
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
