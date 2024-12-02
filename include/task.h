#ifndef TASK_H
#define TASK_H

#include <uuid/uuid.h>

#include "bool.h"
#include "utils.h"

#define MAX_TASK_COUNT 16

typedef struct {
	enum REQ_TYPE rt; /* nature of request */
	char *buf;		  /* holds filename */
	char *udir;		  /* user directory */
	uuid_t uuid;	  /* client's unique identifier */
} task_payload;

typedef struct { /* task to be sent to a worker thread */
	int cfd;
	int64_t uid;
	pthread_cond_t *statCond; /* used to wait/signal hashmap storing response */
	task_payload load;
} worker_task;

typedef struct {
	uint8_t count;
	pthread_cond_t *userCond; /* used to wait/signal task completion */
	task_payload tasks[MAX_TASK_COUNT];
} task_list;

bool is_conflicting(const task_payload *newTask, const task_list *uts);
bool append_task(const task_payload *newTask, task_list *uts);
bool remove_task_from_list(uuid_t uuid, task_list *uts);

#endif // TASK_H
