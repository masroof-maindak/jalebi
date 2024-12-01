#include "../include/task.h"

/**
 * @brief determines if a new task the user is trying to add conflicts with
 * other work of theirs being carried out at the moment
 */
bool is_conflicting(const task_payload *newTask,
					task_payload userTasks[MAX_TASK_COUNT]) {
	/* TODO: more robust task handling involving file names eventually */
	for (int i = 0; tasks[i] != NULL; i++)
		if (tasks[i]->rt == UPLOAD ||
			(tasks[i]->rt == DOWNLOAD && newTask->rt == UPLOAD))
			return false;
	return true;
}

bool append_task(const task_payload *newTask, user_tasks *uts) {
	if (uts->count > MAX_TASK_COUNT)
		return false;
	uts->tasks[uts->count++] = *newTask;
	return true;
}
