#include <stdio.h>
#include <stdlib.h>

#include "../include/threadpool.h"

/**
 * @brief create `n` threads, running `fp` function
 */
struct tpool *create_threadpool(const uint16_t n, void *(*fp)(void *)) {
	if (n == 0)
		return NULL;

	struct tpool *tp = malloc(sizeof(struct tpool));
	if (tp == NULL) {
		perror("malloc() in create_tp() - tp");
		return NULL;
	}

	tp->size	= n;
	tp->threads = malloc((sizeof(tp->threads) * n) + 1);
	if (tp->threads == NULL) {
		perror("malloc() in create_tp() - threads");
		free(tp);
		return NULL;
	}

	/* CHECK: what to do if failure occurs ? */
	for (int i = 0; i < n; i++) {
		if (pthread_create(&tp->threads[i], NULL, fp, NULL) != 0) {
			perror("pthread_create() in create_tp()");
			return NULL;
		}

		if (pthread_detach(tp->threads[i]) != 0) {
			perror("pthread_create() in create_tp()");
			return NULL;
		}
	}

	return tp;
}

void delete_threadpool(struct tpool *tp) {
	if (tp == NULL)
		return;

	free(tp->threads);
	free(tp);
}
