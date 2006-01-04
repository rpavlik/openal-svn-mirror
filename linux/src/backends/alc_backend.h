/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * interface_sound.h
 *
 * High level prototypes for sound device aquisition and management.
 *
 */
#ifndef INTERFACE_SOUND_H_
#define INTERFACE_SOUND_H_

#include <AL/al.h>

typedef enum {
	_ALC_OPEN_INPUT,
	_ALC_OPEN_OUTPUT
} _ALCOpenMode;

/*
 * Returns a handle to a backend suitable for reading or writing sound data, or
 * NULL if no such backend is available. This function is used to implement
 * alcOpenDevice.
 */
void *_alcBackendOpen( _ALCOpenMode mode );

/*
 * Closes an (output or input) backend. Returns AL_TRUE if closing was
 * successful, AL_FALSE if the handle was invalid or the backend could not be
 * closed for some reason. This function is used to implement alcCloseDevice.
 */
ALboolean _alcBackendClose( void *handle );

/*
 * Informs a backend that it is about to get paused. This function is used to
 * implement alcMakeContextCurrent(NULL).
 */
void _alcBackendPause( void *handle );

/*
 * Informs a backend that it is about to get resumed. This function is used to
 * implement alcMakeContextCurrent(NON_NULL).
 */
void _alcBackendResume( void *handle );

/*
 * Sets the parameters for an input/output backend (depending on mode). Because
 * we follow a meet-or-exceed policty, *bufsiz, *fmt, and *speed might be
 * different from the parameters initially passed, so the caller should check
 * these after a succesful call. Returns AL_TRUE if setting was successful,
 * AL_FALSE if the parameters could not be matched or exceeded. This function is
 * used to implement alcMakeContextCurrent(NON_NULL).
 */
ALboolean _alcBackendSetAttributes( _ALCOpenMode mode, void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed );

/*
 * Writes a given interleaved array of sound data to an output backend. This
 * function is used to implement (a)sync_mixer_iterate.
 */
void _alcBackendWrite( void *handle, void *data, int size );

/*
 * Captures data from an input backend into the given buffer. This function is
 * used to implement alCaptureGetData_EXT.
 */
ALsizei _alcBackendRead( void *handle, void *data, int size );

/*
 * Returns the normalized volume for the given channel (main/PCM/CD) on an
 * output backend. This function is used to implement alcGetAudioChannel_LOKI.
 */
ALfloat _alcBackendGetAudioChannel( void *handle, ALuint channel );

/*
 * Sets the normalized volume for the given channel (main/PCM/CD) on an output
 * backend. This function is used to implement alcSetAudioChannel_LOKI.
 */
void _alcBackendSetAudioChannel( void *handle, ALuint channel, ALfloat volume );

/******************************************************************************/

void *_alcBackendOpenNative( _ALCOpenMode mode );
void release_native( void *handle );
void pause_nativedevice( void *handle );
void resume_nativedevice( void *handle );
ALboolean _alcBackendSetAttributesNative( _ALCOpenMode mode, void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed );
void native_blitbuffer( void *handle, void *data, int bytes );
ALsizei capture_nativedevice( void *handle, void *capture_buffer, int bufsiz );
ALfloat get_nativechannel( void *handle, ALuint channel );
int set_nativechannel( void *handle, ALuint channel, ALfloat volume );

#include "al_siteconfig.h"

#ifdef USE_BACKEND_ALSA
void *_alcBackendOpenALSA( _ALCOpenMode mode );
void *release_alsa( void *handle );
void pause_alsa( void *handle );
void resume_alsa( void *handle );
ALboolean _alcBackendSetAttributesALSA( _ALCOpenMode mode, void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed);
void alsa_blitbuffer( void *handle, void *data, int bytes );
ALsizei capture_alsa(void *handle, void *capture_buffer, int bufsiz);
ALfloat get_alsachannel( void *handle, ALuint channel );
int set_alsachannel( void *handle, ALuint channel, ALfloat volume );
#else
#define _alcBackendOpenALSA(m)        NULL
#define release_alsa(h)
#define pause_alsa(h)
#define resume_alsa(h)
#define _alcBackendSetAttributesALSA(m,h,b,f,s) AL_FALSE
#define alsa_blitbuffer(h,d,b)
#define capture_alsa(h,d,b)           0
#define get_alsachannel(h,c)          0.0
#define set_alsachannel(h,c,v)        0
#endif /* USE_BACKEND_ALSA */

