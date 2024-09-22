#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>

#define SERVER_PORT		  9173
#define BUFSIZE			  1024
#define HOSTDIR			  "srv"
#define FAILURE_MSG		  "$FAILURE$\n"
#define SUCCESS_MSG		  "$SUCCESS$\n"
#define ULOAD_FAILURE_MSG "$FAILURE$LOW_SPACE$\n"
#define VIEW_FAILURE_MSG  "$FAILURE$NO_CLIENT_DATA$\n"
#define DLOAD_FAILURE_MSG "$FAILURE$FILE_NOT_FOUND$\n"

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

char *copy_string(const char *str);
int identify_request(char *buf);
ssize_t view(char *buf, int size);

int serv_upload(char *filename, size_t bytes, int cfd);
int serv_download(char *filename, size_t bytes, int cfd);

#endif // UTILS_H
