/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * interface_sound.h
 *
 * High level prototypes for sound device aquisition and management.
 *
 */
#ifndef AL_BACKENDS_ALC_BACKEND_H_
#define AL_BACKENDS_ALC_BACKEND_H_

#include "al_siteconfig.h"

#include <AL/al.h>

/*
 * An opaque backend structure, only used via pointers from outside.
 */
typedef struct ALC_BackendStruct ALC_Backend;

typedef enum
{
  ALC_OPEN_INPUT_,
  ALC_OPEN_OUTPUT_
} ALC_OpenMode;

/*
 * Returns a handle to a backend suitable for reading or writing sound data, or
 * NULL if no such backend is available. This function is used to implement
 * alcOpenDevice.
 */
ALC_Backend *alcBackendOpen_ (ALC_OpenMode mode);

/*
 * Closes an (output or input) backend. Returns AL_TRUE if closing was
 * successful, AL_FALSE if the handle was invalid or the backend could not be
 * closed for some reason. This function is used to implement alcCloseDevice.
 */
ALboolean alcBackendClose_ (ALC_Backend *backend);

/*
 * Informs a backend that it is about to get paused. This function is used to
 * implement alcMakeContextCurrent(NULL).
 */
void alcBackendPause_ (ALC_Backend *backend);

/*
 * Informs a backend that it is about to get resumed. This function is used to
 * implement alcMakeContextCurrent(NON_NULL).
 */
void alcBackendResume_ (ALC_Backend *backend);

/*
 * Sets the parameters for a backend. Because we follow a meet-or-exceed
 * policty, *bufsiz, *fmt, and *speed might be different from the parameters
 * initially passed, so the caller should check these after a succesful
 * call. Returns AL_TRUE if setting was successful, AL_FALSE if the parameters
 * could not be matched or exceeded. This function is used to implement
 * alcMakeContextCurrent(NON_NULL).
 */
ALboolean alcBackendSetAttributes_ (ALC_Backend *backend,
                                    ALuint *bufsiz, ALenum *fmt,
                                    ALuint *speed);

/*
 * Writes a given interleaved array of sound data to an output backend. This
 * function is used to implement (a)sync_mixer_iterate.
 */
void alcBackendWrite_ (ALC_Backend *backend, void *data, int size);

/*
 * Captures data from an input backend into the given buffer. This function is
 * used to implement alCaptureGetData_EXT.
 */
ALsizei alcBackendRead_ (ALC_Backend *backend, void *data, int size);

/*
 * Returns the normalized volume for the given channel (main/PCM/CD) on an
 * output backend. This function is used to implement alcGetAudioChannel_LOKI.
 */
ALfloat alcBackendGetAudioChannel_ (ALC_Backend *backend, ALuint channel);

/*
 * Sets the normalized volume for the given channel (main/PCM/CD) on an output
 * backend. This function is used to implement alcSetAudioChannel_LOKI.
 */
void alcBackendSetAudioChannel_ (ALC_Backend *backend, ALuint channel,
                                 ALfloat volume);

/******************************************************************************/

/*
 * Various data, the details depend on the backend in question.
 */
typedef void ALC_BackendPrivateData;

ALC_BackendPrivateData *alcBackendOpenNative_ (ALC_OpenMode mode);
void release_native (ALC_BackendPrivateData *privateData);
void pause_nativedevice (ALC_BackendPrivateData *privateData);
void resume_nativedevice (ALC_BackendPrivateData *privateData);
ALboolean alcBackendSetAttributesNative_ (ALC_OpenMode mode,
                                          ALC_BackendPrivateData *privateData,
                                          ALuint *bufsiz, ALenum *fmt,
                                          ALuint *speed);
void native_blitbuffer (ALC_BackendPrivateData *privateData, void *data,
                        int bytes);
ALsizei capture_nativedevice (ALC_BackendPrivateData *privateData,
                              void *capture_buffer, int bufsiz);
ALfloat get_nativechannel (ALC_BackendPrivateData *privateData,
                           ALuint channel);
int set_nativechannel (ALC_BackendPrivateData *privateData, ALuint channel,
                       ALfloat volume);

#ifdef USE_BACKEND_ALSA
ALC_BackendPrivateData *alcBackendOpenALSA_ (ALC_OpenMode mode);
void release_alsa (ALC_BackendPrivateData *privateData);
void pause_alsa (ALC_BackendPrivateData *privateData);
void resume_alsa (ALC_BackendPrivateData *privateData);
ALboolean alcBackendSetAttributesALSA_ (ALC_OpenMode mode,
                                        ALC_BackendPrivateData *privateData,
                                        ALuint *bufsiz, ALenum *fmt,
                                        ALuint *speed);
void alsa_blitbuffer (ALC_BackendPrivateData *privateData, void *data,
                      int bytes);
