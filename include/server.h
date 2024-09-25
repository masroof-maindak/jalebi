#ifndef SERVER_H
#define SERVER_H

#include <sys/types.h>

#define MAXCLIENTS		 4
#define MAX_CLIENT_SPACE 10 * 1024 * 1024

int init_server_socket(struct sockaddr_in *saddr);
void ensure_srv_dir_exists();
__off_t get_used_space(const char *dir);

int serv_download(char *filename, size_t bytes, int cfd);
int serv_upload(char *filename, size_t bytes, int cfd);

int serv_wrap_view(int cfd);
int serv_wrap_upload(int cfd, char *buf);
int serv_wrap_download(int cfd, char *buf);

#endif // SERVER_H
