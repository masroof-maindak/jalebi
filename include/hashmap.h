#ifndef HASHMAP_H
#define HASHMAP_H

#include "answer.h"
#include "uthash.h"
#include <uuid/uuid.h>

struct answermap {
	uuid_t uuid;
	answer *answers;
	UT_hash_handle hh;
};


//for answermap
int add_to_answermap(struct answermap **map, uuid_t key, answer *answers);
answer *get_answers(struct answermap *map, uuid_t key);
int delete_from_answermap(struct answermap **map, uuid_t key);
int key_exists_answermap(struct answermap*map, uuid_t key);
int free_answermap(struct answermap **map);




#endif
