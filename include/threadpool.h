#ifndef BOOL_H
#define BOOL_H

#include <pthread.h>

struct THREADPOOL {
	pthread_t *tp;
};

struct THREADPOOL *create_tp(int n);
void *delete_tp(struct THREADPOOL *tp);

#endif // BOOL_H
