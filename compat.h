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
 *
 *				* * *
 *
 * endianness macros (originally gtypes.h) : GLIB
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __COMPAT_H__
#define __COMPAT_H__

#include <stdint.h>
#include <stddef.h>
#include "config.h"



#undef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))


#ifndef HAVE_STRNDUP

#include <sys/types.h>
char *strndup(char const *s, size_t n);

#endif

char *ustrtohex (unsigned char *data, size_t len);
void wu64(uint64_t val, char **ptr);
void wu32(uint32_t val, char **ptr);
void wu16(uint16_t val, char **ptr);
void wu8(uint8_t val, char **ptr);
void wstaticstring(char *str, int maxlen, char **ptr);
uint64_t ru64(char **ptr);
uint32_t ru32(char **ptr);
uint16_t ru16(char **ptr);
uint8_t ru8(char **ptr);
char *rstaticstring(int maxlen, char **ptr);

#ifdef __APPLE__
#include <machine/endian.h>
#else
#include <endian.h>
#endif

#ifndef NULL
#define NULL 0
#endif


#if BYTE_ORDER == LITTLE_ENDIAN

#define GINT16_TO_LE(val)      ((int16_t) (val))
#define GUINT16_TO_LE(val)     ((uint16_t) (val))
#define GINT16_TO_BE(val)       ((int16_t) GUINT16_SWAP_LE_BE (val))
#define GUINT16_TO_BE(val)      (GUINT16_SWAP_LE_BE (val))

#define GINT32_TO_LE(val)       ((int32_t) (val))
#define GUINT32_TO_LE(val)      ((uint32_t) (val))
#define GINT32_TO_BE(val)       ((int32_t) GUINT32_SWAP_LE_BE (val))
#define GUINT32_TO_BE(val)      (GUINT32_SWAP_LE_BE (val))

#define GINT64_TO_LE(val)       ((int64_t) (val))
#define GUINT64_TO_LE(val)      ((uint64_t) (val))
#define GINT64_TO_BE(val)       ((int64_t) GUINT64_SWAP_LE_BE (val))
#define GUINT64_TO_BE(val)      (GUINT64_SWAP_LE_BE (val))

#else

#define GINT16_TO_LE(val)      ((int16_t) GUINT16_SWAP_LE_BE (val))
#define GUINT16_TO_LE(val)     (GUINT16_SWAP_LE_BE (val))
#define GINT16_TO_BE(val)       ((int16_t) (val))
#define GUINT16_TO_BE(val)      ((uint16_t) (val))

#define GINT32_TO_LE(val)       ((int32_t) GUINT32_SWAP_LE_BE (val))
#define GUINT32_TO_LE(val)      (GUINT32_SWAP_LE_BE (val))
#define GINT32_TO_BE(val)       ((int32_t) (val))
#define GUINT32_TO_BE(val)      ((uint32_t) (val))

#define GINT64_TO_LE(val)       ((int64_t) GUINT64_SWAP_LE_BE (val))
#define GUINT64_TO_LE(val)      (GUINT64_SWAP_LE_BE (val))
#define GINT64_TO_BE(val)       ((int64_t) (val))
#define GUINT64_TO_BE(val)      ((uint64_t) (val))

#endif

/* Basic bit swapping functions
 */
#define GUINT16_SWAP_LE_BE_CONSTANT(val)	((uint16_t) ( \
    (uint16_t) ((uint16_t) (val) >> 8) |	\
    (uint16_t) ((uint16_t) (val) << 8)))

#define GUINT32_SWAP_LE_BE_CONSTANT(val)	((uint32_t) ( \
    (((uint32_t) (val) & (uint32_t) 0x000000ffU) << 24) | \
    (((uint32_t) (val) & (uint32_t) 0x0000ff00U) <<  8) | \
    (((uint32_t) (val) & (uint32_t) 0x00ff0000U) >>  8) | \
    (((uint32_t) (val) & (uint32_t) 0xff000000U) >> 24)))

#define GUINT64_SWAP_LE_BE_CONSTANT(val)	((uint64_t) ( \
      (((uint64_t) (val) &						\
	(uint64_t) 0x00000000000000ffULL) << 56) |	\
      (((uint64_t) (val) &						\
	(uint64_t) 0x000000000000ff00ULL) << 40) |	\
      (((uint64_t) (val) &						\
	(uint64_t) 0x0000000000ff0000ULL) << 24) |	\
      (((uint64_t) (val) &						\
	(uint64_t) 0x00000000ff000000ULL) <<  8) |	\
      (((uint64_t) (val) &						\
	(uint64_t) 0x000000ff00000000ULL) >>  8) |	\
      (((uint64_t) (val) &						\
	(uint64_t) 0x0000ff0000000000ULL) >> 24) |	\
      (((uint64_t) (val) &						\
	(uint64_t) 0x00ff000000000000ULL) >> 40) |	\
      (((uint64_t) (val) &						\
	(uint64_t) 0xff00000000000000ULL) >> 56)))

