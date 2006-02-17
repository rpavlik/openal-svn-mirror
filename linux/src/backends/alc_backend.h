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
                                    ALuint *bufferSize, ALenum *format,
                                    ALuint *speed);

/*
 * Writes a given interleaved array of sound data to an output backend. This
 * function is used to implement (a)sync_mixer_iterate.
 */
void alcBackendWrite_ (ALC_Backend *backend, const void *data, int size);

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

typedef struct ALC_BackendOpsStruct
{
  ALC_BackendPrivateData *(*open) (ALC_OpenMode mode);
  void (*close) (ALC_BackendPrivateData *privateData);
  void (*pause) (ALC_BackendPrivateData *privateData);
  void (*resume) (ALC_BackendPrivateData *privateData);
  ALboolean (*setAttributes) (ALC_BackendPrivateData *privateData,
                              ALuint *bufferSize, ALenum *format,
                              ALuint *speed);
  void (*write) (ALC_BackendPrivateData *privateData, const void *data,
                 int size);
  ALsizei (*read) (ALC_BackendPrivateData *privateData, void *data, int size);
  ALfloat (*getAudioChannel) (ALC_BackendPrivateData *privateData,
                              ALuint channel);
  int (*setAudioChannel) (ALC_BackendPrivateData *privateData, ALuint channel,
                          ALfloat volume);
} ALC_BackendOps;

ALC_BackendOps *alcGetBackendOpsNative_ (void);

#ifdef USE_BACKEND_ALSA
ALC_BackendOps *alcGetBackendOpsALSA_ (void);
#else
#define alcGetBackendOpsALSA_() NULL
#endif /* USE_BACKEND_ALSA */

#ifdef USE_BACKEND_ARTS
ALC_BackendOps *alcGetBackendOpsARts_ (void);
#else
#define alcGetBackendOpsARts_() NULL
#endif /* USE_BACKEND_ARTS */

#ifdef USE_BACKEND_DMEDIA
ALC_BackendOps *alcGetBackendOpsDMedia_ (void);
#else
#define alcGetBackendOpsDMedia_() NULL
#endif /* USE_BACKEND_DMEDIA */

#ifdef USE_BACKEND_ESD
ALC_BackendOps *alcGetBackendOpsESD_ (void);
#else
#define alcGetBackendOpsESD_() NULL
#endif /* USE_BACKEND_ESD */

#ifdef USE_BACKEND_SDL
ALC_BackendOps *alcGetBackendOpsSDL_ (void);
#else
#define alcGetBackendOpsSDL_() NULL
#endif /* USE_BACKEND_SDL */

#ifdef USE_BACKEND_NULL
ALC_BackendOps *alcGetBackendOpsNull_ (void);
#else
#define alcGetBackendOpsNull_() NULL
#endif /* USE_BACKEND_NULL */

#ifdef USE_BACKEND_WAVEOUT
ALC_BackendOps *alcGetBackendOpsWAVE_ (void);
#else
#define alcGetBackendOpsWAVE_() NULL
#endif /* USE_BACKEND_WAVEOUT */

#endif /* AL_BACKENDS_ALC_BACKEND_H_ */
