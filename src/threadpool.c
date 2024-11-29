#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/threadpool.h"
#include "../include/queue.h"

/**
 * @brief create `n` threads, running `fp` function
 */
struct threadpool *create_threadpool(size_t n, size_t structSize,void *(*fp)(void *)) {
	if (n == 0)
		return NULL;

	struct threadpool *tp = malloc(sizeof(struct threadpool));
	if (tp == NULL) {
		perror("malloc() in create_tp() - tp");
		return NULL;
	}

	tp->size	= n;
	tp->threads = malloc(sizeof(tp->threads) * n);
	if (tp->threads == NULL) {
		perror("malloc() in create_tp() - threads");
		free(tp);
		return NULL;
	}
    tp->q = create_queue(structSize);
	if (tp->q == NULL) {
		perror("malloc() in create_tp() - queue");
		free(tp);
		return NULL;
	}
	tp->fp = fp;
	pthread_mutex_init(&tp->lock, NULL);
	pthread_cond_init(&tp->notify, NULL);
	/* CHECK: what to do if failure occurs ? */
	for (size_t i = 0; i < n; i++) {
		if (pthread_create(&tp->threads[i], NULL, internal_f, (void *)tp) !=
			0) {
			perror("pthread_create() in create_tp()");
			return NULL;
		}

		if (pthread_detach(tp->threads[i]) != 0) {
			perror("pthread_detach() in create_tp()");
			return NULL;
		}
	}

	return tp;
}

void *internal_f(void *arg) {
	void *task;
	if (!arg) {
		perror("arg null in internal_f()");
		return NULL;
	}
	struct threadpool *tp = (struct threadpool *)arg;
	for (;;) {
		pthread_mutex_lock(&tp->lock);
		while (tp->q->size == 0) {
			pthread_cond_wait(&tp->notify, &tp->lock);
		}
		task = peek_top(tp->q);
        dequeue(tp->q);
		pthread_mutex_unlock(&tp->lock);
		tp->fp(task);
	}
	return NULL;
}


void add_task(struct threadpool* tp, void* data){
	if(!tp){
		perror("tp is null in add_task()");
		return;

	}
	if(!data){
		perror("data is null in add_task()");
		return;
	}
    pthread_mutex_lock(&tp->lock);
	enqueue(tp->q, data);
	pthread_mutex_unlock(&tp->lock);
	pthread_cond_signal(&tp->notify);
}



void delete_threadpool(struct threadpool *tp) {
	if (tp == NULL)
		return;
	free(tp->threads);
	free(tp->q);
	free(tp);
}