/* Arch specific stuff for speed
 */
#if defined (__GNUC__) && (__GNUC__ >= 2) && defined (__OPTIMIZE__)
#  if defined (__i386__)
#    define GUINT16_SWAP_LE_BE_IA32(val) \
       (__extension__						\
	({ register uint16_t __v, __x = ((uint16_t) (val));	\
	   if (__builtin_constant_p (__x))			\
	     __v = GUINT16_SWAP_LE_BE_CONSTANT (__x);		\
	   else							\
	     __asm__ ("rorw $8, %w0"				\
		      : "=r" (__v)				\
		      : "0" (__x)				\
		      : "cc");					\
	    __v; }))
#    if !defined (__i486__) && !defined (__i586__) \
	&& !defined (__pentium__) && !defined (__i686__) \
	&& !defined (__pentiumpro__) && !defined (__pentium4__)
#       define GUINT32_SWAP_LE_BE_IA32(val) \
	  (__extension__					\
	   ({ register uint32_t __v, __x = ((uint32_t) (val));	\
	      if (__builtin_constant_p (__x))			\
		__v = GUINT32_SWAP_LE_BE_CONSTANT (__x);	\
	      else						\
		__asm__ ("rorw $8, %w0\n\t"			\
			 "rorl $16, %0\n\t"			\
			 "rorw $8, %w0"				\
			 : "=r" (__v)				\
			 : "0" (__x)				\
			 : "cc");				\
	      __v; }))
#    else /* 486 and higher has bswap */
#       define GUINT32_SWAP_LE_BE_IA32(val) \
	  (__extension__					\
	   ({ register uint32_t __v, __x = ((uint32_t) (val));	\
	      if (__builtin_constant_p (__x))			\
		__v = GUINT32_SWAP_LE_BE_CONSTANT (__x);	\
	      else						\
		__asm__ ("bswap %0"				\
			 : "=r" (__v)				\
			 : "0" (__x));				\
	      __v; }))
#    endif /* processor specific 32-bit stuff */
#    define GUINT64_SWAP_LE_BE_IA32(val) \
       (__extension__							\
	({ union { uint64_t __ll;					\
		   uint32_t __l[2]; } __w, __r;				\
	   __w.__ll = ((uint64_t) (val));				\
	   if (__builtin_constant_p (__w.__ll))				\
	     __r.__ll = GUINT64_SWAP_LE_BE_CONSTANT (__w.__ll);		\
	   else								\
	     {								\
	       __r.__l[0] = GUINT32_SWAP_LE_BE (__w.__l[1]);		\
	       __r.__l[1] = GUINT32_SWAP_LE_BE (__w.__l[0]);		\
	     }								\
	   __r.__ll; }))
     /* Possibly just use the constant version and let gcc figure it out? */
#    define GUINT16_SWAP_LE_BE(val) (GUINT16_SWAP_LE_BE_IA32 (val))
#    define GUINT32_SWAP_LE_BE(val) (GUINT32_SWAP_LE_BE_IA32 (val))
#    define GUINT64_SWAP_LE_BE(val) (GUINT64_SWAP_LE_BE_IA32 (val))
#  elif defined (__ia64__)
#    define GUINT16_SWAP_LE_BE_IA64(val) \
       (__extension__						\
	({ register uint16_t __v, __x = ((uint16_t) (val));	\
	   if (__builtin_constant_p (__x))			\
	     __v = GUINT16_SWAP_LE_BE_CONSTANT (__x);		\
	   else							\
	     __asm__ __volatile__ ("shl %0 = %1, 48 ;;"		\
				   "mux1 %0 = %0, @rev ;;"	\
				    : "=r" (__v)		\
				    : "r" (__x));		\
	    __v; }))
#    define GUINT32_SWAP_LE_BE_IA64(val) \
       (__extension__						\
	 ({ register uint32_t __v, __x = ((uint32_t) (val));	\
	    if (__builtin_constant_p (__x))			\
	      __v = GUINT32_SWAP_LE_BE_CONSTANT (__x);		\
	    else						\
	     __asm__ __volatile__ ("shl %0 = %1, 32 ;;"		\
				   "mux1 %0 = %0, @rev ;;"	\
				    : "=r" (__v)		\
				    : "r" (__x));		\
	    __v; }))
#    define GUINT64_SWAP_LE_BE_IA64(val) \
       (__extension__						\
	({ register uint64_t __v, __x = ((uint64_t) (val));	\
	   if (__builtin_constant_p (__x))			\
	     __v = GUINT64_SWAP_LE_BE_CONSTANT (__x);		\
	   else							\
	     __asm__ __volatile__ ("mux1 %0 = %1, @rev ;;"	\
				   : "=r" (__v)			\
				   : "r" (__x));		\
	   __v; }))
