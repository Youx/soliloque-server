#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <pthread.h>

struct q_elem
{
	size_t size;
	void *elem;
	struct q_elem *prev;
	struct q_elem *next;
};

struct queue
{
	struct q_elem *first;
	struct q_elem *last;
	int nb_elem;

	pthread_mutex_t mutex;
};

struct queue *new_queue();
void destroy_queue(struct queue *q);
void add_to_queue(struct queue *q, void *elem, size_t size);
void *get_from_queue(struct queue *q);
void *peek_at_queue(struct queue *q);
size_t peek_at_size(struct queue *q);
#endif
