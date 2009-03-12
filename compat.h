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

#ifdef __APPLE__
#include <machine/endian.h>
#else
#include <endian.h>
#endif

#ifndef NULL
#define NULL 0
#endif

#define G_BYTE_ORDER BYTE_ORDER

#define GUINT16_SWAP_LE_BE_CONSTANT(val)        ((uint16_t) ( \
    (uint16_t) ((uint16_t) (val) >> 8) |  \
    (uint16_t) ((uint16_t) (val) << 8)))

#define GUINT32_SWAP_LE_BE_CONSTANT(val)        ((uint32_t) ( \
    (((uint32_t) (val) & (uint32_t) 0x000000ffU) << 24) | \
    (((uint32_t) (val) & (uint32_t) 0x0000ff00U) <<  8) | \
    (((uint32_t) (val) & (uint32_t) 0x00ff0000U) >>  8) | \
    (((uint32_t) (val) & (uint32_t) 0xff000000U) >> 24)))

#if BYTE_ORDER == LITTLE_ENDIAN

#define GINT16_TO_LE(val)	((int16_t) (val))
#define GUINT16_TO_LE(val)	((uint16_t) (val))
#define GINT16_TO_BE(val)       ((int16_t) GUINT16_SWAP_LE_BE (val))
#define GUINT16_TO_BE(val)      (GUINT16_SWAP_LE_BE (val))

#define GINT32_TO_LE(val)       ((int32_t) (val))
#define GUINT32_TO_LE(val)      ((uint32_t) (val))
#define GINT32_TO_BE(val)       ((int32_t) GUINT32_SWAP_LE_BE (val))
#define GUINT32_TO_BE(val)      (GUINT32_SWAP_LE_BE (val))

#else

#define GINT16_TO_LE(val)	((int16_t) GUINT16_SWAP_LE_BE (val))
#define GUINT16_TO_LE(val)	(GUINT16_SWAP_LE_BE (val))
#define GINT16_TO_BE(val)       ((int16_t) (val))
#define GUINT16_TO_BE(val)      ((uint16_t) (val))

#define GINT32_TO_LE(val)       ((int32_t) GUINT32_SWAP_LE_BE (val))
#define GUINT32_TO_LE(val)      (GUINT32_SWAP_LE_BE (val))
#define GINT32_TO_BE(val)       ((int32_t) (val))
#define GUINT32_TO_BE(val)      ((uint32_t) (val))


#endif

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
     /* Possibly just use the constant version and let gcc figure it out? */
#    define GUINT16_SWAP_LE_BE(val) (GUINT16_SWAP_LE_BE_IA32 (val))
#    define GUINT32_SWAP_LE_BE(val) (GUINT32_SWAP_LE_BE_IA32 (val))
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
#    define GUINT16_SWAP_LE_BE(val) (GUINT16_SWAP_LE_BE_IA64 (val))
#    define GUINT32_SWAP_LE_BE(val) (GUINT32_SWAP_LE_BE_IA64 (val))
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
     /* gcc seems to figure out optimal code for this on its own */
#    define GUINT16_SWAP_LE_BE(val) (GUINT16_SWAP_LE_BE_CONSTANT (val))
#    define GUINT32_SWAP_LE_BE(val) (GUINT32_SWAP_LE_BE_X86_64 (val))
#  else /* generic gcc */
#    define GUINT16_SWAP_LE_BE(val) (GUINT16_SWAP_LE_BE_CONSTANT (val))
#    define GUINT32_SWAP_LE_BE(val) (GUINT32_SWAP_LE_BE_CONSTANT (val))
#  endif
#else /* generic */
#  define GUINT16_SWAP_LE_BE(val) (GUINT16_SWAP_LE_BE_CONSTANT (val))
#  define GUINT32_SWAP_LE_BE(val) (GUINT32_SWAP_LE_BE_CONSTANT (val))
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

#endif
