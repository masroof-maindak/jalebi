#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <bsd/readpassphrase.h>
#include <readline/readline.h>

#include "../include/bool.h"
#include "../include/client.h"

char select_mode() {
	int c, x, ctr;

	do {
		ctr = 0;
		printf("[L]ogin/[R]egister? ");
		c = getchar();

		/* clean stdin */
		while ((x = getchar()) != '\n' && x != EOF)
			ctr++;

	} while (ctr > 0 || (c != 'L' && c != 'R'));

	return c;
}

/**
 * @brief registers user w/ server; returns 0 on success
 */
int user_registration(int sfd) {
	char *un = NULL, *pw = NULL;
	char buf[64];
	bool unSuccess = false, pwSuccess = false;
	uint8_t unLen = 0, pwLen = 0, ret = 0;
	ssize_t written;

	if ((pw = malloc(PW_MAX_LEN + 1)) == NULL)
		return -1;

	/* get username */
	while (!unSuccess) {
		un		  = readline("Username: ");
		unLen	  = strlen(un);
		unSuccess = unLen >= PW_MIN_LEN && unLen <= PW_MAX_LEN;
		if (!unSuccess)
			free(un);
	}

	/* get password */
	while (!pwSuccess) {
		if (NULL ==
			readpassphrase("Password: ", pw, sizeof(pw), RPP_REQUIRE_TTY)) {
			goto cleanup;
			return -2;
		}

		pwLen	  = strlen(pw);
		pwSuccess = pwLen >= PW_MIN_LEN && pwLen <= PW_MAX_LEN;
	}

	/* format server message */
	if ((written = snprintf(buf, sizeof(buf), "%c%d%d%s%s", 'R', unLen, pwLen, un,
							pw)) < 0) {
		ret = -3;
		goto cleanup;
	}

	/* send to server */
	if (send(sfd, buf, written, 0) == -1) {
		perror("send()");
		ret = -4;
		goto cleanup;
	}

	if ((recv_success(sfd, "Error: server couldn't register!")) < 0)
		ret = -5;

cleanup:
	free(un);
	free(pw);
	return ret;
}

/**
 * @brief tries to log in thrice before exiting; returns 0 on success
 */
int user_login(int sfd) {
	/* int attempts   = 0; */
	/* bool userGiven = false; */

	/* get username */
	/* get password */
	/* send both */
	/* recv_success */

	return 0;
}

int user_auth(int sfd) {
	char opt = select_mode();

	switch (opt) {
	case 'L':
		return user_login(sfd);
	case 'R':
		return user_registration(sfd);
	default:
		break;
	}

	return -1;
}

int main() {
	int sfd, status = 0;
	enum REQUEST reqType;
	struct sockaddr_in saddr;
	char *userInput = NULL;

	if ((sfd = init_client_socket(&saddr)) < 0)
		return 1;

	printf("%sSuccesfully connected to server -- %s:%d%s\n", COL_GREEN,
		   SERVER_IP, SERVER_PORT, COL_RESET);

	status = user_auth(sfd);

	while (status == 0) {
		userInput = readline("namak-paare > ");

		if ((reqType = handle_input(userInput)) < 0)
			continue;

		switch (reqType) {
		case VIEW:
			status = client_wrap_view(sfd);
			break;
		case DOWNLOAD:
			status = client_wrap_download(sfd, userInput);
			break;
		case UPLOAD:
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
		return 0;

	/* tell server we're ready to accept size */
	if (send(sfd, SUCCESS_MSG, sizeof(SUCCESS_MSG), 0) == -1) {
		perror("send()");
		return -2;
	}

	/* recv size */
	if (recv(sfd, &fsize, sizeof(fsize), 0) == -1) {
		perror("recv()");
		return -3;
	}

	/* download file */
	if (download(filename, fsize, sfd) != 0) {
		fprintf(stderr, "download()\n");
		return -4;
	}

	return 0;
}

char *extract_filename_if_exists(const char *fpath, struct stat *fstat) {
	char *fpSave, *fname, *fnSave = NULL;

	if (stat(fpath, fstat) != 0) {
		if (errno == ENOENT) {
			fprintf(stderr, "Error: file does not exist\n");
			return NULL;
		} else {
			perror("stat()");
			return NULL;
		}
	}

	if ((fpSave = copy_string(fpath)) == NULL)
		return NULL;

	if ((fname = strtok(fpSave, "/")) != NULL) {
		do {
			fnSave = fname;
		} while ((fname = strtok(NULL, "/")) != NULL);
	}

	fname = fnSave;

	char *result = copy_string(fname);
	free(fpSave);
	return result;
}

int client_wrap_upload(int sfd, char *buf) {
	char *filepath = buf + 8, *filename = NULL, msg[BUFSIZE];
	struct stat fstat;
	size_t fsize = 0;
	ssize_t written;

	if ((filename = extract_filename_if_exists(filepath, &fstat)) == NULL)
		return -1;

	/* send upload message */
	if ((written = snprintf(msg, sizeof(msg), "$UPLOAD$%s", filename)) < 0)
		return -2;

	if (send(sfd, msg, written, 0) == -1) {
		perror("send()");
		return -3;
	}

	free(filename);

	if ((recv_success(sfd, "Error: something went wrong!")) < 0)
		return -4;

	/* send fsize */
	fsize = fstat.st_size;
	if (send(sfd, &fsize, sizeof(fsize), 0) == -1) {
		perror("send()");
		return -5;
	}

	if ((recv_success(sfd, "Error: Not enough space available!")) < 0)
		return -6;

	if (upload(filepath, fsize, sfd) < 0) {
		perror("upload()");
		return -7;
	}

	if ((recv_success(sfd, "Error: something went wrong!")) < 0)
		return -8;

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

enum REQUEST handle_input(char *userInput) {
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

uint8_t valid_user_input(const char *input, enum REQUEST reqType, size_t len) {
	switch (reqType) {
	case VIEW:
		return input[6] == '\0';
		break;
	case DOWNLOAD:
		if (input[len - 1] == '$' && len > 11 && input[10] != '$')
			return 1;
		break;
	case UPLOAD:
		if (input[len - 1] == '$' && len > 9 && input[8] != '$')
			return 1;
		break;
	case INVALID:
		break;
	}

	return 0;
}
