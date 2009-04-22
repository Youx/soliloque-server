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

#ifndef __AUDIO_PACKET_H__
#define __AUDIO_PACKET_H__

#include "server.h"

#define CODEC_CELP_5_1    0
#define CODEC_CELP_6_3    1
#define CODEC_GSM_14_8    2
#define CODEC_GSM_16_4    3
#define CODEC_CELPWin_5_2 4
#define CODEC_SPEEX_3_4   5
#define CODEC_SPEEX_5_2   6
#define CODEC_SPEEX_7_2   7
#define CODEC_SPEEX_9_3   8
#define CODEC_SPEEX_12_3  9
#define CODEC_SPEEX_16_3  10
#define CODEC_SPEEX_19_6  11
#define CODEC_SPEEX_25_9  12

struct server;

int audio_received(char *in, size_t len, struct server *s);

#endif
