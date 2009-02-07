/**
 *  array.c
 *  sol-server
 *
 *  Created by Hugo Camboulive on 28/11/08.
 *  Copyright 2008 Hugo Camboulive.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "array.h"
#include <sys/types.h>
#include <assert.h>
#include <strings.h>
#include <math.h>
#include <limits.h>

#include "compat.h"

int tmp_arr_iterator;

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))


/**
 * Find the next available slot in the array.
 *
 * @param a the array
 *
 * @return the first slot available, or -1 if the array is already full
 */
int ar_next_available(const struct array *a)
{
	int i;

	for (i=0 ; i < a->total_slots ; i++) {
		if(a->array[i] == NULL)
			return i;
	}
	return -1;
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

	if (a->used_slots == a->total_slots) {
		ar_grow(a);
	}
	i = ar_next_available(a);
	if (i != -1) {
		a->array[i] = elem;
		a->used_slots++;
		return 1;
	}
	return 0;
}

/**
 * Grow an array to twice its current size
 *
 * @param a the array that needs to be grown
 */
int ar_grow(struct array *a)
{
	int old_size, new_size;

	if (a->total_slots < a->max_slots) {
		old_size = a->total_slots;
		new_size = MIN(a->total_slots * 2, a->max_slots);

		a->total_slots = new_size;
		a->array = (void **)realloc(a->array, sizeof(void *) * (a->total_slots));
		/* realloc does not set to zero!! */
		bzero(a->array + old_size, (a->total_slots - old_size) * sizeof(void *));
		return 1;
	}
	return 0;
}

/**
 * Create a new empty array of a given size.
 *
 * @param size the initial size of the array
 *
 * @return the allocated array
 */
struct array *ar_new(int size)
{
	struct array *a;
	a = (struct array *)calloc(sizeof(struct array), 1);
	a->total_slots = size;
	a->used_slots = 0;
	a->max_slots = INT_MAX;
	a->array = (void **)calloc(sizeof(void *), size);
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
void ar_remove_index(struct array *a, int index)
{
	if (a->array[index] != NULL) {
		a->array[index] = NULL; /* clear the pointer */
		a->used_slots--;
	}
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
	int i;

	for (i=0 ; i < a->total_slots ; i++) {
		if (a->array[i] == el) {
			ar_remove_index(a, i);
		}
	}
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
int ar_get_n_elems_start_at(struct array *a, int max_elem, int start_at, void **res)
{
	int i=0;
	int nb_elem = 0;
	int el_counter = 0;

	if (a->used_slots <= start_at)
		return 0;

	for (i=0 ; i < a->total_slots ; i++) {
		if (a->array[i] != NULL) {
			if (nb_elem >= start_at && nb_elem < (start_at + max_elem)) {
				res[nb_elem - start_at] = a->array[i];
				el_counter++;
			}
			nb_elem++;
		}
	}
	return el_counter;
}

#ifdef TEST_ARRAY

int main()
{
	uint32_t i, j;
	struct array *arr_test;
	void *test[4] = {NULL, NULL, NULL, NULL};
	int res;

	arr_test = ar_new(100);

	for (i = 0 ; i < arr_test->total_slots ; i++) {
		ar_insert(arr_test, (void *)i+1);
	}
	for (i = 0 ; i < arr_test->total_slots ; i++) {
		printf("%i\n", arr_test->array[i]);
	}
	res = ar_get_n_elems_start_at(arr_test, 4, 98, test);
	for (i = 0 ; i < 4 ; i++) {
		printf("%i\n", test[i]);
	}
	printf("NB elem : %i\n", res);
}

#endif
