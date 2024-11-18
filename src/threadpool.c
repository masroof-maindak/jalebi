#include <stdio.h>
#include <stdlib.h>

#include "../include/threadpool.h"

/**
 * @brief create `n` threads, running `fp` function
 */
struct threadpool *create_threadpool(size_t n, void *(*fp)(void *)) {
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

	/* CHECK: what to do if failure occurs ? */
	for (size_t i = 0; i < n; i++) {
		if (pthread_create(&tp->threads[i], NULL, fp, NULL) != 0) {
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

void delete_threadpool(struct threadpool *tp) {
	if (tp == NULL)
		return;
	free(tp->threads);
	free(tp);
}
