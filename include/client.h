#ifndef CLIENT_H
#define CLIENT_H

#include "utils.h"

#define SERVER_IP		 "127.0.0.1"
#define USER_INPUT_LIMIT 128
#define RESPONSE_LIMIT	 128

enum REQ_TYPE handle_input(char *userInput);
int init_client_socket(struct sockaddr_in *saddr);
uint8_t valid_user_input(const char *input, enum REQ_TYPE reqType, size_t len);
char *extract_filename_if_exists(const char *fpath, struct stat *fstat);

char select_mode();
char *get_password(char *pw, uint8_t *pwLen);
char *get_username(char *un, uint8_t *unLen);
int user_authentication(int sfd);
int send_auth_info(int sfd, char mode, const char *pw, const char *un,
				   uint8_t unLen, uint8_t pwLen);

int client_wrap_view(int sfd);
int client_wrap_upload(int sfd, const char *buf);
int client_wrap_download(int sfd, const char *buf);

#endif // CLIENT_H
