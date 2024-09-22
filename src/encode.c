#include "../include/encode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



void run_length_encode(const char *str, char *result, size_t initial_size) {
    char current, prev;
    size_t current_length = initial_size;
    size_t bytes          = 0;
    int count             = 1;
    int idx               = 1;
    if ((prev = str[0]) == '\0') {
        result = NULL;
        return;
    }

    while ((current = str[idx++]) != '\0') {
        if (current == prev) {
            count++;
        } else {
            if (bytes + 10 >= current_length) {
                result = double_if_of(result, bytes + 10, 0, &current_length);
                if (result == NULL) {
                    return;
                }
            }
            bytes += snprintf(result + bytes, current_length - bytes, "%c%d",
                              prev, count);

            prev  = current;
            count = 1;
        }
    }
    result = double_if_of(result, bytes, 10, &current_length);
    if (result == NULL) {
        return;
    }
    snprintf(result + bytes, current_length - bytes, "%c%d", prev, count);
    return;
}

void run_length_decode(char *encoded_string, char *result, size_t initial_size) {
    size_t current_length = initial_size;
    char c;
    int count;
    size_t bytes = 0;
    for (int i = 0; encoded_string[i] != '\0';) {
        c = encoded_string[i];
        i++;

        char buffer[16];
        int idx = 0;
        while (encoded_string[i] >= '0' && encoded_string[i] <= '9') {
            buffer[idx++] = encoded_string[i];
            i++;
        }
        buffer[idx] = '\0';
        count       = atoi(buffer);
        while (count > 0) {
            if (bytes + 1 >= current_length) {
                result = double_if_of(result, bytes + 1, 0, &current_length);
                if (result == NULL) {
                    return;
                }
            }
            result[bytes++] = c;
            count--;
        }
    }

    result[bytes] = '\0';
    return;
}