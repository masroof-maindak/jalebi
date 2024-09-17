#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include "../include/utils.h"

char *copy_string(const char *str) {
	if (str == NULL) {
		fputs("copy_string receieved NULL!\n", stderr);
		return NULL;
	}

	size_t size = strlen(str);
	char *copy	= malloc(size + 1);
	if (copy == NULL) {
		perror("malloc()");
		return NULL;
	}

	memcpy(copy, str, size);
	copy[size] = '\0';
	return copy;
}

/**
 * @brief download `bytes` bytes from the socket behind `sockfd`, into
 * `filename` file
 *
 * @details the client must first send the number of bytes that it is going to
 * send in a separate call; that value is passed as the `bytes` parameter
 */
int download(char *filename, size_t bytes, int sockfd) {
	FILE *fp;
	int bytesRead, toRead;
	char *buf;

	if ((fp = fopen(filename, "b")) == NULL) {
		perror("fopen()");
		return 1;
	}

	if ((buf = malloc(BUFSIZE)) == NULL) {
		perror("malloc()");
		fclose(fp);
		return 2;
	}

	while (bytes > 0) {

		toRead = min(BUFSIZE, bytes);

		if ((bytesRead = recv(sockfd, buf, toRead, 0)) == -1) {
			perror("read()");
			fclose(fp);
			return 3;
		}

		if (bytesRead != toRead) {
			fprintf(stderr, "Didn't read as much as we were expecting...");
			fclose(fp);
			return 4;
		}

		fwrite(buf, bytesRead, 1, fp);
		bytes -= toRead;
	}

	return 0;
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
