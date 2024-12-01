#include "../include/task.h"

/**
 * @brief determines if a new task the user is trying to add conflicts with
 * other work of theirs being carried out at the moment
 */
bool is_conflicting(const task_payload *newTask, const user_tasks *uts) {
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
bool append_task(const task_payload *newTask, user_tasks *uts) {
	if (uts->count > MAX_TASK_COUNT)
		return false;
	uts->tasks[uts->count++] = *newTask;
	return true;
}
