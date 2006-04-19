/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_debug.h
 *
 * Prototypes, macros and definitions related to the printing debugging
 * messages.
 *
 * The debugging facility relies heavily on defines provided by
 * al_siteconfig.h.  One future optimization could rely on the varadic macro
 * facilities in ISO99 C.
 *
 */
#ifndef AL_AL_DEBUG_H_
#define AL_AL_DEBUG_H_

#include "al_siteconfig.h"
#include <assert.h>

#if defined(DEBUG)        || defined(DEBUG_LOOP) || defined(DEBUG_STUB)    || \
    defined(DEBUG_CONFIG) || defined(DEBUG_LOCK) || defined(DEBUG_CONVERT) || \
    defined(DEBUG_EXT)    || defined(DEBUG_SOURCE) || defined(DEBUG_STREAMING)\
    || defined(DEBUG_MATH) || defined(DEBUG_MEM) || defined(DEBUG_CONTEXT)    \
    || defined(DEBUG_MAXIMUS) || defined(DEBUG_BUFFER)                        \
    || defined(DEBUG_LISTENER) || defined(DEBUG_QUEUE) || defined(DEBUG_FILTER)

#define NEED_DEBUG
#endif /* debug stuff */

#ifdef DEBUG_MAXIMUS
#define DEBUG
#endif /* DEBUG_MAXIMUS */

/*
 * aldEnum is an enum which enumerates each type of debug type.  There isn't
 * any rule to which message should be associated with which debug type, but
 * the general idea is that if the user configures openal with
 * --enable-debug-foo, only debug messages relavant to foo should be printed.
 */
typedef enum _aldEnum
{
	ALD_INVALID,
	ALD_CONVERT,
	ALD_CONFIG,
	ALD_SOURCE,
	ALD_LOOP,
	ALD_STUB,
	ALD_CONTEXT,
	ALD_MATH,
	ALD_MIXER,
	ALD_ERROR,
	ALD_EXT,
	ALD_LOCK,
	ALD_STREAMING,
	ALD_MEM,
	ALD_MAXIMUS,
	ALD_BUFFER,
	ALD_LISTENER,
	ALD_QUEUE,
	ALD_FILTER,
	ALD_MAXMAXMAX = 0x7fffffff
} aldEnum;

/*
 * If debugging messages for the type level are enabled, print the debugging
 * message specified by format, ... ( printf format ).  Otherwise, return.
 */
#ifdef NEED_DEBUG
AL_ATTRIBUTE_FORMAT_PRINTF_(4,5)
int _alDebug( aldEnum level, const char *fn, int ln, const char *format, ... );
#else /* not NEED_DEBUG */
AL_ATTRIBUTE_FORMAT_PRINTF_(4,5) static __inline
int _alDebug( aldEnum level, const char *fn, int ln, const char *format, ... )
{
	(void)level; (void)fn; (void)ln; (void)format;
	return 0;
}
#endif /* not NEED_DEBUG */

#endif /* not AL_AL_DEBUG_H_ */
