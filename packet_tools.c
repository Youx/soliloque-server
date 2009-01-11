/*
 *  packet_tools.c
 *  sol-server
 *
 *  Created by Hugo Camboulive on 21/12/08.
 *  Copyright 2008 Université du Maine - IUP MIME. All rights reserved.
 *
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "packet_tools.h"
#include "compat.h"
#include "crc.h"

void packet_add_crc(char *data, unsigned int len, unsigned int offset)
{
	char *ptr = data + offset;
	uint32_t new_crc;
	uint32_t *crc_ptr = (uint32_t *)ptr;
	
	*crc_ptr = 0x00000000;
	new_crc = GUINT32_TO_LE(crc_32(data, len, 0xEDB88320));
	printf("new crc : 0x%x\n", new_crc);
	*crc_ptr = new_crc;
}

char packet_check_crc(char *data, unsigned int len, unsigned int offset)
{
	char *buff = (char *)calloc(sizeof(char), len);
	memcpy(buff, data, len);
	
	char *ptr = buff + offset;
	uint32_t old_crc;
	uint32_t new_crc;
	uint32_t *crc_ptr = (uint32_t *)ptr;
	
	old_crc = *crc_ptr;
	*crc_ptr = 0x00000000;
	new_crc = GUINT32_TO_LE(crc_32(buff, len, 0xEDB88320));
	
	//printf("OLD : 0x%x | NEW : 0x%x\n", old_crc, new_crc);
	return new_crc == old_crc;
}

void packed_add_crc_d(char *data, unsigned int len)
{
	packet_add_crc(data, len, 24);
}

char packet_check_crc_d(char *data, unsigned int len)
{
	return 	packet_check_crc(data, len, 24);
}