#ifdef USE_BACKEND_ARTS
void *_alcBackendOpenARts( _ALCOpenMode mode );
void release_arts(void *handle);
void pause_arts( void *handle );
void resume_arts( void *handle );
ALboolean _alcBackendSetAttributesARts(_ALCOpenMode mode, void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed);
void arts_blitbuffer(void *handle, void *data, int bytes);
ALsizei capture_arts( void *handle, void *capture_buffer, int bufsiz );
ALfloat get_artschannel( void *handle, ALuint channel );
int set_artschannel( void *handle, ALuint channel, ALfloat volume );
#else
#define _alcBackendOpenARts(m)        NULL
#define release_arts(h)
#define pause_arts(h)
#define resume_arts(h)
#define _alcBackendSetAttributesARts(m,h,b,f,s) AL_FALSE
#define arts_blitbuffer(h,d,b)
#define capture_arts(h,d,b)           0
#define get_artschannel(h,c)          0.0
#define set_artschannel(h,c,v)        0
#endif /* USE_BACKEND_ARTS */

#ifdef USE_BACKEND_ESD
void *_alcBackendOpenESD( _ALCOpenMode mode );
void release_esd(void *handle);
void pause_esd(void *handle);
void resume_esd(void *handle);
ALboolean _alcBackendSetAttributesESD(_ALCOpenMode mode, void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed);
void esd_blitbuffer(void *handle, void *data, int bytes);
ALsizei capture_esd(void *handle, void *capture_buffer, int bufsiz);
ALfloat get_esdchannel( void *handle, ALuint channel );
int set_esdchannel( void *handle, ALuint channel, ALfloat volume );
#else
#define _alcBackendOpenESD(m)         NULL
#define pause_esd(h)
#define release_esd(h)
#define resume_esd(h)
#define _alcBackendSetAttributesESD(m,h,b,f,s) AL_FALSE
#define esd_blitbuffer(h,d,b)
#define capture_esd(h,d,b)            0
#define get_esdchannel(h,c)           0.0
#define set_esdchannel(h,c,v)         0
#endif /* USE_BACKEND_ESD */

#ifdef USE_BACKEND_SDL
void *_alcBackendOpenSDL( _ALCOpenMode mode );
void release_sdl(void *handle);
void pause_sdl( void *handle );
void resume_sdl( void *handle );
ALboolean _alcBackendSetAttributesSDL(_ALCOpenMode mode, void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed);
void sdl_blitbuffer(void *handle, void *data, int bytes);
ALsizei capture_sdl(void *handle, void *capture_buffer, int bufsiz);
ALfloat get_sdlchannel( void *handle, ALuint channel );
int set_sdlchannel( void *handle, ALuint channel, ALfloat volume );
#else
#define _alcBackendOpenSDL(m)         NULL
#define release_sdl(h)
#define pause_sdl(h)
#define resume_sdl(h)
#define _alcBackendSetAttributesSDL(m,h,b,f,s) AL_FALSE
#define sdl_blitbuffer(h,d,b)
#define capture_sdl(h,d,b)            0
#define get_sdlchannel(h,c)           0.0
#define set_sdlchannel(h,c,v)         0
#endif /* USE_BACKEND_SDL */

#ifdef USE_BACKEND_NULL
void *_alcBackendOpenNull( _ALCOpenMode mode );
void release_null(void *handle);
void pause_null( void *handle );
void resume_null(void *handle);
ALboolean _alcBackendSetAttributesNull(_ALCOpenMode mode, void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed);
void null_blitbuffer(void *handle, void *data, int bytes);
ALsizei capture_null(void *handle, void *capture_buffer, int bufsiz);
ALfloat get_nullchannel( void *handle, ALuint channel );
int set_nullchannel( void *handle, ALuint channel, ALfloat volume );
#else
#define _alcBackendOpenNull(m)        NULL
#define release_null(h)
#define pause_null(h)
#define resume_null(h)
#define _alcBackendSetAttributesNull(m,h,b,f,s) AL_FALSE
#define null_blitbuffer(h,d,b)
#define capture_null(h,d,b)           0
#define get_nullchannel(h,c)          0.0
#define set_nullchannel(h,c,v)        0
#endif /* USE_BACKEND_NULL */

#ifdef USE_BACKEND_WAVEOUT
void *_alcBackendOpenWAVE( _ALCOpenMode mode );
void release_waveout(void *handle);
void pause_waveout( void *handle );
void resume_waveout( void *handle );
ALboolean _alcBackendSetAttributesWAVE(_ALCOpenMode mode, void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed);
void waveout_blitbuffer(void *handle, void *data, int bytes);
ALsizei capture_waveout(void *handle, void *capture_buffer, int bufsiz);
ALfloat get_waveoutchannel( void *handle, ALuint channel );
int set_waveoutchannel( void *handle, ALuint channel, ALfloat volume );
#else
#define _alcBackendOpenWAVE(m)        NULL
#define release_waveout(h)
#define pause_waveout(h)
#define resume_waveout(h)
#define _alcBackendSetAttributesWAVE(m,h,b,f,s) AL_FALSE
#define waveout_blitbuffer(h,d,b)
#define capture_waveout(h,d,b)        0
#define get_waveoutchannel(h,c)       0.0
#define set_waveoutchannel(h,c,v)     0
#endif /* USE_BACKEND_WAVEOUT */

#endif /* INTERFACE_SOUND_H_ */
