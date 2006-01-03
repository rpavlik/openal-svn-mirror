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

/*
 * Returns a handle to a backend suitable for writing data to, or NULL if no
 * such backend is available. This function is used to implement alcOpenDevice.
 */
void *_alcBackendOpenOutput( void );

/*
 * Returns a handle to a backend suitable for reading data from, or NULL if no
 * such backend is available. This function is used to implement alcOpenDevice.
 */
void *_alcBackendOpenInput( void );

/*
 * Closes an (output or input) backend. Returns AL_TRUE if closing was
 * successful, AL_FALSE if the handle was invalid or the backend could not be
 * closed for some reason. This function is used to implement alcCloseDevice.
 */
ALboolean _alcBackendClose( void *handle );

/*
 * Informs an output backend that it is about to get paused. This function is
 * used to implement alcMakeContextCurrent(NULL).
 */
void _alcBackendPause( void *handle );

/*
 * Informs an output backend that it is about to get resumed. This function is
 * used to implement alcMakeContextCurrent(NON_NULL).
 */
void _alcBackendResume( void *handle );

/*
 * Sets the parameters for an output backend. Because we follow a meet-or-exceed
 * policty, *bufsiz, *fmt, and *speed might be different from the parameters
 * initially passed, so the caller should check these after a succesful
 * call. Returns AL_TRUE if setting was successful, AL_FALSE if the parameters
 * could not be matched or exceeded. This function is used to implement
 * alcMakeContextCurrent(NON_NULL).
 */
ALboolean _alcBackendSetWrite( void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed );

/*
 * Sets the parameters for an input backend. Because we follow a meet-or-exceed
 * policty, *bufsiz, *fmt, and *speed might be different from the parameters
 * initially passed, so the caller should check these after a succesfull
 * call. Returns AL_TRUE if setting was succesful, AL_FALSE if the parameters
 * could not be matched or exceeded. This function is used to implement
 * alcMakeContextCurrent(NON_NULL) and alCaptureInit_EXT.
 */
ALboolean _alcBackendSetRead( void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed );

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

void *grab_write_native( void );
void *grab_read_native( void );
void release_native( void *handle );
void pause_nativedevice( void *handle );
void resume_nativedevice( void *handle );
ALboolean set_write_native( void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed );
ALboolean set_read_native( void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed );
void native_blitbuffer( void *handle, void *data, int bytes );
ALsizei capture_nativedevice( void *handle, void *capture_buffer, int bufsiz );
ALfloat get_nativechannel( void *handle, ALuint channel );
int set_nativechannel( void *handle, ALuint channel, ALfloat volume );

#include "al_siteconfig.h"

#ifdef USE_BACKEND_ALSA
void *grab_write_alsa( void );
void *grab_read_alsa( void );
void *release_alsa( void *handle );
void pause_alsa( void *handle );
void resume_alsa( void *handle );
ALboolean set_read_alsa( void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed);
ALboolean set_write_alsa( void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed);
void alsa_blitbuffer( void *handle, void *data, int bytes );
ALsizei capture_alsa(void *handle, void *capture_buffer, int bufsiz);
ALfloat get_alsachannel( void *handle, ALuint channel );
int set_alsachannel( void *handle, ALuint channel, ALfloat volume );
#else
#define grab_write_alsa()             NULL
#define grab_read_alsa()              NULL
#define release_alsa(h)
#define pause_alsa(h)
#define resume_alsa(h)
#define set_write_alsa(h,b,f,s)       AL_FALSE
#define set_read_alsa(h,b,f,s)        AL_FALSE
#define alsa_blitbuffer(h,d,b)
#define capture_alsa(h,d,b)           0
#define get_alsachannel(h,c)          0.0
#define set_alsachannel(h,c,v)        0
#endif /* USE_BACKEND_ALSA */

#ifdef USE_BACKEND_ARTS
void *grab_write_arts(void);
void *grab_read_arts(void);
void release_arts(void *handle);
void pause_arts( void *handle );
void resume_arts( void *handle );
ALboolean set_write_arts(void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed);
ALboolean set_read_arts(void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed);
void arts_blitbuffer(void *handle, void *data, int bytes);
ALsizei capture_arts( void *handle, void *capture_buffer, int bufsiz );
ALfloat get_artschannel( void *handle, ALuint channel );
int set_artschannel( void *handle, ALuint channel, ALfloat volume );
#else
#define grab_write_arts()             NULL
#define grab_read_arts()              NULL
#define release_arts(h)
#define pause_arts(h)
#define resume_arts(h)
#define set_write_arts(h,b,f,s)       AL_FALSE
#define set_read_arts(h,b,f,s)        AL_FALSE
#define arts_blitbuffer(h,d,b)
#define capture_arts(h,d,b)           0
#define get_artschannel(h,c)          0.0
#define set_artschannel(h,c,v)        0
#endif /* USE_BACKEND_ARTS */

