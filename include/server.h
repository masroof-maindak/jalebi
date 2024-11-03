#ifndef SERVER_H
#define SERVER_H

#include "bool.h"
#include "utils.h"

#define MAXCLIENTS		 8
#define MAX_CLIENT_SPACE 10 * 1024 * 1024

void *handle_client(void *arg);
int init_server_socket(struct sockaddr_in *saddr);
bool ensure_dir_exists(char *dir);
__off_t get_used_space(const char *dir);
int64_t authenticate_and_get_uid(int cfd, char *buf);
int init(int *sfd, struct sockaddr_in *saddr);

int server_wrap_view(int cfd, char *udir);
int server_wrap_upload(int cfd, const char *buf, char *udir);
int server_wrap_download(int cfd, const char *buf, char *udir);

#endif // SERVER_H
