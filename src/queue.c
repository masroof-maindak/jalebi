#include <stdio.h>
#include <string.h>

#include "../include/queue.h"

struct queue *create_queue(size_t elemSize) {
	struct queue *q = malloc(sizeof(struct queue));
	if (q == NULL) {
		perror("malloc()");
		return NULL;
	}

	q->header = malloc(sizeof(struct node));
	if (q->header == NULL) {
		perror("malloc()");
		free(q);
		return NULL;
	}

	q->sentinel = malloc(sizeof(struct node));
	if (q->sentinel == NULL) {
		perror("malloc()");
		free(q->header);
		free(q);
		return NULL;
	}

	q->header->prev	  = NULL;
	q->header->next	  = q->sentinel;
	q->sentinel->next = NULL;
	q->sentinel->prev = q->header;

	q->size		= 0;
	q->elemSize = elemSize;

	return q;
}

void delete_queue(struct queue *q) {
	struct node *curr = q->header->next;

	while (curr != q->sentinel) {
		struct node *next = curr->next;
		free(curr->data);
		free(curr);
		curr = next;
	}

	free(q->header);
	free(q->sentinel);

	q->header	= NULL;
	q->sentinel = NULL;
	q->size		= -1;
	q->elemSize = -1;

	free(q);
}

void enqueue(struct queue *q, const void *data) {
	struct node *new = malloc(sizeof(struct node));
	if (new == NULL) {
		perror("malloc()");
		return;
	}

	new->data = malloc(sizeof(q->elemSize));
	if (new->data == NULL) {
		perror("malloc()");
		return;
	}

	struct node *secLast = q->sentinel->prev;
	secLast->next		 = new;
	q->sentinel->prev	 = new;
	new->prev			 = secLast;
	new->next			 = q->sentinel;

	memcpy(new->data, data, q->elemSize);
	q->size++;
	return;
}

void dequeue(struct queue *q) {
	if (q->size == 0)
		return;

	struct node *rem = q->header->next;
	q->header->next	 = rem->next;
	rem->next->prev	 = q->header;

	q->size--;

	free(rem->data);
	free(rem);
}

void for_each_data(struct queue *q, void (*fn)(void *)) {
	for (struct node *n = q->header->next; n != q->sentinel; n = n->next)
		fn(n->data);
}