ALsizei capture_alsa (ALC_BackendPrivateData *privateData,
                      void *capture_buffer, int bufsiz);
ALfloat get_alsachannel (ALC_BackendPrivateData *privateData, ALuint channel);
int set_alsachannel (ALC_BackendPrivateData *privateData, ALuint channel,
                     ALfloat volume);
#else
#define alcBackendOpenALSA_(m)        NULL
#define release_alsa(h)
#define pause_alsa(h)
#define resume_alsa(h)
#define alcBackendSetAttributesALSA_(m,h,b,f,s) AL_FALSE
#define alsa_blitbuffer(h,d,b)
#define capture_alsa(h,d,b)           0
#define get_alsachannel(h,c)          0.0
#define set_alsachannel(h,c,v)        0
#endif /* USE_BACKEND_ALSA */

#ifdef USE_BACKEND_ARTS
ALC_BackendPrivateData *alcBackendOpenARts_ (ALC_OpenMode mode);
void release_arts (ALC_BackendPrivateData *privateData);
void pause_arts (ALC_BackendPrivateData *privateData);
void resume_arts (ALC_BackendPrivateData *privateData);
ALboolean alcBackendSetAttributesARts_ (ALC_OpenMode mode,
                                        ALC_BackendPrivateData *privateData,
                                        ALuint *bufsiz, ALenum *fmt,
                                        ALuint *speed);
void arts_blitbuffer (ALC_BackendPrivateData *privateData, void *data,
                      int bytes);
ALsizei capture_arts (ALC_BackendPrivateData *privateData,
                      void *capture_buffer, int bufsiz);
ALfloat get_artschannel (ALC_BackendPrivateData *privateData, ALuint channel);
int set_artschannel (ALC_BackendPrivateData *privateData, ALuint channel,
                     ALfloat volume);
#else
#define alcBackendOpenARts_(m)        NULL
#define release_arts(h)
#define pause_arts(h)
#define resume_arts(h)
#define alcBackendSetAttributesARts_(m,h,b,f,s) AL_FALSE
#define arts_blitbuffer(h,d,b)
#define capture_arts(h,d,b)           0
#define get_artschannel(h,c)          0.0
#define set_artschannel(h,c,v)        0
#endif /* USE_BACKEND_ARTS */

#ifdef USE_BACKEND_DMEDIA
ALC_BackendPrivateData *alcBackendOpenDMedia_ (ALC_OpenMode mode);
void release_dmedia (ALC_BackendPrivateData *privateData);
void pause_dmedia (ALC_BackendPrivateData *privateData);
void resume_dmedia (ALC_BackendPrivateData *privateData);
ALboolean alcBackendSetAttributesDMedia_ (ALC_OpenMode mode,
                                        ALC_BackendPrivateData *privateData,
                                        ALuint *bufsiz, ALenum *fmt,
                                        ALuint *speed);
void dmedia_blitbuffer (ALC_BackendPrivateData *privateData, void *data,
                      int bytes);
ALsizei capture_dmedia (ALC_BackendPrivateData *privateData,
                      void *capture_buffer, int bufsiz);
ALfloat get_dmediachannel (ALC_BackendPrivateData *privateData, ALuint channel);
int set_dmediachannel (ALC_BackendPrivateData *privateData, ALuint channel,
                     ALfloat volume);
#else
#define alcBackendOpenDMedia_(m)        NULL
#define release_dmedia(h)
#define pause_dmedia(h)
#define resume_dmedia(h)
#define alcBackendSetAttributesDMedia_(m,h,b,f,s) AL_FALSE
#define dmedia_blitbuffer(h,d,b)
#define capture_dmedia(h,d,b)           0
#define get_dmediachannel(h,c)          0.0
#define set_dmediachannel(h,c,v)        0
#endif /* USE_BACKEND_DMEDIA */

#ifdef USE_BACKEND_ESD
void *alcBackendOpenESD_ (ALC_OpenMode mode);
void release_esd (ALC_BackendPrivateData *privateData);
void pause_esd (ALC_BackendPrivateData *privateData);
void resume_esd (ALC_BackendPrivateData *privateData);
ALboolean alcBackendSetAttributesESD_ (ALC_OpenMode mode,
                                       ALC_BackendPrivateData *privateData,
                                       ALuint *bufsiz, ALenum *fmt,
                                       ALuint *speed);
void esd_blitbuffer (ALC_BackendPrivateData *privateData, void *data,
                     int bytes);
ALsizei capture_esd (ALC_BackendPrivateData *privateData,
                     void *capture_buffer, int bufsiz);
ALfloat get_esdchannel (ALC_BackendPrivateData *privateData, ALuint channel);
int set_esdchannel (ALC_BackendPrivateData *privateData, ALuint channel,
                    ALfloat volume);
