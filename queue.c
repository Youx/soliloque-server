/*
 * soliloque-server, an open source implementation of the TeamSpeak protocol.
 * Copyright (C) 2009 Hugo Camboulive <hugo.camboulive AT gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "queue.h"
#include "log.h"

#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>

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
	if (q->first != NULL)
		logger(LOG_WARN, "destroy_queue : destroyed a queue that was NOT empty! That should not happen!");
	pthread_mutex_destroy(&q->mutex);
	free(q);
}

/**
 * Add an element at the end of a queue.
 * Should be thread-safe
 *
 * @param q the queue
 * @param elem the element
 */
void add_to_queue(struct queue *q, void *elem, size_t size)
{
	struct q_elem *q_e;

	q_e = (struct q_elem *)calloc(sizeof(struct q_elem), 1);
	q_e->elem = elem;
	q_e->size = size;
	timerclear(&q_e->last_sent);

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

void queue_update_time(struct queue *q)
{
	if (q->first != NULL)
		gettimeofday(&q->first->last_sent, NULL);
}

struct timeval * queue_get_time(struct queue *q)
{
	if (q->first != NULL)
		return &q->first->last_sent;
	return NULL;
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
	struct q_elem *old_first;

	old_first = q->first;

	if(old_first == NULL) {	/* empty queue */
		elem = NULL;
	} else {
		new_first = old_first->next; /* NULL or the n-1th element */
		if(new_first == NULL) {
		/* only one element */
			q->last = NULL;
			q->first = NULL;
		} else {
			/* retrieve the data and free the container */
			q->first = new_first;
			new_first->prev = NULL;
		}
		elem = old_first->elem;
		free(old_first);
	}

	return elem;
}

/**
 * Peek at the first element of the queue.
 * NB : the queue mutex has to be locked MANUALLY
 * when using the peek function.
 *
 * @param q the queue
 *
 * @return the element
 */
void *peek_at_queue(struct queue *q)
{
	void *elem;

	if(q->first == 0) {	/* empty queue */
		elem = NULL;
	} else {
		elem = q->first->elem; /* retrieve the data */
	}

	return elem;
}

size_t peek_at_size(struct queue *q)
{
	size_t size;

	if(q->first == 0) {	/* empty queue */
		size = 0;
	} else {
		size = q->first->size; /* retrieve the data */
	}

	return size;
}
