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

int main() {
	int sfd, status = 0;
	enum REQ_TYPE reqType;
	struct sockaddr_in saddr;
	char *line = NULL;

	if ((sfd = init_client_socket(&saddr)) < 0)
		return 1;

	printf(GREEN "Succesfully connected to Jalebi -- %s:%d\n" RESET, SERVER_IP,
		   SERVER_PORT);

	if ((status = user_authentication(sfd)) != 0)
		fprintf(stderr, RED "Error: couldn't authenticate user!\n" RESET);

	while (status == 0) {
		line = readline("namak-paare > ");

		if ((reqType = handle_input(line)) < 0)
			continue;

		switch (reqType) {
		case VIEW:
			status = client_wrap_view(sfd);
			break;
		case DOWNLOAD:
			status = client_wrap_download(sfd, line);
			break;
		case UPLOAD:
			status = client_wrap_upload(sfd, line);
			break;
		case 4:
			status = -1;
			break;
		default:
			break;
		}

		free(line);
		line = NULL;
	}

	close(sfd);
	return 0;
}

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

char *get_password(char *pw, uint8_t *pwLen) {
	bool pwSuccess = false;
	char *tmp	   = NULL;

	while (!pwSuccess) {
		if ((tmp = readpassphrase("Password: ", pw, PW_MAX_LEN,
								  RPP_REQUIRE_TTY)) == NULL) {
			return NULL;
		}

		*pwLen	  = strlen(pw);
		pwSuccess = (*pwLen >= PW_MIN_LEN && *pwLen <= PW_MAX_LEN);
	}

	return pw;
}

char *get_username(char *un, uint8_t *unLen) {
	bool unSuccess = false;

	while (!unSuccess) {
		if ((un = readline("Username: ")) == NULL)
			continue;

		*unLen	  = strlen(un);
		unSuccess = *unLen >= PW_MIN_LEN && *unLen <= PW_MAX_LEN;
		if (!unSuccess)
			free(un);
	}

	return un;
}

int send_auth_info(int sfd, char mode, const char *pw, const char *un,
				   uint8_t unL, uint8_t pwL) {
	char buf[64];
	int n;

	n = snprintf(buf, sizeof(buf), "%c%c%c%s%s", mode, unL, pwL, un, pw);
	if (n < 0)
		return -1;

	if (send(sfd, buf, n, 0) == -1) {
		perror("send() in send_auth_info()");
		return -2;
	}

	return 0;
}

/**
 * @brief register OR log-in user w/ server; returns 0 on success
 */
int user_authentication(int sfd) {
	char *un = NULL, *pw = NULL, opt = select_mode();
	uint8_t unLen = 0, pwLen = 0, ret = 0;

	pw = malloc(PW_MAX_LEN + 1);
	if (pw == NULL) {
		perror("malloc() in user_authentication()");
		return -1;
	}

	un = get_username(un, &unLen);
	if ((pw = get_password(pw, &pwLen)) == NULL) {
		ret = -2;
		goto cleanup;
	}

	if (send_auth_info(sfd, opt, pw, un, unLen, pwLen) != 0) {
		ret = -3;
		goto cleanup;
	}

	if ((recv_success(sfd, NULL)) < 0)
		ret = -4;

cleanup:
	free(un);
	free(pw);
	return ret;
}

int client_wrap_download(int sfd, const char *buf) {
	const char *fname = buf + 10;
	size_t fsize	  = 0;

	/* send download message */
	if (send(sfd, buf, strlen(buf), 0) == -1) {
		perror("send() #1 in client_wrap_download()");
		return -1;
	}

	/* either it wasn't there or we get back the filesize */
	if ((recv_success(sfd, "File not available on server, try $VIEW$")) < 0)
		return 0;

	/* tell server we're ready to accept size */
	if (send(sfd, SUCCESS_MSG, sizeof(SUCCESS_MSG), 0) == -1) {
		perror("send() #2 in client_wrap_download()");
		return -2;
	}

	/* recv size */
	if (recv(sfd, &fsize, sizeof(fsize), 0) == -1) {
		perror("recv() in client_wrap_download()");
		return -3;
	}

	/* download file */
	if (download_file(fname, fsize, sfd) != 0)
		return -4;

	return 0;
}

