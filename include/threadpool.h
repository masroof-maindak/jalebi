#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <stdint.h>

/* TODO: power of 2 */
struct tpool {
	uint16_t size;
	pthread_t *threads;
};

struct tpool *create_threadpool(uint16_t n, void *(*fp)(void *));
void delete_threadpool(struct tpool *tp);

#endif // THREADPOOL_H_H
