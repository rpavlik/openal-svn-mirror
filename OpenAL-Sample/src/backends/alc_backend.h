/*
 * High level prototypes for sound device aquisition and management.
 */
#ifndef AL_BACKENDS_ALC_BACKEND_H_
#define AL_BACKENDS_ALC_BACKEND_H_

#include "al_siteconfig.h"

#include <AL/al.h>

typedef enum
{
  ALC_OPEN_INPUT_,
  ALC_OPEN_OUTPUT_
} ALC_OpenMode;

/*
 * Various data, the details depend on the backend in question.
 */
typedef void ALC_BackendPrivateData;

typedef struct ALC_BackendOpsStruct
{
  /*
   * Closes an (output or input) backend. Returns AL_TRUE if closing was
   * successful, AL_FALSE if the backend could not be closed for some
   * reason. This function is used to implement alcCloseDevice.
   */
  void (*close) (ALC_BackendPrivateData *privateData);

  /*
   * Informs a backend that it is about to get paused. This function is used to
   * implement alcMakeContextCurrent(NULL).
   */
  void (*pause) (ALC_BackendPrivateData *privateData);

  /*
   * Informs a backend that it is about to get resumed. This function is used to
   * implement alcMakeContextCurrent(NON_NULL).
   */
  void (*resume) (ALC_BackendPrivateData *privateData);

  /*
   * Sets the parameters for a backend. Because we follow a meet-or-exceed
   * policty, *deviceBufferSizeInBytes, *fmt, and *speed might be different from
   * the parameters initially passed, so the caller should check these after a
   * succesful call. Returns AL_TRUE if setting was successful, AL_FALSE if the
   * parameters could not be matched or exceeded. This function is used to
   * implement alcMakeContextCurrent(NON_NULL).
   */
  ALboolean (*setAttributes) (ALC_BackendPrivateData *privateData,
                              ALuint *deviceBufferSizeInBytes, ALenum *format,
                              ALuint *speed);

  /*
   * Writes a given interleaved array of sound data to an output backend. This
   * function is used to implement (a)sync_mixer_iterate.
   */
  void (*write) (ALC_BackendPrivateData *privateData, const void *data,
                 int bytesToWrite);

  /*
   * Captures data from an input backend into the given buffer. This function is
   * used to implement capture functionality.
   */
  ALsizei (*read) (ALC_BackendPrivateData *privateData, void *data,
                   int bytesToRead);

  /*
   * Returns the normalized volume for the given channel (main/PCM/CD) on an
   * output backend. This function is used to implement alcGetAudioChannel_LOKI.
   */
  ALfloat (*getAudioChannel) (ALC_BackendPrivateData *privateData,
                              ALuint channel);
  /*
   * Sets the normalized volume for the given channel (main/PCM/CD) on an output
   * backend. This function is used to implement alcSetAudioChannel_LOKI.
   */
  int (*setAudioChannel) (ALC_BackendPrivateData *privateData, ALuint channel,
                          ALfloat volume);
} ALC_BackendOps;

/*
 * Returns a pointer to private backend data via the 3rd argument, or NULL if no
 * such backend is available. In the former case, a pointer to a backend
 * function table suitable for reading or writing sound data is returned via the
 * 2nd argument. This function is used to implement alcOpenDevice.
 */
void alcBackendOpen_ (ALC_OpenMode mode, ALC_BackendOps **ops,
                      ALC_BackendPrivateData **privateData);

/******************************************************************************/

void alcBackendOpenOSS_ (ALC_OpenMode mode, ALC_BackendOps **ops,
                         ALC_BackendPrivateData **privateData);
void alcBackendOpenNative_ (ALC_OpenMode mode, ALC_BackendOps **ops,
                            ALC_BackendPrivateData **privateData);
void alcBackendOpenALSA_ (ALC_OpenMode mode, ALC_BackendOps **ops,
                          ALC_BackendPrivateData **privateData);
void alcBackendOpenARts_ (ALC_OpenMode mode, ALC_BackendOps **ops,
                          ALC_BackendPrivateData **privateData);
void alcBackendOpenDMedia_ (ALC_OpenMode mode, ALC_BackendOps **ops,
                            ALC_BackendPrivateData **privateData);
void alcBackendOpenESD_ (ALC_OpenMode mode, ALC_BackendOps **ops,
                         ALC_BackendPrivateData **privateData);
void alcBackendOpenSDL_ (ALC_OpenMode mode, ALC_BackendOps **ops,
                         ALC_BackendPrivateData **privateData);
void alcBackendOpenNull_ (ALC_OpenMode mode, ALC_BackendOps **ops,
                          ALC_BackendPrivateData **privateData);
void alcBackendOpenWAVE_ (ALC_OpenMode mode, ALC_BackendOps **ops,
                          ALC_BackendPrivateData **privateData);

#endif /* AL_BACKENDS_ALC_BACKEND_H_ */
