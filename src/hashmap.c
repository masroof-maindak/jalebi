#include <stdio.h>
#include <stdlib.h>
#include <uuid/uuid.h>

#include "../include/hashmap.h"

int add_to_answermap(struct answermap **map, uuid_t key, answer *answers) {
	struct answermap *entry = malloc(sizeof(struct answermap));
	if (entry == NULL) {
		perror("malloc() in add_to_answermap()");
		return -1;
	}

	uuid_copy(entry->uuid, key);
	entry->answers = answers;
	HASH_ADD_KEYPTR(hh, *map, entry->uuid, sizeof(uuid_t), entry);
	return 0;
}

int delete_from_answermap(struct answermap **map, uuid_t key) {
	struct answermap *entry;
	HASH_FIND(hh, *map, key, sizeof(uuid_t), entry);
	if (entry == NULL) {
		return -1;
	}
	HASH_DEL(*map, entry);
	free(entry);
	return 0;
}

answer *get_answers(struct answermap *map, uuid_t key) {
	struct answermap *entry;
	HASH_FIND(hh, map, key, sizeof(uuid_t), entry);
	if (entry == NULL)
		return NULL;
	return entry->answers;
}

int key_exists_answermap(struct answermap *map, uuid_t key) {
	struct answermap *entry;
	HASH_FIND(hh, map, key, sizeof(uuid_t), entry);
	return entry ? 1 : 0;
}

int free_answermap(struct answermap **map) {
	if (map == NULL || *map == NULL) {
		return -1;
	}
	struct answermap *current_entry, *tmp;
	HASH_ITER(hh, *map, current_entry, tmp) {
		HASH_DEL(*map, current_entry);
		free(current_entry);
	}
	*map = NULL;
	return 0;
}

int free_map(struct answermap **map) {
	if (map == NULL || *map == NULL)
		return -1;

	struct answermap *current_entry, *tmp;
	HASH_ITER(hh, *map, current_entry, tmp) {
		HASH_DEL(*map, current_entry);
		free(current_entry);
	}

	*map = NULL;
	return 0;
}
