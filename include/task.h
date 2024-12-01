#ifndef TASK_H
#define TASK_H

#include <uuid/uuid.h>

#include "bool.h"
#include "utils.h"

typedef struct {
	enum REQ_TYPE rt; /* nature of request */
	char *buf;		  /* holds filename */
	char *udir;		  /* user directory */
	uuid_t uuid;	  /* client's unique identifier */
} task_payload;

typedef struct {
	int cfd;
	int64_t uid;
	pthread_cond_t *cond;
	task_payload load;
} worker_task;

bool is_conflicting(const task_payload *newTask, const task_payload **tasks);

#endif // TASK_H
