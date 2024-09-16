#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "../include/bool.h"
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
