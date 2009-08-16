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
