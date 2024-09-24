#ifndef ENCODE_H
#define ENCODE_H

#include <sys/types.h>

int run_length_encode(const char *str, char *ret, size_t bytesToEncode);
int run_length_decode(char *encStr, char *ret, size_t size);

#endif // ENCODE_H
