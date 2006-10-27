#ifndef AL_AL_SITECONFIG_H_
#define AL_AL_SITECONFIG_H_

#include "config.h"

#if HAVE_STDINT_H
#include <stdint.h>
#elif HAVE___INT8
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
#else
#error Do not know sized types on this platform
#endif

#if HAVE_SNPRINTF
#elif HAVE__SNPRINTF
#define snprintf _snprintf
#else
#error No snprintf-like function on this platform
#endif

#ifdef HAVE___ATTRIBUTE__
#define UNUSED(x) x __attribute__((unused))
#define AL_ATTRIBUTE_FORMAT_PRINTF_(x,y) __attribute__((format(printf, x, y)))
#define AL_ATTRIBUTE_NORETURN_ __attribute__((noreturn))
#else /* not HAVE___ATTRIBUTE__ */
#define UNUSED(x) x
#define AL_ATTRIBUTE_FORMAT_PRINTF_(x,y)
#define AL_ATTRIBUTE_NORETURN_
#endif /* not HAVE___ATTRIBUTE__ */

#ifdef _WIN32
#define CDECL __cdecl
#else /* _WIN32 */
#define CDECL
#endif /* _WIN32 */

#define USE_LRINT 0 /* icculus look here JIV FIXME */

#if USE_LRINT

#define __USE_ISOC99 1
#define _ISOC99_SOURCE 1
#define __USE_EXTERN_INLINES 1
#define __FAST_MATH__ 1
#include <math.h>

#endif /* USE_LRINT */

#endif /* not AL_SITE_CONFIG_H_ */
