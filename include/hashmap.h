#ifndef HASHMAP_H
#define HASHMAP_H

#include <pthread.h>

#include "bool.h"
#include "task.h"
#include "uthash.h"
#include "utils.h"

/* uuid_t -> status */
/* To let a client uniquely identify the status of it's task */
struct task_status_map {
	uuid_t uuid;
	enum STATUS st;
	UT_hash_handle hh;
};

bool_t add_new_status(struct task_status_map **map, uuid_t key, enum STATUS st);
enum STATUS *get_status(struct task_status_map *map, uuid_t key);
int delete_from_status_map(struct task_status_map **map, uuid_t key);
int free_status_map(struct task_status_map **map);

/* uid -> { cond_t, count, tasks[16] } */
/* To prevent conflict when completing tasks for a user */
struct user_tasks_map {
	int64_t uid;
	task_list *tList;
	UT_hash_handle hh;
};

bool_t add_new_user(struct user_tasks_map **map, int64_t key);
task_list *get_user_tasks(struct user_tasks_map *map, int64_t key);
void delete_from_user_map(struct user_tasks_map **map, int64_t key);
void free_user_map(struct user_tasks_map **map);

#endif
