#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>

void run_length_encode(const char *str, char *result, size_t initial_size);
void run_length_decode(char *encoded_string, char *result, size_t initial_size) ;

#endif // UTILS_H
