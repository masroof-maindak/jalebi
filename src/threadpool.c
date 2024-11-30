#include <pthread.h>
#include <stdio.h>

#include "../include/threadpool.h"

/**
 * @brief create `n` threads, w/ entrypoint `fp`, and tasks of size `elemSize`
 */
struct threadpool *create_threadpool(size_t n, size_t elemSize,
									 void *(*fp)(void *)) {
	if (n == 0)
		return NULL;

	struct threadpool *tp = malloc(sizeof(*tp));
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

	tp->q = create_queue(elemSize);
	if (tp->q == NULL) {
		perror("malloc() in create_tp() - queue");
		free(tp->threads);
		free(tp);
		return NULL;
	}

	tp->fp = fp;

	pthread_mutex_init(&tp->lock, NULL);
	pthread_cond_init(&tp->notify, NULL);

	/* CHECK: what to do if failure occurs ? */
	for (size_t i = 0; i < n; i++) {
		if (pthread_create(&tp->threads[i], NULL, internal_f, tp) != 0) {
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

/**
 * @brief if work is available, pass it to the user-designated entrypoint; the
 * user should free the argument after they are done using it */
void *internal_f(void *arg) {
	void *task;
	struct threadpool *tp = (struct threadpool *)arg;
	for (;;) {
		pthread_mutex_lock(&tp->lock);
		while (tp->q->size == 0)
			pthread_cond_wait(&tp->notify, &tp->lock);
		task = copy_top(tp->q);

		if (task == NULL)
			/* CHECK: how to clean up? */
			continue;

		dequeue(tp->q);
		pthread_mutex_unlock(&tp->lock);

		tp->fp(task);
	}

	return NULL;
}

/**
 * @brief add a task to the threadpool
 */
void add_task(struct threadpool *tp, void *data) {
	pthread_mutex_lock(&tp->lock);
	enqueue(tp->q, data);
	pthread_mutex_unlock(&tp->lock);
	pthread_cond_signal(&tp->notify);
}

void delete_threadpool(struct threadpool *tp) {
	if (tp == NULL)
		return;
	/* TODO: destroy mutex & cond_var */
	delete_queue(tp->q);
	free(tp->threads);
	free(tp);
}
