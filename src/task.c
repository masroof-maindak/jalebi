#include "../include/task.h"
#include <pthread.h>

/**
 * @brief determines if a new task the user is trying to add conflicts with
 * other work of theirs being carried out at the moment
 */
bool_t is_conflicting(const task_payload *newTask, const task_list *uts) {
	/* TODO: more robust task handling involving file names eventually */
	for (int i = 0; i < uts->count; i++)
		if (uts->tasks[i].rt == UPLOAD ||
			(uts->tasks[i].rt == DOWNLOAD && newTask->rt == UPLOAD))
			return false;
	return true;
}

/**
 * @brief add a task to a user's active task list they have enough room
 */
bool_t append_task(const task_payload *newTask, task_list *uts) {
	if (uts->count > MAX_TASK_COUNT)
		return false;
	uts->tasks[uts->count++] = *newTask;
	return true;
}

/**
 * @brief remove a certain task from a user's list after it has been completed.
 * It is assumed that input is valid
 */
bool_t remove_task_from_list(uuid_t uuid, task_list *tl) {
	int i = 0;

	/* get to the right value */
	for (; i < tl->count; i++)
		if (uuid_compare(uuid, tl->tasks[i].uuid) == 0)
			break;

	if (i == tl->count)
		return false;

	/* shift left every value that comes after */
	for (; i < tl->count - 1; i++)
		tl->tasks[i] = tl->tasks[i + 1];

	return true;
}
