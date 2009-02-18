#include "player_stat.h"

#include <stdlib.h>

/**
 * Allocate a new player stat structure
 */
struct player_stat *new_plstat()
{
	return (struct player_stat *)calloc(1, sizeof(struct player_stat));
}
