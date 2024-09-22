#ifndef SERVER_H
#define SERVER_H

#define MAXCLIENTS 4

int init_server_socket(struct sockaddr_in *saddr);
void ensure_srv_dir_exists();

int serv_wrap_view(int cfd);
int serv_wrap_upload(int cfd, char *buf);
int serv_wrap_download(int cfd, char *buf);

#endif // SERVER_H
