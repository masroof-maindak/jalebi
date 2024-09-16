#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>

#define SERVER_PORT 9173
#define BUFSIZE		1024

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

char *copy_string(const char *str);
void download(char *filename, size_t bytes, int sockfd);

#endif // UTILS_H
