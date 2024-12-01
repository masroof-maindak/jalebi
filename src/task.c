#include "../include/task.h"

/**
 * @brief determines if a new task the user is trying to add conflicts with
 * other work of theirs being carried out at the moment
 */
bool is_conflicting(const task_payload *newTask, const task_payload **tasks) {
	/* TODO: more robust task handling involving file names eventually */
	for (int i = 0; tasks[i] != NULL; i++)
		if (tasks[i]->rt == UPLOAD ||
			(tasks[i]->rt == DOWNLOAD && newTask->rt == UPLOAD))
			return false;
	return true;
}
