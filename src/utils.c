
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

int get_num_digits(__off_t n) {
	int r = 1;
	for (; n > 9; n /= 10, r++)
		;
	return r;
}

char *double_if_of(char *buf, int idx, int addition, int *size) {
	char *tmp = NULL;

	if (idx + addition > *size) {
		*size *= 2;

		if ((tmp = realloc(buf, *size)) == NULL) {
			perror("realloc()");
			free(buf);
			return NULL;
		}
	}

	return buf;
}

ssize_t view(char *buf, int size) {
	DIR *d;
	char path[BUFSIZE >> 1];
	int idx = 0, entSz, sz;
	struct dirent *ent;
	struct stat inf;

	if ((d = opendir(HOSTDIR)) == NULL) {
		perror("opendir()");
		return -1;
	}

	while ((ent = readdir(d)) != NULL) {
		if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
			continue;

		sprintf(path, HOSTDIR "/%s", ent->d_name);
		if ((stat(path, &inf)) != 0) {
			perror("stat()");
			idx = -2;
			goto cleanup;
		}

		entSz = strlen(ent->d_name) + get_num_digits(inf.st_size) + 5;
		if ((buf = double_if_of(buf, idx, entSz, &size)) == NULL) {
			idx = -3;
			goto cleanup;
		}

		sz = snprintf(buf + idx, entSz, "%s - %ld\n", ent->d_name, inf.st_size);
		if (sz < 0) {
			idx = -4;
			goto cleanup;
		}
		idx += sz;
	}

	buf[idx] = '\0';
cleanup:
	closedir(d);
	return idx;
}

/**
 * @brief upload `bytes` bytes to the socket behind `sockfd`, from
 * `filename` file
 *
 * @note the file's existence must be guaranteed before calling this function
 */
int upload(char *filename, size_t bytes, int sockfd) {
	FILE *fp;
	int bytesRead, toWrite, ret = 0;
	char *buf;

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

		if ((bytesRead = fread(buf, 1, toWrite, fp)) == -1) {
			perror("fread()");
			fclose(fp);
			return 4;
		}

		if (bytesRead != toWrite) {
			fprintf(stderr, "Error: file read mismatch!");
			ret = 4;
			goto cleanup;
		}

		if (send(sockfd, buf, bytesRead, 0) == -1) {
			perror("send()");
			ret = 5;
			goto cleanup;
		}

		bytes -= bytesRead;
	}

cleanup:
	free(buf);
	fclose(fp);
	return ret;
}

/**
 * @brief download `bytes` bytes from the socket behind `sockfd`, into
 * `filename` file
 *
 * @details the side that is downloading must first recv() the number of bytes
 * that it is going to send in a separate call; that value is passed as the
 * `bytes` parameter. Then, the other party must, in similar and simultaneous
 * fashion, send over the chunks of that file until there are none left
 */
int download(char *filename, size_t bytes, int sockfd) {
	FILE *fp;
	int bytesRead, toRead, ret = 0;
	char *buf;

	if ((fp = fopen(filename, "w")) == NULL) {
		perror("fopen()");
		return 1;
	}

	if ((buf = malloc(BUFSIZE)) == NULL) {
		perror("malloc()");
		ret = 2;
		goto cleanup;
	}

	while (bytes > 0) {

		toRead = min(BUFSIZE, bytes);

		if ((bytesRead = recv(sockfd, buf, toRead, 0)) == -1) {
			perror("recv()");
			ret = 3;
			goto cleanup;
		}

		if (bytesRead != toRead) {
			fprintf(stderr, "Error: socket read mismatch!");
			ret = 4;
			goto cleanup;
		}

		fwrite(buf, bytesRead, 1, fp);
		if (ferror(fp)) {
			perror("fwrite()");
			ret = 5;
			goto cleanup;
		}

		bytes -= toRead;
	}

cleanup:
	free(buf);
	fclose(fp);
	return ret;
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