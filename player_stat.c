#include "player_stat.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

/**
 * Allocate a new player stat structure
 */
struct player_stat *new_plstat()
{
	struct player_stat *ps;

	ps = (struct player_stat *)calloc(1, sizeof(struct player_stat));
	if (ps == NULL) {
		printf("(WW) new_plstat, calloc failed : %s.\n", strerror(errno));
		return NULL;
	}
	return ps;
}
