#ifndef HASHMAP_H
#define HASHMAP_H

#include <pthread.h>

#include "answer.h"
#include "bool.h"
#include "task.h"
#include "uthash.h"
#include "utils.h"

/* uuid_t -> answer */
/* To let a client uniquely identify the answer generated for it */
struct answer_map {
	uuid_t uuid;
	answer *answers;
	UT_hash_handle hh;
};

bool add_to_answer_map(struct answer_map **map, uuid_t key, answer *answers);
answer *get_answers(struct answer_map *map, uuid_t key);
int delete_from_answer_map(struct answer_map **map, uuid_t key);
bool key_exists_answer_map(struct answer_map *map, uuid_t key);
int free_answer_map(struct answer_map **map);

/* uid -> rt */
/* To prevent conflict when completing tasks for a user */
struct user_info {
	enum REQ_TYPE rt;
	pthread_cond_t condVar;
};

struct user_map {
	int64_t uid;
	struct user_info ui;
	UT_hash_handle hh;
};

bool add_to_user_map(struct user_map **map, int64_t key, struct user_info ui);
struct user_info *get_userinfo(struct user_map *map, int64_t key);
int delete_from_user_map(struct user_map **map, int64_t key);
bool key_exists_user_map(struct user_map *map, int64_t key);
int free_user_map(struct user_map **map);
int update_value_in_user_map(struct user_map **map, int64_t key,
							 struct user_info new_ui);
#endif
