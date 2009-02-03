#include "registration.h"

#include <stdlib.h>

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

	return r;
}
