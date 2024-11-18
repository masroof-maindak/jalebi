#ifndef SERVER_H
#define SERVER_H

#include <semaphore.h>

#include "bool.h"
#include "utils.h"

struct producer_consumer {
	sem_t queued; /* number of queued objects */
	sem_t empty;  /* number of empty positions */
	sem_t mutex;  /* binary semaphore */
};

#define MAXCLIENTS		 8
#define MAX_CLIENT_SPACE 10 * 1024 * 1024

void *client_thread(void *arg);
void *worker_thread(void *arg);

int init(int *sfd, struct sockaddr_in *saddr);
bool ensure_dir_exists(char *dir);
int init_server_socket(struct sockaddr_in *saddr);

int64_t authenticate_and_get_uid(int cfd, char *buf);
__off_t get_used_space(const char *dir);

void init_producer_consumer(struct producer_consumer *pc, int empty);
void destroy_producer_consumer(struct producer_consumer *pc);

int server_wrap_view(int cfd, const char *udir);
int server_wrap_upload(int cfd, const char *buf, const char *udir);
int server_wrap_download(int cfd, const char *buf, const char *udir);

#endif // SERVER_H
