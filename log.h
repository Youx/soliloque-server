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

#ifndef __LOG_H__
#define __LOG_H__

#include <stdarg.h>

#include "configuration.h"

#define LOG_ERR 1
#define LOG_WARN 2
#define LOG_INFO 3
#define LOG_DBG 4

#ifndef LOG_LEVEL
#define LOG_LEVEL 3
#endif

void logger(int loglevel, char *str, ...);
void set_config(struct config *cfg);

/* define log at compile time */
#if LOG_LEVEL >= LOG_ERR
#define ERROR(args...) \
logger(LOG_ERR, ## args)
#else /* do nothing */
#define ERROR(args...)
#endif

#if LOG_LEVEL >= LOG_WARN
#define WARNING(args...) \
logger(LOG_WARN, ## args)
#else /* do nothing */
#define WARNING(args...)
#endif

#if LOG_LEVEL >= LOG_INFO
#define INFO(args...) \
logger(LOG_INFO, ## args)
#else /* do nothing */
#define INFO(args...)
#endif

#if LOG_LEVEL >= LOG_DBG
#define DEBUG(args...) \
logger(LOG_DBG, ## args)
#else
#define DEBUG(...) /* do nothing */
#endif

#endif
