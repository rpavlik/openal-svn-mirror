/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * platform.h
 *
 * Defines include for platform specific backends.  Support for backends is
 * determined at configure time, when defines are set in such a way that the
 * specific backend headers are included.
 */
#ifndef PLATFORM_H_
#define PLATFORM_H_

/* native audio support */
#if USE_BACKEND_NATIVE_LINUX
#include "arch/linux/lin_dsp.h"
#elif USE_BACKEND_NATIVE_BSD
#include "arch/bsd/bsd_dsp.h"
#elif USE_BACKEND_NATIVE_SOLARIS
#include "arch/solaris/solaris_native.h"
#elif USE_BACKEND_NATIVE_IRIX
#include "arch/irix/iris.h"
#elif USE_BACKEND_NATIVE_DARWIN
#include "arch/darwin/darwin_native.h"
#elif USE_BACKEND_NATIVE_MORPHOS
#include "arch/morphos/morphos_native.h"
#elif USE_BACKEND_NATIVE_WINDOWS
#include "arch/windows/windows_native.h"
#endif

/* Here's the hacky stuff */
#ifdef USE_BACKEND_SDL
#include "arch/sdl/sdl.h"
#else
#define grab_read_sdl()               NULL
#define grab_write_sdl()              NULL
#define set_write_sdl(h,b,f,s)        AL_FALSE
#define set_read_sdl(h,b,f,s)         AL_FALSE
#define release_sdl(h)
#define sdl_blitbuffer(h,d,b)
#endif /* USE_BACKEND_SDL */

#ifdef USE_BACKEND_ALSA
#include "arch/alsa/alsa.h"
#else
#define grab_read_alsa()	      NULL
#define grab_write_alsa()	      NULL
#define set_write_alsa(h,b,f,s)       AL_FALSE
#define set_read_alsa(h,b,f,s)        AL_FALSE
#define release_alsa(h)
#define alsa_blitbuffer(h,d,b)
#define capture_alsa(h,d,b)           0
#endif /* USE_BACKEND_ALSA */

#ifdef USE_BACKEND_ARTS
#include "arch/arts/arts.h"
#else
#define grab_read_arts()	      NULL
#define grab_write_arts()	      NULL
#define set_write_arts(h,b,f,s)       AL_FALSE
#define set_read_arts(h,b,f,s)        AL_FALSE
#define release_arts(h)
#define arts_blitbuffer(h,d,b)
#endif /* USE_BACKEND_ARTS */

#ifdef USE_BACKEND_ESD
#include "arch/esd/esd.h"
#else
#define grab_read_esd()	              NULL
#define grab_write_esd()	      NULL
#define set_read_esd(h,b,f,s)         AL_FALSE
#define set_write_esd(h,b,f,s)        AL_FALSE
#define release_esd(h)
#define esd_blitbuffer(h,d,b)
#define pause_esd(h)
#define resume_esd(h)
#endif /* USE_BACKEND_ESD */

#ifdef USE_BACKEND_WAVEOUT
#include "arch/waveout/waveout.h"
#else
#define grab_read_waveout()	      NULL
#define grab_write_waveout()	      NULL
#define set_read_waveout(h,b,f,s)     AL_FALSE
#define set_write_waveout(h,b,f,s)    AL_FALSE
#define release_waveout(h)
#define waveout_blitbuffer(h,d,b)
#endif /* USE_BACKEND_WAVEOUT */

#ifdef USE_BACKEND_NULL
#include "arch/null/null.h"
#else
#define grab_read_null()	      NULL
#define grab_write_null()	      NULL
#define set_read_null(h,b,f,s)        AL_FALSE
#define set_write_null(h,b,f,s)       AL_FALSE
#define release_null(h)
#define null_blitbuffer(h,d,b)
#endif /* USE_BACKEND_NULL */

#endif /* PLATFORM_H_ */