char *extract_filename_if_exists(const char *fpath, struct stat *fstat) {
	char *fpSave, *fname, *fnSave = NULL;

	if (stat(fpath, fstat) != 0) {
		if (errno == ENOENT) {
			fprintf(stderr,
					"extract_filename_if_exists(): non-existent file\n");
			return NULL;
		} else {
			perror("stat() in extract_filename_if_exists()");
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

int client_wrap_upload(int sfd, const char *buf) {
	char const *fpath;
	char *fname = NULL, msg[BUFSIZE];
	struct stat fstat;
	size_t fsize = 0;
	int n;

	fpath = buf + 8;

	if ((fname = extract_filename_if_exists(fpath, &fstat)) == NULL)
		return -1;

	/* send upload message */
	if ((n = snprintf(msg, sizeof(msg), "$UPLOAD$%s", fname)) < 0)
		return -2;

	if (send(sfd, msg, n, 0) == -1) {
		perror("send() #1 in client_wrap_upload()");
		return -3;
	}

	free(fname);

	if ((recv_success(sfd, "Error: something went wrong!")) < 0)
		return -4;

	/* send fsize */
	fsize = fstat.st_size;
	if (send(sfd, &fsize, sizeof(fsize), 0) == -1) {
		perror("send() #2 in client_wrap_upload()");
		return -5;
	}

	if ((recv_success(sfd, "Error: Not enough space available!")) < 0)
		return -6;

	if (upload_file(fpath, fsize, sfd) < 0)
		return -7;

	if ((recv_success(sfd, "Error: something went wrong!")) < 0)
		return -8;

	printf(GREEN "File uploaded to server successfully!\n" RESET);
	return 0;
}

int client_wrap_view(int sfd) {
	int status = 0;
	ssize_t idx;
	char *buf;

	if (send(sfd, "$VIEW$", 6, 0) == -1) {
		perror("send() in client_wrap_view()");
		return -1;
	}

	if (recv(sfd, &idx, sizeof(idx), 0) == -1) {
		perror("recv() in client_wrap_view()");
		return -2;
	}

	if (idx == 0) {
		fprintf(stderr, RED "No files on server\n" RESET);
		return 0;
	}

	if ((buf = malloc(idx + 1)) == NULL) {
		perror("malloc() in client_wrap_view()");
		return -3;
	}

	buf[idx] = '\0';
	for (int i = 0; idx > 0; i++, idx -= BUFSIZE) {
		if ((recv(sfd, buf + (i << 10), min(BUFSIZE, idx), 0)) == -1) {
			perror("recv() in client_wrap_view()");
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
		perror("socket() in init_client_socket()");
		return -1;
	}

	saddr->sin_family = AF_INET;
	saddr->sin_port	  = htons(SERVER_PORT);
	memset(saddr->sin_zero, '\0', sizeof(saddr->sin_zero));

	if (inet_pton(AF_INET, SERVER_IP, &saddr->sin_addr) <= 0) {
		perror("inet_pton() in init_client_socket()");
		return -2;
	}

	if (connect(sfd, (struct sockaddr *)saddr, sizeof(*saddr)) < 0) {
		perror("connect() in init_client_socket()");
		close(sfd);
		return -3;
	}

	return sfd;
}

enum REQ_TYPE handle_input(char *userInput) {
	size_t len	= strlen(userInput);
	int reqType = -1;

	if (strcmp(userInput, "exit") == 0)
		return 4;
	else if (strlen(userInput) == 0)
		return -1;

	if ((reqType = identify_req_type(userInput)) == -1) {
		fprintf(stderr, "%sError: invalid command!%s\n", RED, RESET);
		return -2;
	}

	if (!valid_user_input(userInput, reqType, len)) {
		fprintf(stderr, "%sError: invalid format!%s\n", RED, RESET);
		return -3;
	}

	userInput[len - 1] = '\0';
	return reqType;
}

uint8_t valid_user_input(const char *input, enum REQ_TYPE reqType, size_t len) {
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
