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
#include "array.h"
#include <sys/types.h>
#include <assert.h>
#include "compat.h"

int tmp_arr_iterator;

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
	for(i=0 ; i< a->total_slots ; i++) {
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
 */
void ar_insert(struct array *a, void * elem)
{
	int i;

	if(a->used_slots == a->total_slots)
		ar_grow(a);
	i = ar_next_available(a);
	a->array[i] = elem;
	a->used_slots++;
}

/**
 * Grow an array to twice its current size
 *
 * @param a the array that needs to be grown
 */
void ar_grow(struct array *a)
{
	a->total_slots *= 2;
	a->array = (void **)realloc(a->array, sizeof(void *) * (a->total_slots) );
}

/**
 * Create a new empty array of a given size.
 *
 * @param size the initial size of the array
 *
 * @return the allocated array
 */
struct array * ar_new(int size)
{
	struct array * a;
	a = (struct array *)calloc(sizeof(struct array), 1);
	a->total_slots = size;
	a->used_slots = 0;
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
	if(a->array[index] != NULL) {
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

	for(i=0 ; i<a->total_slots ; i++) {
		if(a->array[i] == el) {
			ar_remove_index(a, i);
		}
	}
}	

#if 0

struct array * arr_test;

int main()
{
	uint32_t i, j;

	arr_test = ar_new(10);
	for(i=0;i<10;i++)
		printf("%i\n", arr_test->array[i]);

	printf("Should now work..\n");
	for(i=0;i<10;i++)
		ar_grow(arr_test);
	printf("new size = %i\n", arr_test->total_slots);
	for(i=0 ; i<arr_test->total_slots ; i++) {
		j = ar_next_available(arr_test);
		printf("next available = %i\n", j);
		ar_insert(arr_test, (void *)i+1);
	}
	for(i=0 ; i<arr_test->total_slots ; i++) {
		arr_test->array[i] = 1;
	}
}

#endif
