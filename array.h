/*
 *  array.h
 *  sol-server
 *
 *  Created by Hugo Camboulive on 28/11/08.
 *  Copyright 2008 Université du Maine - IUP MIME. All rights reserved.
 *
 */
#ifndef __ARRAY_H__
#define __ARRAY_H__

struct array {
	void **array;
	size_t used_slots;
	size_t total_slots;
	size_t  max_slots;
};

extern size_t tmp_arr_iterator;

#define AR_OK 1

#define ar_each(type, el_ptr, a)\
for(tmp_arr_iterator=0 ; tmp_arr_iterator < a->total_slots ; tmp_arr_iterator++) {\
	if(a->array[tmp_arr_iterator] != NULL) {\
		el_ptr = (type) a->array[tmp_arr_iterator];

#define ar_end_each }}


struct array *ar_new(size_t size);
int ar_insert(struct array *a, void *elem);
void ar_remove(struct array *a, void *el);
int ar_get_n_elems_start_at(struct array *a, int max_elem, size_t start_at, void **res);
int ar_free(struct array *a);

#endif
