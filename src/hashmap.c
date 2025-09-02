#include "../include/hashmap.h"

bool_t add_new_status(struct task_status_map **map, uuid_t key,
					  enum STATUS st) {
	struct task_status_map *entry = malloc(sizeof(struct task_status_map));
	if (entry == NULL)
		return false;

	uuid_copy(entry->uuid, key);
	entry->st = st;

	HASH_ADD_KEYPTR(hh, *map, entry->uuid, sizeof(uuid_t), entry);
	return true;
}

int delete_from_status_map(struct task_status_map **map, uuid_t key) {
	struct task_status_map *entry;
	HASH_FIND(hh, *map, key, sizeof(uuid_t), entry);

	if (entry == NULL)
		return -1;

	HASH_DEL(*map, entry);
	free(entry);
	return 0;
}

enum STATUS *get_status(struct task_status_map *map, uuid_t key) {
	struct task_status_map *entry;
	HASH_FIND(hh, map, key, sizeof(uuid_t), entry);

	if (entry == NULL)
		return NULL;

	return &entry->st;
}

int free_status_map(struct task_status_map **map) {
	if (map == NULL || *map == NULL)
		return -1;

	struct task_status_map *current_entry, *tmp;
	HASH_ITER(hh, *map, current_entry, tmp) {
		HASH_DEL(*map, current_entry);
		free(current_entry);
	}

	*map = NULL;
	return 0;
}

bool_t add_new_user(struct user_tasks_map **map, int64_t key) {
	struct user_tasks_map *newUser = malloc(sizeof(*newUser));
	if (newUser == NULL)
		return false;

	newUser->tList = malloc(sizeof(*newUser->tList));
	if (newUser->tList == NULL) {
		free(newUser);
		return false;
	}

	newUser->tList->userCond = malloc(sizeof(*newUser->tList->userCond));
	if (newUser->tList->userCond == NULL) {
		free(newUser->tList);
		free(newUser);
		return false;
	}

	newUser->uid		  = key;
	newUser->tList->count = 0;
	pthread_cond_init(newUser->tList->userCond, NULL);

	HASH_ADD_INT(*map, uid, newUser);
	return true;
}

task_list *get_user_tasks(struct user_tasks_map *map, int64_t key) {
	struct user_tasks_map *entry;
	HASH_FIND_INT(map, &key, entry);
	if (entry == NULL)
		return NULL;
	return entry->tList;
}

/**
 * @brief delete a user's entry from the hashmap; the user must exist and have
 * no pending tasks left
 */
void delete_from_user_map(struct user_tasks_map **map, int64_t key) {
	struct user_tasks_map *entry = NULL;
	HASH_FIND_INT(*map, &key, entry);
	pthread_cond_destroy(entry->tList->userCond);
	HASH_DEL(*map, entry);
	free(entry->tList->userCond);
	free(entry->tList);
	free(entry);
}

void free_user_map(struct user_tasks_map **map) {
	if (map == NULL || *map == NULL)
		return;

	struct user_tasks_map *currEntry, *tmp;
	HASH_ITER(hh, *map, currEntry, tmp) {
		pthread_cond_destroy(currEntry->tList->userCond);
		HASH_DEL(*map, currEntry);
		free(currEntry->tList->userCond);
		free(currEntry->tList);
		free(currEntry);
	}

	*map = NULL;
	return;
}
