/*
 * Null device. Does basically nothing.
*/

#include "al_siteconfig.h"
#include "backends/alc_backend.h"
#include <stdlib.h>

#ifndef USE_BACKEND_NULL

void
alcBackendOpenNull_ (UNUSED (ALC_OpenMode mode),
                     UNUSED (ALC_BackendOps **ops),
                     struct ALC_BackendPrivateData **privateData)
{
  *privateData = NULL;
}

#else

#include <AL/al.h>

#include "al_debug.h"

/* private data for this backend */
struct nullData
{
  ALC_OpenMode mode;
};

static void
closeNull (struct ALC_BackendPrivateData *privateData)
{
  free (privateData);
}

static void
pauseNull (UNUSED (struct ALC_BackendPrivateData *privateData))
{
}

static void
resumeNull (UNUSED (struct ALC_BackendPrivateData *privateData))
{
}

static ALboolean
setAttributesNull (UNUSED (struct ALC_BackendPrivateData *privateData),
                   UNUSED (ALuint *deviceBufferSizeInBytes),
                   UNUSED (ALenum *format), UNUSED (ALuint *speed))
{
  return AL_TRUE;
}

static void
writeNull (struct ALC_BackendPrivateData *privateData,
           UNUSED (const void *data), UNUSED (int bytesToWrite))
{
  struct nullData *nd = (struct nullData *) privateData;
  if (nd->mode != ALC_OPEN_OUTPUT_)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "device not opened for output");
    }
}

static ALsizei
readNull (struct ALC_BackendPrivateData *privateData,
          UNUSED (void *data), UNUSED (int bytesToRead))
{
  struct nullData *nd = (struct nullData *) privateData;
  if (nd->mode != ALC_OPEN_INPUT_)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "device not opened for input");
    }
  return 0;
}

static ALfloat
getAudioChannelNull (UNUSED (struct ALC_BackendPrivateData *privateData),
                     UNUSED (ALuint channel))
{
  return 0.0f;
}

static int
setAudioChannelNull (UNUSED (struct ALC_BackendPrivateData *privateData),
                     UNUSED (ALuint channel), UNUSED (ALfloat volume))
{
  return 0;
}

static const ALCchar *getNameNull(struct ALC_BackendPrivateData *privateData)
{
    return "Null Driver";
}

static ALC_BackendOps nullOps = {
  closeNull,
  pauseNull,
  resumeNull,
  setAttributesNull,
  writeNull,
  readNull,
  getAudioChannelNull,
  setAudioChannelNull,
  getNameNull
};

void
alcBackendOpenNull_ (ALC_OpenMode mode, ALC_BackendOps **ops,
                     struct ALC_BackendPrivateData **privateData)
{
  struct nullData *nd = (struct nullData *) malloc (sizeof *nd);
  if (nd == NULL)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "failed to allocate backend data");
      *privateData = NULL;
      return;
    }

  nd->mode = mode;

  *ops = &nullOps;
  *privateData = (struct ALC_BackendPrivateData *) nd;
}

#endif /* USE_BACKEND_NULL */
