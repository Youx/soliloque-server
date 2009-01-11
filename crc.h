/**
 * file 		:	crc.h
 * creator		:	Hugo Camboulive
 * description	:	Include to generate CRC32s
 */

#ifndef __CRC32_H__
#define __CRC32_H__

#include "compat.h"

uint32_t crc_32(void * str, int length, uint32_t poly);

#endif

