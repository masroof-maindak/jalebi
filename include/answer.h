#ifndef ANSWER_H
#define ANSWER_H

#include <uuid/uuid.h>

typedef struct {
	int status;
	uuid_t uuid;
} answer;

#endif // ANSWER_H
