#ifndef HASHMAP_H
#define HASHMAP_H

#include <pthread.h>

#include "bool.h"
#include "task.h"
#include "uthash.h"
#include "utils.h"

enum STATUS { SUCCESS = 0, FAILURE = 1 };

/* uuid_t -> status */
/* To let a client uniquely identify the status of it's task */
struct status_map {
	uuid_t uuid;
	enum STATUS st;
	UT_hash_handle hh;
};

bool add_to_status_map(struct status_map **map, uuid_t key, enum STATUS status);
enum STATUS *get_status(struct status_map *map, uuid_t key);
int delete_from_status_map(struct status_map **map, uuid_t key);
int free_status_map(struct status_map **map);

/* uid -> { cond_t, count, tasks } */
/* To prevent conflict when completing tasks for a user */

struct user_tasks_map {
	int64_t uid;
	user_tasks *ut;
	UT_hash_handle hh;
};

bool add_new_user(struct user_tasks_map **map, int64_t key);
user_tasks *get_user_tasks(struct user_tasks_map *map, int64_t key);
void delete_from_user_map(struct user_tasks_map **map, int64_t key);
void free_user_map(struct user_tasks_map **map);

#endif