#    define GUINT16_SWAP_LE_BE(val) (GUINT16_SWAP_LE_BE_IA64 (val))
#    define GUINT32_SWAP_LE_BE(val) (GUINT32_SWAP_LE_BE_IA64 (val))
#    define GUINT64_SWAP_LE_BE(val) (GUINT64_SWAP_LE_BE_IA64 (val))
#  elif defined (__x86_64__)
#    define GUINT32_SWAP_LE_BE_X86_64(val) \
       (__extension__						\
	 ({ register uint32_t __v, __x = ((uint32_t) (val));	\
	    if (__builtin_constant_p (__x))			\
	      __v = GUINT32_SWAP_LE_BE_CONSTANT (__x);		\
	    else						\
	     __asm__ ("bswapl %0"				\
		      : "=r" (__v)				\
		      : "0" (__x));				\
	    __v; }))
#    define GUINT64_SWAP_LE_BE_X86_64(val) \
       (__extension__						\
	({ register uint64_t __v, __x = ((uint64_t) (val));	\
	   if (__builtin_constant_p (__x))			\
	     __v = GUINT64_SWAP_LE_BE_CONSTANT (__x);		\
	   else							\
	     __asm__ ("bswapq %0"				\
		      : "=r" (__v)				\
		      : "0" (__x));				\
	   __v; }))
     /* gcc seems to figure out optimal code for this on its own */
#    define GUINT16_SWAP_LE_BE(val) (GUINT16_SWAP_LE_BE_CONSTANT (val))
#    define GUINT32_SWAP_LE_BE(val) (GUINT32_SWAP_LE_BE_X86_64 (val))
#    define GUINT64_SWAP_LE_BE(val) (GUINT64_SWAP_LE_BE_X86_64 (val))
#  else /* generic gcc */
#    define GUINT16_SWAP_LE_BE(val) (GUINT16_SWAP_LE_BE_CONSTANT (val))
#    define GUINT32_SWAP_LE_BE(val) (GUINT32_SWAP_LE_BE_CONSTANT (val))
#    define GUINT64_SWAP_LE_BE(val) (GUINT64_SWAP_LE_BE_CONSTANT (val))
#  endif
#else /* generic */
#  define GUINT16_SWAP_LE_BE(val) (GUINT16_SWAP_LE_BE_CONSTANT (val))
#  define GUINT32_SWAP_LE_BE(val) (GUINT32_SWAP_LE_BE_CONSTANT (val))
#  define GUINT64_SWAP_LE_BE(val) (GUINT64_SWAP_LE_BE_CONSTANT (val))
#endif /* generic */

#define GUINT16_SWAP_LE_PDP(val)	((uint16_t) (val))
#define GUINT16_SWAP_BE_PDP(val)	(GUINT16_SWAP_LE_BE (val))
#define GUINT32_SWAP_LE_PDP(val)	((uint32_t) ( \
    (((uint32_t) (val) & (uint32_t) 0x0000ffffU) << 16) | \
    (((uint32_t) (val) & (uint32_t) 0xffff0000U) >> 16)))
#define GUINT32_SWAP_BE_PDP(val)	((uint32_t) ( \
    (((uint32_t) (val) & (uint32_t) 0x00ff00ffU) << 8) | \
    (((uint32_t) (val) & (uint32_t) 0xff00ff00U) >> 8)))

/* The G*_TO_?E() macros are defined in glibconfig.h.
 * The transformation is symmetric, so the FROM just maps to the TO.
 */
#define GINT16_FROM_LE(val)	(GINT16_TO_LE (val))
#define GUINT16_FROM_LE(val)	(GUINT16_TO_LE (val))
#define GINT16_FROM_BE(val)	(GINT16_TO_BE (val))
#define GUINT16_FROM_BE(val)	(GUINT16_TO_BE (val))
#define GINT32_FROM_LE(val)	(GINT32_TO_LE (val))
#define GUINT32_FROM_LE(val)	(GUINT32_TO_LE (val))
#define GINT32_FROM_BE(val)	(GINT32_TO_BE (val))
#define GUINT32_FROM_BE(val)	(GUINT32_TO_BE (val))

#define GINT64_FROM_LE(val)	(GINT64_TO_LE (val))
#define GUINT64_FROM_LE(val)	(GUINT64_TO_LE (val))
#define GINT64_FROM_BE(val)	(GINT64_TO_BE (val))
#define GUINT64_FROM_BE(val)	(GUINT64_TO_BE (val))

#define GLONG_FROM_LE(val)	(GLONG_TO_LE (val))
#define GULONG_FROM_LE(val)	(GULONG_TO_LE (val))
#define GLONG_FROM_BE(val)	(GLONG_TO_BE (val))
#define GULONG_FROM_BE(val)	(GULONG_TO_BE (val))

#define GINT_FROM_LE(val)	(GINT_TO_LE (val))
#define GUINT_FROM_LE(val)	(GUINT_TO_LE (val))
#define GINT_FROM_BE(val)	(GINT_TO_BE (val))
#define GUINT_FROM_BE(val)	(GUINT_TO_BE (val))

#endif
