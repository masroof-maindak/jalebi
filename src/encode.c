#include <stdio.h>
#include <stdlib.h>

#include "../include/encode.h"
#include "../include/utils.h"

/**
 * @param bytesToEncode len of input str
 * @return < 0 if error else size of ret
 */
int run_length_encode(const char *str, char *ret, size_t len) {
	char current, prev;
	size_t bytes = 0, idx = 1;
	int count = 1;

	if (ret == NULL)
		return -1;

	if (len <= 1)
		return -2;

	prev = str[0];
	while (idx <= len) {
		current = str[idx++];
		if (current == prev) {
			count++;
		} else {

			if ((ret = double_if_of(ret, idx, 10, &len)) == NULL)
				return -3;

			bytes += snprintf(ret + bytes, len - bytes, "%c%d", prev, count);

			prev  = current;
			count = 1;
		}
	}

	if ((ret = double_if_of(ret, idx, 10, &len)) == NULL)
		return -3;

	snprintf(ret + bytes, len - bytes, "%c%d", prev, count);
	return 0;
}

int run_length_decode(char *encStr, char *ret, size_t size) {
	char c;
	int count;
	size_t bytes = 0, i = 0;

	if (size <= 0)
		return -1;

	for (i = 0; i < size;) {
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
				return -2;

			ret[bytes++] = c;
			count--;
		}
	}

	ret[bytes] = '\0';
	return 0;
}
