#include <stdio.h>
#include <stdlib.h>
#include <uuid/uuid.h>

#include "../include/answers.h"
#include "../include/hashmap.h"

int add_to_map(struct hashmap **map, uuid_t key, struct answer *answers) {
	struct hashmap *entry = malloc(sizeof(struct hashmap));
	if (entry == NULL) {
		perror("malloc() in add_to_map()");
		return -1;
	}

	uuid_copy(entry->uuid, key);
	entry->answers = answers;
	HASH_ADD_KEYPTR(hh, *map, entry->uuid, sizeof(uuid_t), entry);
	return 0;
}

int delete_from_map(struct hashmap **map, uuid_t key) {
	struct hashmap *entry;
	HASH_FIND(hh, *map, key, sizeof(uuid_t), entry);
	if (entry == NULL) {
		return -1;
	}

	HASH_DEL(*map, entry);
	free(entry);
	return 0;
}

struct answer *get_answers(struct hashmap *map, uuid_t key) {
	struct hashmap *entry;
	HASH_FIND(hh, map, key, sizeof(uuid_t), entry);
	if (entry == NULL) {
		return NULL;
	}
	return entry->answers;
}

int key_exists(struct hashmap *map, uuid_t key) {
	struct hashmap *entry;
	HASH_FIND(hh, map, key, sizeof(uuid_t), entry);
	return entry ? 1 : 0;
}

int free_map(struct hashmap **map) {
	if (map == NULL || *map == NULL) {
		return -1;
	}
	struct hashmap *current_entry, *tmp;
	HASH_ITER(hh, *map, current_entry, tmp) {
		HASH_DEL(*map, current_entry);
		free(current_entry);
	}
	*map = NULL;
	return 0;
}
