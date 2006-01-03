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

#endif /* INTERFACE_SOUND_H_ */
