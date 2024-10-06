#ifndef SERVER_H
#define SERVER_H

#include <sys/types.h>

#define MAXCLIENTS		 4
#define MAX_CLIENT_SPACE 10 * 1024 * 1024

void *handle_client(void *arg);
int init_server_socket(struct sockaddr_in *saddr);
int ensure_srv_dir_exists();
__off_t get_used_space(const char *dir);

int serv_wrap_view(int cfd);
int serv_wrap_upload(int cfd, char *buf);
int serv_wrap_download(int cfd, char *buf);

#endif // SERVER_H
