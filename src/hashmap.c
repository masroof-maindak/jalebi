#include <stdio.h>

#include "../include/hashmap.h"

bool add_to_answer_map(struct status_map **map, uuid_t key, answer *answers) {
	struct status_map *entry = malloc(sizeof(struct status_map));
	if (entry == NULL)
		return false;

	uuid_copy(entry->uuid, key);
	entry->answers = answers;
	HASH_ADD_KEYPTR(hh, *map, entry->uuid, sizeof(uuid_t), entry);
	return true;
}

int delete_from_answer_map(struct status_map **map, uuid_t key) {
	struct status_map *entry;
	HASH_FIND(hh, *map, key, sizeof(uuid_t), entry);

	if (entry == NULL)
		return -1;

	HASH_DEL(*map, entry);
	free(entry);
	return 0;
}

answer *get_answers(struct status_map *map, uuid_t key) {
	struct status_map *entry;
	HASH_FIND(hh, map, key, sizeof(uuid_t), entry);
	if (entry == NULL)
		return NULL;

	return entry->answers;
}

bool key_exists_answer_map(struct status_map *map, uuid_t key) {
	struct status_map *entry;
	HASH_FIND(hh, map, key, sizeof(uuid_t), entry);
	return entry ? true : false;
}

int free_answer_map(struct status_map **map) {
	if (map == NULL || *map == NULL)
		return -1;

	struct status_map *current_entry, *tmp;
	HASH_ITER(hh, *map, current_entry, tmp) {
		HASH_DEL(*map, current_entry);
		free(current_entry);
	}

	*map = NULL;
	return 0;
}

bool add_new_user(struct user_map **map, worker_task *info) {
	struct user_map *entry = malloc(sizeof(*entry));
	if (entry == NULL)
		return false;

	entry->ut.count = 0;
	pthread_cond_init(&entry->ut.condVar);

	HASH_ADD_INT(*map, wt->uid, entry);
	return true;
}

user_tasks *get_user_tasks(struct user_map *map, int64_t key) {
	struct user_map *entry;
	HASH_FIND_INT(map, &key, entry);
	if (entry == NULL)
		return NULL;
	return &entry->ut;
}

int delete_from_user_map(struct user_map **map, int64_t key) {
	struct user_map *entry;
	HASH_FIND_INT(*map, &key, entry);

	if (entry == NULL)
		return -1;

	pthread_cond_destroy(&entry->ut.condVar);

	if (entry->ut.tasks != NULL) {
		free(entry->ut.tasks);
	}

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
		pthread_cond_destroy(&current_entry->ut.condVar);

		if (current_entry->ut.tasks != NULL) {
			free(current_entry->ut.tasks);
		}

		HASH_DEL(*map, current_entry);
		free(current_entry);
	}

	*map = NULL;
	return 0;
}

int update_value_in_user_map(struct user_map **map, int64_t key,
							 user_tasks new_ut) {
	if (map == NULL || *map == NULL)
		return -1;

	struct user_map *entry;
	HASH_FIND_INT(*map, &key, entry);
	if (entry == NULL)
		return -2;

	pthread_cond_destroy(&entry->ut.condVar);
	if (entry->ut.tasks != NULL) {
		free(entry->ut.tasks);
	}
	pthread_cond_init(&new_ut.condVar, NULL);
	entry->ut = new_ut;

	return 0;
}
