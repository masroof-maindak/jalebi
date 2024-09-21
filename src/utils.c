#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

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