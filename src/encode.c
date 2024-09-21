#include "../include/encode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *rle(char *inp) { return inp; }

char *run_length_encode(const char *str) {
	char current, prev;
	size_t current_length = 1024;
	char *result		  = malloc(current_length);
	if (result == NULL) {
		perror("malloc()");
		return NULL;
	}

	if ((prev = str[0]) == '\0') {
		free(result);
		return NULL;
	}

	size_t bytes = 0;
	int count	 = 1;
	int idx		 = 1;

	while ((current = str[idx++]) != '\0') {
		if (current == prev) {
			count++;
		} else {
			if (bytes + 10 >= current_length) {
				current_length *= 2;
				char *temp = realloc(result, current_length);
				if (temp == NULL) {
					perror("realloc()");
					free(result);
					return NULL;
				}
				result = temp;
			}
			bytes += snprintf(result + bytes, current_length - bytes, "%c%d",
							  prev, count);

			prev  = current;
			count = 1;
		}
	}
	snprintf(result + bytes, current_length - bytes, "%c%d", prev, count);
	return result;
}

char *run_length_decode(char *encoded_string) {
	size_t current_length = 1024;
	char *result		  = malloc(current_length);
	if (result == NULL) {
		perror("malloc()");
		return NULL;
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
		count			= atoi(buffer);
		while (count > 0) {
			if (bytes + 1 >= current_length) {
				current_length *= 2;
				char *temp = (char *)realloc(result, current_length);
				if (temp == NULL) {
					perror("Memory allocation failed");
					free(result);
					return NULL;
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
