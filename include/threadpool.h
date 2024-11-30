#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>

#include "queue.h"

struct threadpool {
	size_t size;
	struct queue *q;
	pthread_t *threads;
	void *(*fp)(void *);
	pthread_mutex_t lock;
	pthread_cond_t notify;
};

struct threadpool *create_threadpool(size_t n, size_t elemSize,
									 void *(*fp)(void *));
void delete_threadpool(struct threadpool *tp);

void *internal_f(void *arg);
void add_task(struct threadpool *tp, void *data);

#endif // THREADPOOL_H_H
