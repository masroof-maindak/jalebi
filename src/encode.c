#include "../include/encode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* rle(char* inp) {
	return inp;
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

