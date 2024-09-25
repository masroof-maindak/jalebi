#include <dirent.h>
#include <stdint.h>
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

int identify_request(char *buf) {
	if (!strncmp(buf, "$VIEW$", 6))
		return 1;
	else if (!strncmp(buf, "$DOWNLOAD$", 10))
		return 2;
	else if (!strncmp(buf, "$UPLOAD$", 8))
		return 3;
	return -1;
}