#else
#define alcBackendOpenESD_(m)         NULL
#define pause_esd(h)
#define release_esd(h)
#define resume_esd(h)
#define alcBackendSetAttributesESD_(m,h,b,f,s) AL_FALSE
#define esd_blitbuffer(h,d,b)
#define capture_esd(h,d,b)            0
#define get_esdchannel(h,c)           0.0
#define set_esdchannel(h,c,v)         0
#endif /* USE_BACKEND_ESD */

#ifdef USE_BACKEND_SDL
ALC_BackendPrivateData *alcBackendOpenSDL_ (ALC_OpenMode mode);
void release_sdl (ALC_BackendPrivateData *privateData);
void pause_sdl (ALC_BackendPrivateData *privateData);
void resume_sdl (ALC_BackendPrivateData *privateData);
ALboolean alcBackendSetAttributesSDL_ (ALC_OpenMode mode,
                                       ALC_BackendPrivateData *privateData,
                                       ALuint *bufsiz, ALenum *fmt,
                                       ALuint *speed);
void sdl_blitbuffer (ALC_BackendPrivateData *privateData, void *data,
                     int bytes);
ALsizei capture_sdl (ALC_BackendPrivateData *privateData,
                     void *capture_buffer, int bufsiz);
ALfloat get_sdlchannel (ALC_BackendPrivateData *privateData, ALuint channel);
int set_sdlchannel (ALC_BackendPrivateData *privateData, ALuint channel,
                    ALfloat volume);
#else
#define alcBackendOpenSDL_(m)         NULL
#define release_sdl(h)
#define pause_sdl(h)
#define resume_sdl(h)
#define alcBackendSetAttributesSDL_(m,h,b,f,s) AL_FALSE
#define sdl_blitbuffer(h,d,b)
#define capture_sdl(h,d,b)            0
#define get_sdlchannel(h,c)           0.0
#define set_sdlchannel(h,c,v)         0
#endif /* USE_BACKEND_SDL */

#ifdef USE_BACKEND_NULL
ALC_BackendPrivateData *alcBackendOpenNull_ (ALC_OpenMode mode);
void release_null (ALC_BackendPrivateData *privateData);
void pause_null (ALC_BackendPrivateData *privateData);
void resume_null (ALC_BackendPrivateData *privateData);
ALboolean alcBackendSetAttributesNull_ (ALC_OpenMode mode,
                                        ALC_BackendPrivateData *privateData,
                                        ALuint *bufsiz, ALenum *fmt,
                                        ALuint *speed);
void null_blitbuffer (ALC_BackendPrivateData *privateData, void *data,
                      int bytes);
ALsizei capture_null (ALC_BackendPrivateData *privateData,
                      void *capture_buffer, int bufsiz);
ALfloat get_nullchannel (ALC_BackendPrivateData *privateData, ALuint channel);
int set_nullchannel (ALC_BackendPrivateData *privateData, ALuint channel,
                     ALfloat volume);
#else
#define alcBackendOpenNull_(m)        NULL
#define release_null(h)
#define pause_null(h)
#define resume_null(h)
#define alcBackendSetAttributesNull_(m,h,b,f,s) AL_FALSE
#define null_blitbuffer(h,d,b)
#define capture_null(h,d,b)           0
#define get_nullchannel(h,c)          0.0
#define set_nullchannel(h,c,v)        0
#endif /* USE_BACKEND_NULL */

#ifdef USE_BACKEND_WAVEOUT
ALC_BackendPrivateData *alcBackendOpenWAVE_ (ALC_OpenMode mode);
void release_waveout (ALC_BackendPrivateData *privateData);
void pause_waveout (ALC_BackendPrivateData *privateData);
void resume_waveout (ALC_BackendPrivateData *privateData);
ALboolean alcBackendSetAttributesWAVE_ (ALC_OpenMode mode,
                                        ALC_BackendPrivateData *privateData,
                                        ALuint *bufsiz, ALenum *fmt,
                                        ALuint *speed);
void waveout_blitbuffer (ALC_BackendPrivateData *privateData, void *data,
                         int bytes);
ALsizei capture_waveout (ALC_BackendPrivateData *privateData,
                         void *capture_buffer, int bufsiz);
ALfloat get_waveoutchannel (ALC_BackendPrivateData *privateData,
                            ALuint channel);
int set_waveoutchannel (ALC_BackendPrivateData *privateData, ALuint channel,
                        ALfloat volume);
#else
#define alcBackendOpenWAVE_(m)        NULL
#define release_waveout(h)
#define pause_waveout(h)
#define resume_waveout(h)
#define alcBackendSetAttributesWAVE_(m,h,b,f,s) AL_FALSE
#define waveout_blitbuffer(h,d,b)
#define capture_waveout(h,d,b)        0
#define get_waveoutchannel(h,c)       0.0
#define set_waveoutchannel(h,c,v)     0
#endif /* USE_BACKEND_WAVEOUT */

#endif /* AL_BACKENDS_ALC_BACKEND_H_ */
