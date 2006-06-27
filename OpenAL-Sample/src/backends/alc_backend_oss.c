/*
 * Open Sound System (OSS) backend for OpenAL
 */

#include "al_siteconfig.h"
#include "backends/alc_backend.h"

#ifndef USE_BACKEND_OSS

void
alcBackendOpenOSS_ (UNUSED (ALC_OpenMode mode),
                    UNUSED (ALC_BackendOps **ops),
                    ALC_BackendPrivateData **privateData)
{
  *privateData = NULL;
}

#else

#include <stdlib.h>
#include <sys/soundcard.h>

static void
closeOSS (UNUSED (void *privateData))
{
}

static ALboolean
setAttributesOSS (UNUSED (void *privateData),
                  UNUSED (ALuint *bufferSizeInBytes), UNUSED (ALenum *format),
                  UNUSED (ALuint *speed))
{
  return AL_FALSE;
}

static void
writeOSS (UNUSED (void *privateData), UNUSED (const void *data),
          UNUSED (int bytesToWrite))
{
}

static ALsizei
readOSS (UNUSED (void *privateData), UNUSED (void *data),
         UNUSED (int bytesToRead))
{
  return 0;
}

static void
pauseOSS (UNUSED (void *privateData))
{
}

static void
resumeOSS (UNUSED (void *privateData))
{
}

static ALfloat
getAudioChannelOSS (UNUSED (void *privateData), UNUSED (ALuint channel))
{
  return 0.0;
}

static int
setAudioChannelOSS (UNUSED (void *privateData), UNUSED (ALuint channel),
                    UNUSED (ALfloat volume))
{
  return 0;
}

static ALC_BackendOps ossOps = {
  closeOSS,
  pauseOSS,
  resumeOSS,
  setAttributesOSS,
  writeOSS,
  readOSS,
  getAudioChannelOSS,
  setAudioChannelOSS
};

void
alcBackendOpenOSS_ (UNUSED (ALC_OpenMode mode), ALC_BackendOps **ops,
                    ALC_BackendPrivateData **privateData)
{
  *ops = &ossOps;
  *privateData = (ALC_BackendPrivateData *) 0xDEADBEEF;
}

#endif /* USE_BACKEND_OSS */
