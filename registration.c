#include "registration.h"

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/**
 * Allocate and return a new registration structure
 * used to check credentials on login.
 *
 * @return the allocated structure
 */
struct registration *new_registration()
{
	struct registration *r;

	r = (struct registration *)calloc(1, sizeof(struct registration));
	if (r == NULL) {
		printf("(WW) new_registration, calloc failed : %s.\n", strerror(errno));
		return NULL;
	}

	return r;
}
