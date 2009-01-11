/*
 *  packet_tools.h
 *  sol-server
 *
 *  Created by Hugo Camboulive on 21/12/08.
 *  Copyright 2008 Université du Maine - IUP MIME. All rights reserved.
 *
 */

void packet_add_crc(char *data, unsigned int len, unsigned int offset);
char packet_check_crc(char *data, unsigned int len, unsigned int offset);
void packet_add_crc_d(char *data, unsigned int len);
char packet_check_crc_d(char *data, unsigned int len);