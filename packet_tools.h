#ifndef __PACKET_TOOLS_H__
#define __PACKET_TOOLS_H__

/*
 *  packet_tools.h
 *  sol-server
 *
 *  Created by Hugo Camboulive on 21/12/08.
 *  Copyright 2008 Université du Maine - IUP MIME. All rights reserved.
 *
 */

void packet_add_crc(char *data, size_t len, unsigned int offset);
int packet_check_crc(char *data, size_t len, unsigned int offset);
void packet_add_crc_d(char *data, size_t len);
int packet_check_crc_d(char *data, size_t len);

#endif
