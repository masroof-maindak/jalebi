#ifndef TASK_H
#define TASK_H

#include <uuid/uuid.h>

typedef struct {
	char *buf;	 /* holds filename */
	char *udir;	 /* user directory */
	uuid_t uuid; /* client's unique identifier */
} task_payload;

typedef struct {
	int cfd;
	int64_t uid;
	pthread_cond_t *cond;
	task_payload load;
} worker_task;

#endif // TASK_H
