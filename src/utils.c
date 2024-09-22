#include <dirent.h>
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

uint32_t get_num_digits(__off_t n) {
	int r = 1;
	for (; n > 9; n /= 10, r++)
		;
	return r;
}

char *double_if_of(char *buf, size_t idx, size_t addition, size_t *size) {
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

ssize_t view(char *buf, size_t size) {
	DIR *d;
	size_t idx = 0, entSz;
	char path[BUFSIZE >> 1];
	int sz;
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
 * @brief upload `bytes` bytes, from `filename` file, to the client's socket
 *
 * @note the file's existence must be guaranteed before calling this function
 */
int serv_upload(char *filename, size_t bytes, int cfd) {
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
			ret = 3;
			goto cleanup;
		}

		if (bytesRead != toWrite) {
			fprintf(stderr, "Error: file read mismatch!");
			ret = 4;
			goto cleanup;
		}

		if (send(cfd, buf, bytesRead, 0) == -1) {
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
 * @brief download a total of 'bytes' bytes from a client, into
 * `filename` file
 */
int serv_download(char *filename, size_t bytes, int cfd) {
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

		if ((bytesRead = recv(cfd, buf, toRead, 0)) == -1) {
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

/* NOTE: see message above `identify_request` */
int validate_download_request(const char *req) {
	char filename[1024];
	char path[1024];
	int prefixLen = strlen("$DOWNLOAD$");
	struct stat st;

	int filenameLen = strlen(req) - prefixLen;

	if (filenameLen <= 0 || filenameLen >= sizeof(filename))
		return -1;

	strncpy(filename, req + strlen("$DOWNLOAD$"), filenameLen);
	filename[filenameLen] = '\0';

	if (filename[filenameLen - 1] != '$')
		return -2;

	snprintf(path, sizeof(path), HOSTDIR "/%s", filename);
	if (stat(path, &st) != 0)
		return -3;

	return 0;
}
