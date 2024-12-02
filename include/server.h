#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>

#include "bool.h"
#include "utils.h"

#define MAXCLIENTS		 8
#define MAX_CLIENT_SPACE 10 * 1024 * 1024

#define PRINT_SEM_VALUE(sem, name, clr)                                        \
	do {                                                                       \
		int val;                                                               \
		if (sem_getvalue(&(sem), &val) == 0)                                   \
			printf("%s%s: %d\n", (clr), (name), val);                          \
		else                                                                   \
			perror("sem_getvalue() in PRINT_SEM_VALUE");                       \
	} while (0)

void client_thread(void *arg);
void worker_thread(void *arg);

int init(int *sfd, struct sockaddr_in *saddr);
bool ensure_dir_exists(char *dir);
int init_server_socket(struct sockaddr_in *saddr);

int64_t authenticate_and_get_uid(int cfd, char *buf);
__off_t get_used_space(const char *dir);

enum STATUS server_wrap_view(int cfd, const char *udir);
enum STATUS server_wrap_upload(int cfd, const char *buf, const char *udir);
enum STATUS server_wrap_download(int cfd, const char *buf, const char *udir);

#endif // SERVER_H
