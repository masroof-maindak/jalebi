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

bool add_to_user_map(struct user_map **map, int64_t key, struct user_info ui) {
	struct user_map *entry = malloc(sizeof(struct user_map));
	if (entry == NULL)
		return false;

	entry->uid = key;
	entry->ui  = ui;
	HASH_ADD_INT(*map, uid, entry);
	return true;
}

struct user_info *get_userinfo(struct user_map *map, int64_t key) {
	struct user_map *entry;
	HASH_FIND_INT(map, &key, entry);
	if (entry == NULL)
		return NULL;

	return &entry->ui;
}

int delete_from_user_map(struct user_map **map, int64_t key) {
	struct user_map *entry;
	HASH_FIND_INT(*map, &key, entry);

	if (entry == NULL)
		return -1;

	pthread_cond_destroy(&entry->ui.condVar);
	HASH_DEL(*map, entry);
	free(entry);
	return 0;
}

bool key_exists_user_map(struct user_map *map, int64_t key) {
	struct user_map *entry;
	HASH_FIND_INT(map, &key, entry);
	return entry ? true : false;
}

int free_user_map(struct user_map **map) {
	if (map == NULL || *map == NULL)
		return -1;

	struct user_map *current_entry, *tmp;
	HASH_ITER(hh, *map, current_entry, tmp) {
		pthread_cond_destroy(&current_entry->ui.condVar);
		HASH_DEL(*map, current_entry);
		free(current_entry);
	}

	*map = NULL;
	return 0;
}

int update_value_in_user_map(struct user_map **map, int64_t key,
							 struct user_info new_ui) {
	if (map == NULL || *map == NULL)
		return -1;

	struct user_map *entry;
	HASH_FIND_INT(*map, &key, entry);
	if (entry == NULL)
		return -2;

	entry->ui = new_ui;
	return 0;
}
