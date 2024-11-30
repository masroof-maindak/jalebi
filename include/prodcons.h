#ifndef PRODCONS_H
#define PRODCONS_H

#include <semaphore.h>

struct prodcons {
	sem_t queued; /* number of queued objects */
	sem_t empty;  /* number of empty positions */
	sem_t mutex;  /* binary semaphore */
};

void init_producer_consumer(struct prodcons *pc, int empty);
void destroy_producer_consumer(struct prodcons *pc);

#endif // PRODCONS_H
