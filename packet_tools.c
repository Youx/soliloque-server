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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "packet_tools.h"
#include "compat.h"
#include "crc.h"
#include "log.h"

/**
 * Add a crc to a given data packet.
 *
 * @param data the packet
 * @param len the length of the packet
 * @param offset the offset where we want to write our checksum.
 */
void packet_add_crc(char *data, size_t len, unsigned int offset)
{
	char *ptr = data + offset;
	uint32_t new_crc;
	uint32_t *crc_ptr = (uint32_t *)ptr;
	
	*crc_ptr = 0x00000000;
	new_crc = GUINT32_TO_LE(crc_32(data, len, 0xEDB88320));
	*crc_ptr = new_crc;
}

/**
 * Check the crc of a packet
 *
 * @param data the packet
 * @param len the length of the packet
 * @param offset the offset where the checksum is located.
 */
int packet_check_crc(char *data, size_t len, unsigned int offset)
{
	char *buff, *ptr;
	uint32_t old_crc;
	uint32_t new_crc;
	uint32_t *crc_ptr;

	buff = (char *)calloc(len, sizeof(char));
	if (buff == NULL) {
		ERROR("packet_check_crc, buffer allocation failed : %s.", strerror(errno));
		return 0;
	}
	memcpy(buff, data, len);
	ptr = buff + offset;
	crc_ptr = (uint32_t *)ptr;

	old_crc = *crc_ptr;
	*crc_ptr = 0x00000000;
	new_crc = GUINT32_TO_LE(crc_32(buff, len, 0xEDB88320));
	
	free(buff);
	return new_crc == old_crc;
}

/**
 * Add a crc to a packet at the default offset
 * Most packets have the checksum at start+20 bytes,
 * but some have it at start+16 bytes.
 *
 * @param data the packet
 * @param len the length of the packet
 */
void packet_add_crc_d(char *data, size_t len)
{
	packet_add_crc(data, len, 20);
}

/**
 * Check the crc of a packet at the default offset
 * Most packets have the checksum at start+20 bytes,
 * but some have it at start+16 bytes.
 *
 * @param data the packet
 * @param len the length of the packet
 */
int packet_check_crc_d(char *data, size_t len)
{
	return packet_check_crc(data, len, 20);
}
