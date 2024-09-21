#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

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

int view(int cfd) {
	DIR *d;
	char *buf, *path;
	int idx = 0, localSize = BUFSIZE, iter, entrySize;
	struct dirent *entry;
	struct stat info;

	if ((d = opendir(HOSTDIR)) == NULL) {
		perror("opendir()");
		return 1;
	}

	buf	 = malloc(BUFSIZE);
	path = malloc(BUFSIZE >> 1);

	if (buf == NULL || path == NULL) {
		perror("malloc()");
		closedir(d);
		return 2;
	}

	while ((entry = readdir(d)) != NULL) {

		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			continue;

		sprintf(path, HOSTDIR "/%s", entry->d_name);
		if ((stat(path, &info)) != 0) {
			perror("stat()");
			free(buf);
			closedir(d);
			return 3;
		}

		entrySize = strlen(entry->d_name) + get_num_digits(info.st_size) + 4;
		if ((buf = double_if_of(buf, idx, entrySize, &localSize)) == NULL) {
			closedir(d);
			return 4;
		}

		idx += sprintf(buf + idx, "%s - %ld\n", entry->d_name, info.st_size);
	}

	buf[idx] = '\0';

	/*
	 * TODO: abstract this out to where this is called from
	 * NOTE: view should only populate buf, the server should return it
	 */
	for (iter = 0; idx > 0; iter++, idx -= BUFSIZE) {
		if ((send(cfd, buf + (iter << 10), min(BUFSIZE, idx), 0)) == -1) {
			perror("send()");
			closedir(d);
			return 5;
		}
	}
	free(buf);

	free(path);
	return closedir(d);
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



char* run_length_encode(char *filename) {
    FILE* fp;
    char current, prev;
    size_t current_length = 1024;
    char* err = "error";
    char* result;
    result = (char*)malloc(current_length);
    if (result == NULL) {
        perror("Memory allocation failed");
        return err;
    }

    if ((fp = fopen(filename, "r")) == NULL) {
        perror("fopen()");
        return err;
    }
    
    prev = fgetc(fp);
    if (prev == EOF) {
        fclose(fp);
        free(result);
        return err;
    }

    size_t bytes = 0;  
    int count = 1;
    
    while ((current = fgetc(fp)) != EOF) {
        if (current == prev) {
            count++;
        } else {
            if (bytes + 10 >= current_length) { 
                current_length *= 2;
                char* temp = (char*)realloc(result, current_length);
                if (temp == NULL) {
                    perror("Memory allocation failed");
                    free(result);  
                    return err;
                }
                result = temp;
            }
            bytes += snprintf(result + bytes, current_length - bytes, "%c%d", prev, count);
            
            prev = current;
            count = 1;
        }
    }
    snprintf(result + bytes, current_length - bytes, "%c%d", prev, count);
    
    fclose(fp);
    return result;
}


char* run_length_decode(char *encoded_string) {
    char *result;
    char *err = "Error";
    size_t current_length = 1024;
    result = (char*)malloc(current_length);
    if (result == NULL) {
        perror("Memory allocation failed");
        return err;
    }

    char c;
    int count;
    size_t bytes = 0;
    for (int i = 0; encoded_string[i] != '\0';) {
        c = encoded_string[i];
        i++;

        char buffer[16];  
        int buf_idx = 0;
        while (encoded_string[i] >= '0' && encoded_string[i] <= '9') {
            buffer[buf_idx++] = encoded_string[i];
            i++;
        }
        buffer[buf_idx] = '\0';  
        count = atoi(buffer);    
        while (count > 0) {
            if (bytes + 1 >= current_length) { 
                current_length *= 2;
                char* temp = (char*)realloc(result, current_length);
                if (temp == NULL) {
                    perror("Memory allocation failed");
                    free(result);  
                    return err;
                }
                result = temp;
            }
            result[bytes++] = c;
            count--;
        }
    }

    result[bytes] = '\0'; 
    return result;
}