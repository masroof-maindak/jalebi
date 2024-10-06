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

#define COL_RESET	"\x1b[0m"
#define COL_RED		"\x1b[31m"
#define COL_GREEN	"\x1b[32m"
#define COL_YELLOW	"\x1b[33m"
#define COL_BLUE	"\x1b[34m"
#define COL_MAGENTA "\x1b[35m"
#define COL_CYAN	"\x1b[36m"
#define COL_WHITE	"\x1b[37m"

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

char *copy_string(const char *str);
int identify_request(char *buf);
int recv_success(int sfd, char *errMsg);
ssize_t view(char *buf, size_t size);

char *double_if_of(char *buf, size_t idx, size_t addition, size_t *size);

int download(char *filename, size_t bytes, int sfd);
/* TODO: take function pointer to encoder function */
int upload(char *filename, size_t bytes, int sfd);

#endif // UTILS_H
