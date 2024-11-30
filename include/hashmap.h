#ifndef HASHMAP_H
#define HASHMAP_H

#include <uuid/uuid.h>

#include "server.h"
#include "uthash.h"

struct hashmap {
	uuid_t uuid;
	answer *answers;
	UT_hash_handle hh;
};

int add_to_map(struct hashmap **map, uuid_t key, answer *answers);
int delete_from_map(struct hashmap **map, uuid_t key);
answer *get_answers(struct hashmap *map, uuid_t key);
int key_exists(struct hashmap *map, uuid_t key);
int free_map(struct hashmap **map);

#endif
