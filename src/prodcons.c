#include "../include/prodcons.h"

void init_producer_consumer(struct prodcons *pc, int empty) {
	sem_init(&pc->queued, 0, 0);
	sem_init(&pc->empty, 0, empty);
	sem_init(&pc->mutex, 0, 1);
}

void destroy_producer_consumer(struct prodcons *pc) {
	sem_destroy(&pc->queued);
	sem_destroy(&pc->empty);
	sem_destroy(&pc->mutex);
}
