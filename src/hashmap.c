#include <stdio.h>

#include "../include/hashmap.h"

bool add_to_answer_map(struct answer_map **map, uuid_t key, answer *answers) {
	struct answer_map *entry = malloc(sizeof(struct answer_map));
	if (entry == NULL)
		return false;

	uuid_copy(entry->uuid, key);
	entry->answers = answers;
	HASH_ADD_KEYPTR(hh, *map, entry->uuid, sizeof(uuid_t), entry);
	return true;
}

int delete_from_answer_map(struct answer_map **map, uuid_t key) {
	struct answer_map *entry;
	HASH_FIND(hh, *map, key, sizeof(uuid_t), entry);

	if (entry == NULL)
		return -1;

	HASH_DEL(*map, entry);
	free(entry);
	return 0;
}

answer *get_answers(struct answer_map *map, uuid_t key) {
	struct answer_map *entry;
	HASH_FIND(hh, map, key, sizeof(uuid_t), entry);
	if (entry == NULL)
		return NULL;

	return entry->answers;
}

bool key_exists_answer_map(struct answer_map *map, uuid_t key) {
	struct answer_map *entry;
	HASH_FIND(hh, map, key, sizeof(uuid_t), entry);
	return entry ? true : false;
}

int free_answer_map(struct answer_map **map) {
	if (map == NULL || *map == NULL)
		return -1;

	struct answer_map *current_entry, *tmp;
	HASH_ITER(hh, *map, current_entry, tmp) {
		HASH_DEL(*map, current_entry);
		free(current_entry);
	}

	*map = NULL;
	return 0;
}

int free_map(struct answer_map **map) {
	if (map == NULL || *map == NULL)
		return -1;

	struct answer_map *current_entry, *tmp;
	HASH_ITER(hh, *map, current_entry, tmp) {
		HASH_DEL(*map, current_entry);
		free(current_entry);
	}

	*map = NULL;
	return 0;
}
