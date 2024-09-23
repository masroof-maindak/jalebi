#include <stdio.h>
#include <stdlib.h>

#include "../include/encode.h"
#include "../include/utils.h"

/* TODO: refactor and return a (char *) rather than a void */
/* NOTE: Should we really be stopping at a NULL-terminator */
void run_length_encode(const char *str, char *ret, size_t size) {
	char current, prev;
	size_t bytes = 0, idx = 1;
	int count = 1;

	if ((prev = str[0]) == '\0') {
		ret = NULL;
		return;
	}

	while ((current = str[idx++]) != '\0') {
		if (current == prev) {
			count++;
		} else {

			if ((ret = double_if_of(ret, idx, 10, &size)) == NULL)
				return;

			bytes += snprintf(ret + bytes, size - bytes, "%c%d", prev, count);

			prev  = current;
			count = 1;
		}
	}

	if ((ret = double_if_of(ret, idx, 10, &size)) == NULL)
		return;

	snprintf(ret + bytes, size - bytes, "%c%d", prev, count);
	return;
}

void run_length_decode(char *encStr, char *ret, size_t size) {
	char c;
	int count;
	size_t bytes = 0;

	for (int i = 0; encStr[i] != '\0';) {
		c = encStr[i];
		i++;

		char buffer[16];
		int idx = 0;

		while (encStr[i] >= '0' && encStr[i] <= '9') {
			buffer[idx++] = encStr[i];
			i++;
		}

		buffer[idx] = '\0';
		count		= atoi(buffer);

		while (count > 0) {

			if ((ret = double_if_of(ret, bytes, 1, &size)) == NULL)
				return;

			ret[bytes++] = c;
			count--;
		}
	}

	ret[bytes] = '\0';
	return;
}
