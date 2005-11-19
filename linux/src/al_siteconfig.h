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


#define USE_LRINT 0 /* icculus look here JIV FIXME */

#if USE_LRINT

#define __USE_ISOC99 1
#define _ISOC99_SOURCE 1
#define __USE_EXTERN_INLINES 1
#define __FAST_MATH__ 1
#include <math.h>

#endif

#ifdef BROKEN_LIBIO
#include <libio.h>
#define __underflow __broken_underflow
#define __overflow __broken_overflow
#include <stdio.h>

#define __ASSEMBLER__
#include <errnos.h>

#define __USE_POSIX
#include <signal.h>

#endif /* BROKEN_LIBIO */

#endif /* AL_SITE_CONFIG_H_ */
