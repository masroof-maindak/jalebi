#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>

struct threadpool {
	size_t size;
	pthread_t *threads;
};

struct threadpool *create_threadpool(size_t n, void *(*fp)(void *));
void delete_threadpool(struct threadpool *tp);

#endif // THREADPOOL_H_H
