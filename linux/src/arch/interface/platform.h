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

/*
 * grab_write_native( void )
 *
 * Returns handle to an audio device suitable for writing data to, or NULL if
 * no such device is available.
 */
void *grab_write_native( void );

/*
 * grab_read_native( void )
 *
 * Returns handle to an audio device suitable for reading data from, or NULL if
 * no such device is available.
 */
void *grab_read_native( void );

/*
 * release_native( void *handle )
 *
 * Releases an audio device aquired using grab_foo_native.  Returns
 * AL_TRUE if release was successful, AL_FALSE if handle was invalid or the
 * device could not be released for some reason.
 */
void release_native( void *handle );

/*
 * set_read_native ( void *handle, ALuint *bufsiz,
 *                       ALenum *fmt, ALuint *speed )
 *
 * Sets the parameters associated with the device specified by handle.
 * Because we follow a meet-or-exceed policty, *bufsiz, *fmt, and *speed might be
 * different from the parameters initially passed, so the caller should check
 * these after a succesfull call.
 *
 * Returns AL_FALSE if the parameters could not be matched or exceeded.
 */
ALboolean set_read_native( void *handle,
		     ALuint *bufsiz,
		     ALenum *fmt,
		     ALuint *speed );

/*
 * set_write_native( void *handle, ALuint *bufsiz,
 *                        ALenum *fmt, ALuint *speed )
 *
 * Sets the parameters associated with the device specified by handle.
 * Because we follow a meet-or-exceed policty, *bufsiz, *fmt, and *speed might be
 * different from the parameters initially passed, so the caller should check
 * these after a succesfull call.
 *
 * Returns AL_FALSE if the parameters could not be matched or exceeded.
 */
ALboolean set_write_native( void *handle,
			    ALuint *bufsiz,
			    ALenum *fmt,
			    ALuint *speed );

/*
 * native_blitbuffer( void *handle, void *data, int bytes )
 *
 * Writes bytes worth of data from data to the device specified by handle.
 * dataptr is an interleaved array.
 */
void native_blitbuffer( void *handle, void *data, int bytes );

/*
 * get_nativechannel( void *handle, ALuint channel )
 *
 * Returns normalized audio setting for handle at channel.
 */
float get_nativechannel( void *handle, ALCenum channel );

/*
 * set_nativechannel( void *handle, ALuint channel, float volume )
 *
 * Sets normalized audio setting for handle at channel.
 */
int set_nativechannel( void *handle, ALCenum channel, float volume );

/*
 * pause_nativedevice( void *handle )
 *
 * Informs device specified by handle that it's about to get paused.
 */
void pause_nativedevice( void *handle );

/*
 * resume_nativedevice( void *handle )
 *
 * Informs device specified by handle that it's about to get unpaused.
 */
void resume_nativedevice( void *handle );

/*
 * capture_nativedevice( void *handle, void *capture_buffer, int bufsiz )
 *
 * capture data from the audio device specified by handle, into
 * capture_buffer, which is bufsiz long.
 */
ALsizei capture_nativedevice( void *handle, void *capture_buffer, int bufsiz );

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