#ifdef USE_BACKEND_ESD
void *grab_write_esd(void);
void *grab_read_esd(void);
void release_esd(void *handle);
void pause_esd(void *handle);
void resume_esd(void *handle);
ALboolean set_write_esd(void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed);
ALboolean set_read_esd(void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed);
void esd_blitbuffer(void *handle, void *data, int bytes);
ALsizei capture_esd(void *handle, void *capture_buffer, int bufsiz);
ALfloat get_esdchannel( void *handle, ALuint channel );
int set_esdchannel( void *handle, ALuint channel, ALfloat volume );
#else
#define grab_write_esd()              NULL
#define grab_read_esd()               NULL
#define pause_esd(h)
#define release_esd(h)
#define resume_esd(h)
#define set_write_esd(h,b,f,s)        AL_FALSE
#define set_read_esd(h,b,f,s)         AL_FALSE
#define esd_blitbuffer(h,d,b)
#define capture_esd(h,d,b)            0
#define get_esdchannel(h,c)           0.0
#define set_esdchannel(h,c,v)         0
#endif /* USE_BACKEND_ESD */

#ifdef USE_BACKEND_SDL
void *grab_write_sdl(void);
void *grab_read_sdl(void);
void release_sdl(void *handle);
void pause_sdl( void *handle );
void resume_sdl( void *handle );
ALboolean set_write_sdl(void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed);
ALboolean set_read_sdl(void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed);
void sdl_blitbuffer(void *handle, void *data, int bytes);
ALsizei capture_sdl(void *handle, void *capture_buffer, int bufsiz);
ALfloat get_sdlchannel( void *handle, ALuint channel );
int set_sdlchannel( void *handle, ALuint channel, ALfloat volume );
#else
#define grab_write_sdl()              NULL
#define grab_read_sdl()               NULL
#define release_sdl(h)
#define pause_sdl(h)
#define resume_sdl(h)
#define set_write_sdl(h,b,f,s)        AL_FALSE
#define set_read_sdl(h,b,f,s)         AL_FALSE
#define sdl_blitbuffer(h,d,b)
#define capture_sdl(h,d,b)            0
#define get_sdlchannel(h,c)           0.0
#define set_sdlchannel(h,c,v)         0
#endif /* USE_BACKEND_SDL */

#ifdef USE_BACKEND_NULL
void *grab_write_null(void);
void *grab_read_null(void);
void release_null(void *handle);
void pause_null( void *handle );
void resume_null(void *handle);
ALboolean set_write_null(void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed);
ALboolean set_read_null(void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed);
void null_blitbuffer(void *handle, void *data, int bytes);
ALsizei capture_null(void *handle, void *capture_buffer, int bufsiz);
ALfloat get_nullchannel( void *handle, ALuint channel );
int set_nullchannel( void *handle, ALuint channel, ALfloat volume );
#else
#define grab_write_null()             NULL
#define grab_read_null()              NULL
#define release_null(h)
#define pause_null(h)
#define resume_null(h)
#define set_write_null(h,b,f,s)       AL_FALSE
#define set_read_null(h,b,f,s)        AL_FALSE
#define null_blitbuffer(h,d,b)
#define capture_null(h,d,b)           0
#define get_nullchannel(h,c)          0.0
#define set_nullchannel(h,c,v)        0
#endif /* USE_BACKEND_NULL */

#ifdef USE_BACKEND_WAVEOUT
void *grab_write_waveout(void);
void *grab_read_waveout(void);
void release_waveout(void *handle);
void pause_waveout( void *handle );
void resume_waveout( void *handle );
ALboolean set_write_waveout(void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed);
ALboolean set_read_waveout(void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed);
void waveout_blitbuffer(void *handle, void *data, int bytes);
ALsizei capture_waveout(void *handle, void *capture_buffer, int bufsiz);
ALfloat get_waveoutchannel( void *handle, ALuint channel );
int set_waveoutchannel( void *handle, ALuint channel, ALfloat volume );
#else
#define grab_write_waveout()          NULL
#define grab_read_waveout()           NULL
#define release_waveout(h)
#define pause_waveout(h)
#define resume_waveout(h)
#define set_read_waveout(h,b,f,s)     AL_FALSE
#define set_write_waveout(h,b,f,s)    AL_FALSE
#define waveout_blitbuffer(h,d,b)
#define capture_waveout(h,d,b)        0
#define get_waveoutchannel(h,c)       0.0
#define set_waveoutchannel(h,c,v)     0
#endif /* USE_BACKEND_WAVEOUT */

#endif /* INTERFACE_SOUND_H_ */
