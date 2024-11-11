#include <stdio.h>
#include <string.h>

#include "../include/queue.h"

struct queue *create_queue(size_t elemSize) {
	struct queue *q = malloc(sizeof(struct queue));
	if (q == NULL) {
		perror("malloc() in create_queue() - q");
		return NULL;
	}

	q->header = malloc(sizeof(struct node));
	if (q->header == NULL) {
		perror("malloc() in create_queue() - header");
		free(q);
		return NULL;
	}

	q->sentinel = malloc(sizeof(struct node));
	if (q->sentinel == NULL) {
		perror("malloc() in create_queue() - sentinel");
		free(q->header);
		free(q);
		return NULL;
	}

	q->header->data = q->sentinel->data = NULL;
	q->header->prev = q->sentinel->next = NULL;

	q->header->next	  = q->sentinel;
	q->sentinel->prev = q->header;
	q->size			  = 0;
	q->elemSize		  = elemSize;

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
	q = NULL;
}

/**
 * @brief adds a ndoe at the end of the queue
 * @param data must point to a chunk of memory that is the same size as the
 * argument used to create a queue object
 */
void enqueue(struct queue *q, const void *data) {
	struct node *new = malloc(sizeof(struct node));
	if (new == NULL) {
		perror("malloc() #1 in enqueue()");
		return;
	}

	new->data = malloc(q->elemSize);
	if (new->data == NULL) {
		perror("malloc() #2 in enqueue()");
		return;
	}

	struct node *secLast = q->sentinel->prev;
	secLast->next		 = new;
	q->sentinel->prev	 = new;
	new->prev			 = secLast;
	new->next			 = q->sentinel;

	memcpy(new->data, data, q->elemSize);
	q->size++;
}

/**
 * @brief removes the earliest node added
 */
void dequeue(struct queue *q) {
	if (q->size == 0)
		return;

	struct node *n	= q->header->next;
	q->header->next = n->next;
	n->next->prev	= q->header;

	q->size--;

	free(n->data);
	n->data = NULL;

	free(n);
	n = NULL;
}

/**
 * @brief returns a copy of the value at the head of the queue
 */
void *top(struct queue *q) {
	if (q->size == 0)
		return NULL;

	void *data = malloc(q->elemSize);
	if (data == NULL) {
		perror("malloc() in top()");
		return NULL;
	}

	memcpy(data, q->header->next->data, q->elemSize);
	return data;
}

void for_each_data(struct queue *q, void (*fn)(void *)) {
	for (struct node *n = q->header->next; n != q->sentinel; n = n->next)
		fn(n->data);
}
