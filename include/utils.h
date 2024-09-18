#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>

#define SERVER_PORT				9173
#define BUFSIZE					1024
#define HOSTDIR					"srv"
#define INVALID_REQUEST_MESSAGE "Error: Invalid request!"

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

void ensure_srv_dir_exists();
char *copy_string(const char *str);
int download(char *filename, size_t bytes, int sockfd);
int upload(char *filename, size_t bytes, int sockfd);
int view(int cfd);

#endif // UTILS_H
