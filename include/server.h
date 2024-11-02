#ifndef SERVER_H
#define SERVER_H

#include "utils.h"

#define MAXCLIENTS		 4
#define MAX_CLIENT_SPACE 10 * 1024 * 1024

struct USER_TASK {
	int64_t uid;
	enum REQ_TYPE rt;
	char *msg;
};

extern const struct USER_TASK DEFAULT_TASK;

void *handle_client(void *arg);
int init_server_socket(struct sockaddr_in *saddr);
int ensure_dir_exists(char *dir);
__off_t get_used_space(const char *dir);
int64_t authenticate_and_get_uid(int cfd, char *buf);

int server_wrap_view(int cfd, char *udir);
int server_wrap_upload(int cfd, const char *buf, char *udir);
int server_wrap_download(int cfd, const char *buf, char *udir);

#endif // SERVER_H
