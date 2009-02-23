/**
 * CRC32 implementation, copied from zlib
 * http://www.zlib.net
 */
#include <stdio.h>
#include <sys/types.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "compat.h"

static void crc32_table(uint32_t poly, uint32_t *table)
{
	uint32_t i, j;

	for (i = 0 ; i < 256 ; i++) {
		table[i] = i;
		for(j = 8 ; j > 0 ; j--) {
			if((table[i] & 1) != 0)
				table[i] = (table[i] >> 1) ^ poly;
			else
				table[i] >>= 1;
		}
	}     
}

uint32_t crc_32(void *str, size_t length, uint32_t poly)
{
	uint32_t table[256];
	uint32_t crc;
	size_t i;

	bzero(table, 256);
	crc32_table(poly, table);

	crc = 0xFFFFFFFF;
	for (i = 0 ; i < length ; i++)
		crc = (crc >> 8) ^ table[(((uint8_t *)str)[i] ^ crc) & 0x000000FF];

	return ~crc;
}
