#ifndef CLIENT_H
#define CLIENT_H

#include <stdint.h>
#include <sys/types.h>

#define SERVER_IP		 "127.0.0.1"
#define USER_INPUT_LIMIT 128
#define RESPONSE_LIMIT	 128

int handle_input(char *userInput);
int init_client_socket(struct sockaddr_in *saddr);
uint8_t valid_user_input(const char *input, int reqType, size_t len);
char *extract_filename_if_exists(const char *filepath, struct stat *fstat);

int client_wrap_view(int sfd);
int client_wrap_upload(int sfd, char *buf);
int client_wrap_download(int sfd, char *buf);

#endif // CLIENT_H
