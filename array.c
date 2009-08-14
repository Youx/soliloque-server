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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>
#include <strings.h>
#include <math.h>
#include <limits.h>
#include <errno.h>

#include "array.h"
#include "log.h"
#include "compat.h"

/**
 * Find the next available slot in the array.
 *
 * @param a the array
 *
 * @return the first slot available, or -1 if the array is already full
 */
static int ar_next_available(const struct array *a)
{
	size_t i;

	for (i = 0 ; i < a->total_slots ; i++) {
		if(a->array[i] == NULL)
			return (int)i;
	}
	return -1;
}

/**
 * Grow an array to twice its current size
 *
 * @param a the array that needs to be grown
 */
static int ar_grow(struct array *a)
{
	size_t old_size, new_size;
	void **tmp_alloc;

	if (a == NULL || a->array == NULL) {
		logger(LOG_WARN, "ar_grow : passed array is not allocated.\n");
		return 0;
	}

	if (a->total_slots < a->max_slots) {
		old_size = a->total_slots;
		new_size = MIN(a->total_slots * 2, a->max_slots);

		a->total_slots = new_size;
		tmp_alloc = (void **)realloc(a->array, sizeof(void *) * (a->total_slots));
		if (tmp_alloc == NULL) {
			logger(LOG_ERR, "ar_grow, realloc failed : %s\n", strerror(errno));
			return 0;
		}
		a->array = tmp_alloc;

		/* realloc does not set to zero!! */
		bzero(a->array + old_size, (a->total_slots - old_size) * sizeof(void *));
		return AR_OK;
	}
	return 0;
}

/**
 * Insert an element into the array, resizing if needed
 *
 * @param a the array
 * @param elem a generic pointer to an element
 *
 * @return 0 on failure, 1 on success
 */
int ar_insert(struct array *a, void *elem)
{
	int i;
	int err;

	pthread_mutex_lock(&a->lock);
	if (a->used_slots == a->total_slots) {
		err = ar_grow(a);
		if (err != AR_OK) {
			logger(LOG_ERR, "Could not grow array any further. Insertion impossible.\n");
			pthread_mutex_unlock(&a->lock);
			return 0;
		}
	}
	i = ar_next_available(a);
	if (i != -1) {
		a->array[i] = elem;
		a->used_slots++;
		pthread_mutex_unlock(&a->lock);
		return AR_OK;
	}
	pthread_mutex_unlock(&a->lock);
	return 0;
}

/**
 * Create a new empty array of a given size.
 *
 * @param size the initial size of the array
 *
 * @return the allocated array
 */
struct array *ar_new(size_t size)
{
	struct array *a;

	a = (struct array *)calloc(1, sizeof(struct array));
	if (a == NULL) {
		logger(LOG_ERR, "ar_new, calloc failed : %s\n", strerror(errno));
		return NULL;
	}
	a->total_slots = size;
	a->used_slots = 0;
	a->max_slots = (size_t) - 1;
	a->array = (void **)calloc(size, sizeof(void *));
	if (a->array == NULL) {
		logger(LOG_ERR, "ar_new, a->array calloc failed : %s\n", strerror(errno));
		free(a);
		return NULL;
	}
	pthread_mutex_init(&a->lock, NULL);
	return a;
}

/**
 * Remove the entry at the given index in the array
 * and shrinks the array if necessary.
 * Shrinking is not implemented yet.
 *
 * @param a the array
 * @param index the index of the element that has to be removed
 */
static void ar_remove_index(struct array *a, size_t idx)
{
	if (a == NULL || a->array == NULL) {
		logger(LOG_WARN, "ar_remove_index, passed array is unallocated.\n");
		pthread_mutex_unlock(&a->lock);
		return;
	}
	if (a->array[idx] != NULL) {
		a->array[idx] = NULL; /* clear the pointer */
		a->used_slots--;
	}
	return;
	/* Add some table shrinking logic 
	 * if too many slots are unused */
}

/**
 * Remove the element from the array
 * and shrinks the array if necessary.
 * This function is a wrapper around ar_remove_index.
 *
 * @param a the array
 * @param el a generic pointer to an element
 */
void ar_remove(struct array *a, void *el) 
{
	size_t i;
	pthread_mutex_lock(&a->lock);

	for (i=0 ; i < a->total_slots ; i++) {
		if (a->array[i] == el) {
			ar_remove_index(a, i);
		}
	}
	pthread_mutex_unlock(&a->lock);
}	

/**
 * Retrieves a given number of elements, starting at a given index
 * and put it into the array.
 *
 * @param a the array we will be picking elements from
 * @param max_elem the maximum number of elements we will take
 * @param start_at the number of element before  the ones we will pick
 * @param res the array we will put elements in
 *
 * @return the number of elements that were put into res (0 if none)
 */
int ar_get_n_elems_start_at(struct array *a, int max_elem, size_t start_at, void **res)
{
	size_t i=0;
	size_t nb_elem = 0;
	int el_counter = 0;

	pthread_mutex_lock(&a->lock);
	if (a->used_slots <= start_at) {
		pthread_mutex_unlock(&a->lock);
		return 0;
	}

	for (i=0 ; i < a->total_slots ; i++) {
		if (a->array[i] != NULL) {
			if (nb_elem >= start_at && nb_elem < (start_at + max_elem)) {
				res[nb_elem - start_at] = a->array[i];
				el_counter++;
			}
			nb_elem++;
		}
	}
	pthread_mutex_unlock(&a->lock);
	return el_counter;
}

int ar_free(struct array *a)
{
	size_t i;

	pthread_mutex_lock(&a->lock);
	if (a == NULL || a->array == NULL) {
		logger(LOG_ERR, "ar_free : Trying to free an unallocated array.\n");
		pthread_mutex_unlock(&a->lock);
		return 0;
	}

	for (i = 0 ; i < a->total_slots ; i++) {
		if (a->array[i] != NULL) {
			pthread_mutex_unlock(&a->lock);
			return 0;
		}
	}
	free(a->array);
	pthread_mutex_unlock(&a->lock);
	free(a);
	return AR_OK;
}
