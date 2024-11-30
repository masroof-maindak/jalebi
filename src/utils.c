#include <dirent.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include "../include/utils.h"

char *copy_string(const char *str) {
	if (str == NULL) {
		fprintf(stderr, "copy_string receieved NULL\n");
		return NULL;
	}

	size_t size = strlen(str);
	char *copy	= malloc(size + 1);
	if (copy == NULL) {
		perror("malloc() in copy_string()");
		return NULL;
	}

	memcpy(copy, str, size);
	copy[size] = '\0';
	return copy;
}

uint8_t get_num_digits(__off_t n) {
	int r = 1;
	for (; n > 9; n /= 10, r++)
		;
	return r;
}

/**
 * @details if adding `add` bytes to `buf`, (whose maximum capacity is
 * `size` and currently has `idx` bytes written), would overflow it, then double
 * `buf`. `size` is updated in this case.
 *
 * @return NULL on failure, buffer on success
 */
char *double_if_Of(char *buf, size_t idx, size_t add, size_t *size) {
	char *tmp = NULL;

	if (idx + add > *size) {
		*size *= 2;
		if ((tmp = realloc(buf, *size)) == NULL) {
			perror("realloc() in double_if_Of()");
			free(buf);
			return NULL;
		}
		buf = tmp;
	}

	return buf;
}

/**
 * @brief store information of the files in a `udir` directory, into a buffer,
 * `buf`, of `size` size; reallocating it if neccessary
 */
ssize_t view(char *buf, size_t size, const char *udir) {
	DIR *d;
	size_t idx = 0, entLen;
	int n;
	char upath[PATH_MAX];
	struct dirent *ent;
	struct stat inf;

	if ((d = opendir(udir)) == NULL) {
		perror("opendir() in view()");
		return -1;
	}

	while ((ent = readdir(d)) != NULL) {
		if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
			continue;

		if (snprintf(upath, PATH_MAX, "%s/%s", udir, ent->d_name) < 0) {
			idx = -2;
			goto cleanup;
		}

		if ((stat(upath, &inf)) != 0) {
			perror("stat() in view()");
			idx = -3;
			goto cleanup;
		}

		entLen = strlen(ent->d_name) + get_num_digits(inf.st_size) + 5;
		if ((buf = double_if_Of(buf, idx, entLen, &size)) == NULL) {
			idx = -4;
			goto cleanup;
		}

		n = snprintf(buf + idx, entLen, "%s - %ld\n", ent->d_name, inf.st_size);
		if (n < 0) {
			idx = -5;
			goto cleanup;
		}

		idx += n;
	}

	buf[idx] = '\0';
cleanup:
	closedir(d);
	return idx;
}

enum REQ_TYPE identify_req_type(const char *buf) {
	if (strncmp(buf, "$VIEW$", 6) == 0)
		return VIEW;
	else if (strncmp(buf, "$DOWNLOAD$", 10) == 0)
		return DOWNLOAD;
	else if (strncmp(buf, "$UPLOAD$", 8) == 0)
		return UPLOAD;
	return INVALID;
}

/**
 * @brief download a total of 'bytes' bytes from a socket, into
 * `fname` file
 */
int download_file(const char *fname, size_t bytes, int sfd) {
	FILE *fp;
	size_t bytesRead, toRead;
	ssize_t n;
	int ret	  = 0;
	char *buf = NULL;

	if ((fp = fopen(fname, "w")) == NULL) {
		perror("fopen() in download()");
		return 1;
	}

	if ((buf = malloc(BUFSIZE)) == NULL) {
		perror("malloc() in download()");
		fclose(fp);
		return 2;
	}

	while (bytes > 0) {
		toRead = min(BUFSIZE, bytes);

		if ((n = recv(sfd, buf, toRead, 0)) == -1) {
			perror("recv() in download_file()");
			ret = 3;
			goto cleanup;
		}

		bytesRead = n;

		if (bytesRead < toRead) {
			fprintf(stderr, "download_file(): socket read mismatch!\n");
			ret = 4;
			goto cleanup;
		}

		if (fwrite(buf, 1, bytesRead, fp) < bytesRead) {
			if (ferror(fp)) {
				perror("fwrite() in download()");
				ret = 5;
				goto cleanup;
			}
		}

		bytes -= toRead;
	}

cleanup:
	free(buf);
	fclose(fp);
	return ret;
}

/**
 * @brief upload `bytes` bytes, from `fname` file, to the socket
 *
 * @note the file's existence must be guaranteed before calling this function
 */
int upload_file(const char *fname, size_t bytes, int sfd) {
	FILE *fp;
	int bytesRead, toWrite, ret = 0;
	char *buf;

	if ((fp = fopen(fname, "r")) == NULL) {
		perror("fopen() in upload()");
		return 1;
	}

	if ((buf = malloc(BUFSIZE)) == NULL) {
		perror("malloc() in upload()");
		fclose(fp);
		return 2;
	}

	while (bytes > 0) {

		toWrite = min(BUFSIZE, bytes);

		bytesRead = fread(buf, 1, toWrite, fp);
		if (bytesRead != toWrite) {
			if (feof(fp)) {
				fprintf(stderr, "fread() in upload_file - EOF occurred");
				ret = 3;
				goto cleanup;
			} else if (ferror(fp)) {
				perror("fread() in upload_file()");
				ret = 4;
				goto cleanup;
			}
		}

		if (send(sfd, buf, bytesRead, 0) == -1) {
			perror("send() in upload()");
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
 * @details if we `recv` '$SUCCESS$' from the other side, then return 0, else
 * print the error message in ERR*
 */
int recv_success(int sockfd, const char *err) {
	char msg[BUFSIZE];

	if (recv(sockfd, msg, sizeof(msg), 0) == -1) {
		perror("recv() in recv_success()");
		return -1;
	}

	if (!(strncmp(msg, SUCCESS_MSG, sizeof(SUCCESS_MSG)) == 0)) {
		if (err != NULL)
			fprintf(stderr, RED "%s\n" RESET, err);
		return -2;
	}

	return 0;
}
