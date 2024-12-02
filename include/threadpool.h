#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>

#include "queue.h"

struct tpool {
	size_t size;
	struct queue *q;
	pthread_t *threads;
	void (*fp)(void *);
	pthread_mutex_t lock;
	pthread_cond_t notify;
};

struct tpool *create_tpool(size_t n, size_t elemSize, void (*fp)(void *));
void delete_tpool(struct tpool *tp);

void *internal_f(void *arg);
void add_task(struct tpool *tp, void *data);

#endif // THREADPOOL_H
