#ifndef UTILS_H
#define UTILS_H

#include <linux/limits.h>
#include <stdint.h>
#include <sys/types.h>

#define PW_MAX_LEN		  31
#define PW_MIN_LEN		  4
#define SERVER_PORT		  9173
#define BUFSIZE			  1024
#define HOSTDIR			  "srv"
#define FAILURE_MSG		  "$FAILURE$\n"
#define SUCCESS_MSG		  "$SUCCESS$\n"
#define ULOAD_FAILURE_MSG "$FAILURE$LOW_SPACE$\n"
#define DLOAD_FAIL_MSG	  "$FAILURE$FILE_NOT_FOUND$\n"

#define RESET	"\x1b[0m"
#define RED		"\x1b[31m"
#define GREEN	"\x1b[32m"
#define YELLOW	"\x1b[33m"
#define BLUE	"\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN	"\x1b[36m"
#define WHITE	"\x1b[37m"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

enum REQ_TYPE { VIEW = 1, DOWNLOAD, UPLOAD, INVALID };

struct client_request
{
    int id;
    REQUEST reqType;
    char* msg;
};


char *copy_string(const char *str);
enum REQ_TYPE identify_req_type(const char *buf);
int recv_success(int sockfd, const char *err);
ssize_t view(char *buf, size_t size, const char *dir);
uint8_t get_num_digits(__off_t n);

char *double_if_Of(char *buf, size_t idx, size_t addition, size_t *size);

int download_file(const char *fname, size_t bytes, int sfd);
int upload_file(const char *fname, size_t bytes, int sfd);

#endif // UTILS_H
