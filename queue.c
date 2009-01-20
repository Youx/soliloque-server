#include "queue.h"

#include <pthread.h>
#include <stdlib.h>

/**
 * Create a new queue
 *
 * @return the newly allocated queue
 */
struct queue *new_queue()
{
	struct queue *q = (struct queue *)calloc(sizeof(struct queue), 1);
	pthread_mutex_init(&q->mutex, NULL);

	return q;
}

void destroy_queue(struct queue *q)
{
	
}

/**
 * Add an element at the end of a queue.
 * Should be thread-safe
 *
 * @param q the queue
 * @param elem the element
 */
void add_to_queue(struct queue *q, void *elem)
{
	struct q_elem *q_e;

	q_e = (struct q_elem *)calloc(sizeof(struct q_elem), 1);
	q_e->elem = elem;

	pthread_mutex_lock(&q->mutex);
	if(q->first == NULL) {
		q->first = q_e;
	} else {
		q_e->prev = q->last;
		q->last->next = q_e;
	}
	q->last = q_e;
	pthread_mutex_unlock(&q->mutex);
}


/**
 * Get an element from the beginning of the
 * queue and remove it.
 * Should be thread-safe.
 *
 * @param q the queue
 *
 * @return the element
 */
void *get_from_queue(struct queue *q)
{
	void *elem;
	struct q_elem *new_first;

	pthread_mutex_lock(&q->mutex);
	if(q->first == 0) {	/* empty queue */
		elem = NULL;
	} else {
		new_first = q->first->next; /* NULL or the n-1th element */
		if(new_first == NULL)	/* only one element */
			q->last = NULL;
		/* retrieve the data and free the container */
		elem = q->first->elem;
		free(q->first);
		q->first = new_first;
		new_first->prev = NULL;
	}
	pthread_mutex_unlock(&q->mutex);

	return elem;
}

/**
 * Peek at the first element of the queue.
 *
 * @param q the queue
 *
 * @return the element
 */
void *peek_at_queue(struct queue *q)
{
	void *elem;

	pthread_mutex_lock(&q->mutex);
	if(q->first == 0) {	/* empty queue */
		elem = NULL;
	} else {
		elem = q->first->elem; /* retrieve the data */
	}
	pthread_mutex_unlock(&q->mutex);

	return elem;
}


