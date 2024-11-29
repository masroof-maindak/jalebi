#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>

#include "queue.h"

struct threadpool {
	size_t size;
	pthread_t *threads;
    struct queue *q;
	void *(*fp)(void*);
	pthread_mutex_t lock; 
	pthread_cond_t notify; 
};

struct threadpool *create_threadpool(size_t n, void *(*fp)(void *));
void* internal_f(void* arg);
void delete_threadpool(struct threadpool *tp);

#endif // THREADPOOL_H_H
