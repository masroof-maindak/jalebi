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

void download(char *filename, size_t bytes, int sockfd) {
	FILE *fp;
	int bytesRead, toRead;
	char *buf;

	if ((fp = fopen(filename, "b")) == NULL) {
		perror("fopen()");
		return;
	}

	if ((buf = malloc(BUFSIZE)) == NULL) {
		perror("malloc()");
		goto cleanup;
	}

	while (bytes > 0) {

		toRead = min(BUFSIZE, bytes);

		if ((bytesRead = recv(sockfd, buf, toRead, 0)) == -1) {
			perror("read()");
			goto cleanup;
		}

		if (bytesRead != toRead) {
			fprintf(stderr, "Didn't read as much as we were expecting...");
			goto cleanup;
		}

		fwrite(buf, bytesRead, 1, fp);
		bytes -= toRead;
	}

cleanup:
	fclose(fp);
	return;
}

void ensure_srv_dir_exists() {
	char *hostDir = "srv";

	struct stat st = {0};
	if (stat(hostDir, &st) == -1) {
		if (mkdir(hostDir, 0700) == -1) {
			perror("mkdir()");
			return;
		}
	}
}

char *read_file(const char *filename) {
	FILE *file = fopen(filename, "rb");
	if (!file) {
		perror("Failed to open file");
		return NULL;
	}
	fseek(file, 0, SEEK_END);
	long filesize = ftell(file);
	fseek(file, 0, SEEK_SET);
	char *buffer = malloc(filesize + 1);
	if (!buffer) {
		perror("Failed to allocate memory");
		fclose(file);
		return NULL;
	}

	size_t bytesRead  = fread(buffer, 1, filesize, file);
	buffer[bytesRead] = '\0';

	fclose(file);
	return buffer;
}