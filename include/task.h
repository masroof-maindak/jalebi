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

typedef struct {
	int cfd;
	int64_t uid;
	pthread_cond_t *cond;
	task_payload load;
} worker_task;

typedef struct {
	uint8_t count;
	pthread_cond_t condVar;
	task_payload tasks[MAX_TASK_COUNT];
} user_tasks;

bool is_conflicting(const task_payload *newTask,
					task_payload userTasks[MAX_TASK_COUNT]);
bool append_task(const task_payload *newTask, user_tasks *uts);

#endif // TASK_H
