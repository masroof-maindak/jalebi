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

void *internal_f(void *arg);
struct threadpool *create_threadpool(size_t n, void *(*fp)(void *));
void delete_threadpool(struct threadpool *tp);

#endif // THREADPOOL_H_H
