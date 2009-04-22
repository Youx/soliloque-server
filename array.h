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

#ifndef __ARRAY_H__
#define __ARRAY_H__

#include <pthread.h>

struct array {
	void **array;
	size_t used_slots;
	size_t total_slots;
	size_t  max_slots;

	pthread_mutex_t lock;
};


#define AR_OK 1

#define ar_each(type, el_ptr, iter, a)\
for(iter=0 ; iter < a->total_slots ; iter++) {\
	if(a->array[iter] != NULL) {\
		el_ptr = (type) a->array[iter];

#define ar_end_each }}


struct array *ar_new(size_t size);
int ar_insert(struct array *a, void *elem);
void ar_remove(struct array *a, void *el);
int ar_get_n_elems_start_at(struct array *a, int max_elem, size_t start_at, void **res);
int ar_free(struct array *a);

#endif
