#ifndef AL_SITECONFIG_H_
#define AL_SITECONFIG_H_

/*
 * Wrap site specific config stuff
 */

#ifdef DARWIN_PBBUILDER
#include "config-osx.h"
#else
#include "config.h"
#endif /* DARWIN_PBBUILDER */

#if HAVE___ATTRIBUTE__
#define UNUSED(x) x __attribute__((unused))
#define AL_ATTRIBUTE_FORMAT_PRINTF_(x,y) __attribute__((format(printf, x, y)))
#define AL_ATTRIBUTE_NORETURN_ __attribute__((noreturn))
#else
#define UNUSED(x) x
#define AL_ATTRIBUTE_FORMAT_PRINTF_(x,y)
#define AL_ATTRIBUTE_NORETURN_
#endif

#define USE_LRINT 0 /* icculus look here JIV FIXME */

#if USE_LRINT

#define __USE_ISOC99 1
#define _ISOC99_SOURCE 1
#define __USE_EXTERN_INLINES 1
#define __FAST_MATH__ 1
#include <math.h>

#endif

#endif /* AL_SITE_CONFIG_H_ */
