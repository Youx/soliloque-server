/*
 *  packet_tools.c
 *  sol-server
 *
 *  Created by Hugo Camboulive on 21/12/08.
 *  Copyright 2008 Hugo Camboulive
 *
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "packet_tools.h"
#include "compat.h"
#include "crc.h"

/**
 * Add a crc to a given data packet.
 *
 * @param data the packet
 * @param the length of the packet
 * @param offset the offset where we want to write our checksum.
 */
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

/**
 * Check the crc of a packet
 *
 * @param data the packet
 * @param len the length of the packet
 * @param offset the offset where the checksum is located.
 */
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

/**
 * Add a crc to a packet at the default offset
 * Most packets have the checksum at start+24 bytes, 
 * but some have it at start+16 bytes.
 *
 * @param data the packet
 * @param len the length of the packet
 */
void packed_add_crc_d(char *data, unsigned int len)
{
	packet_add_crc(data, len, 24);
}

/**
 * Check the crc of a packet at the default offset
 * Most packets have the checksum at start+24 bytes,
 * but some have it at start+16 bytes.
 *
 * @param data the packet
 * @param len the length of the packet
 */
char packet_check_crc_d(char *data, unsigned int len)
{
	return 	packet_check_crc(data, len, 24);
}
