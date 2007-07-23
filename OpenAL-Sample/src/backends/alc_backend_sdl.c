/*
 * SDL backend.
 */
#include "al_siteconfig.h"
#include "backends/alc_backend.h"
#include <stdlib.h>

#ifndef USE_BACKEND_SDL

void
alcBackendOpenSDL_ (UNUSED (ALC_OpenMode mode), UNUSED (ALC_BackendOps **ops),
                    struct ALC_BackendPrivateData **privateData)
{
  *privateData = NULL;
}

#else

/* for memcpy and memset */
#include <string.h>

/* for the SDL API */
#include <SDL.h>
#include <SDL_audio.h>

/* for the LOKI quad format defines */
#include <AL/alext.h>

/* for _alDebug and related tokens */
#include "al_debug.h"

/* for our dlopen wrapper API */
#include "al_dlopen.h"

static void SDLCALL (*pSDL_Delay) (Uint32 ms);
static void SDLCALL (*pSDL_PauseAudio) (int pause_on);
static void SDLCALL (*pSDL_CloseAudio) (void);
static int SDLCALL (*pSDL_OpenAudio) (SDL_AudioSpec *desired,
                                      SDL_AudioSpec *obtained);
static int SDLCALL (*pSDL_Init) (Uint32 flags);
static char *SDLCALL (*pSDL_GetError) (void);
static void SDLCALL (*pSDL_LockAudio) (void);
static void SDLCALL (*pSDL_UnlockAudio) (void);

static ALboolean
getAPIEntriesSDL (void)
{
  static AL_DLHandle libHandle = (AL_DLHandle) 0;
#ifdef OPENAL_DLOPEN_SDL
  const char *error = NULL;
#endif

  if (libHandle != (AL_DLHandle) 0)
    {
      /* already loaded. */
      return AL_TRUE;
    }

#ifdef OPENAL_DLOPEN_SDL
#define AL_SYM_SDL_(x, t)                          \
        p##x = t alDLFunSym_ (libHandle, #x); \
        error = alDLError_ (); \
        if (error != NULL) { \
                alDLClose_ (libHandle); \
                libHandle = (AL_DLHandle) 0; \
                return AL_FALSE; \
        }

  alDLError_ ();
  libHandle = alDLOpen_ ("libSDL.so");
  error = alDLError_ ();
  if (error != NULL)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "could not open SDL library: %s", error);
      return AL_FALSE;
    }

#else
#define AL_SYM_SDL_(x, t) p##x = x;
  libHandle = (AL_DLHandle) 0xF00DF00D;
#endif

  AL_SYM_SDL_ (SDL_Delay, (void SDLCALL (*)(Uint32)));
  AL_SYM_SDL_ (SDL_PauseAudio, (void SDLCALL (*)(int)));
  AL_SYM_SDL_ (SDL_CloseAudio, (void SDLCALL (*)(void)));
  AL_SYM_SDL_ (SDL_OpenAudio,
               (int SDLCALL (*)(SDL_AudioSpec *, SDL_AudioSpec *)));
  AL_SYM_SDL_ (SDL_Init, (int SDLCALL (*)(Uint32)));
  AL_SYM_SDL_ (SDL_GetError, (char *SDLCALL (*)(void)));
  AL_SYM_SDL_ (SDL_LockAudio, (void SDLCALL (*)(void)));
  AL_SYM_SDL_ (SDL_UnlockAudio, (void SDLCALL (*)(void)));

  return AL_TRUE;
}

/* units are in bytes */
struct sdlData
{
  SDL_AudioSpec spec;
  ALboolean firstTime;
  Uint8 *ringBuffer;
  Uint32 ringBufferSize;
  Uint32 readOffset;
  Uint32 writeOffset;
};

static void
pauseSDL (UNUSED (struct ALC_BackendPrivateData *privateData))
{
}

static void
resumeSDL (UNUSED (struct ALC_BackendPrivateData *privateData))
{
}

static void
closeSDL (struct ALC_BackendPrivateData *privateData)
{
  struct sdlData *sd = (struct sdlData *) privateData;
  free (sd->ringBuffer);
  pSDL_CloseAudio ();
}

static ALboolean
convertFormatFromAL (ALuint *bytesPerFrame, Uint16 *sdlFormat,
                     Uint8 *numChannels, ALenum alFormat)
{
  switch (alFormat)
    {
    case AL_FORMAT_MONO8:
      *bytesPerFrame = 1;
      *sdlFormat = AUDIO_U8;
      *numChannels = 1;
      return AL_TRUE;
    case AL_FORMAT_STEREO8:
      *bytesPerFrame = 2;
      *sdlFormat = AUDIO_U8;
      *numChannels = 2;
      return AL_TRUE;
    case AL_FORMAT_QUAD8_LOKI:
      *bytesPerFrame = 4;
      *sdlFormat = AUDIO_U8;
      *numChannels = 4;
      return AL_TRUE;
    case AL_FORMAT_MONO16:
      *bytesPerFrame = 2;
      *sdlFormat = AUDIO_S16SYS;
      *numChannels = 1;
      return AL_TRUE;
    case AL_FORMAT_STEREO16:
      *bytesPerFrame = 4;
      *sdlFormat = AUDIO_S16SYS;
      *numChannels = 2;
      return AL_TRUE;
    case AL_FORMAT_QUAD16_LOKI:
      *bytesPerFrame = 8;
      *sdlFormat = AUDIO_S16SYS;
      *numChannels = 4;
      return AL_TRUE;
    default:
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "uknown OpenAL format 0x%x", alFormat);
      return AL_FALSE;
    }
}

static void
fillAudio (void *userdata, Uint8 *stream, int len)
{
  struct sdlData *sd = (struct sdlData *) userdata;
  memcpy (stream, sd->ringBuffer + sd->readOffset, (size_t) len);
  sd->readOffset += len;

  if (sd->readOffset >= sd->ringBufferSize)
    {
      sd->readOffset = 0;
      sd->writeOffset = 0;
    }
}

static ALboolean
setAttributesSDL (struct ALC_BackendPrivateData *privateData,
                  ALuint *deviceBufferSizeInBytes, ALenum *format,
                  ALuint *speed)
{
  struct sdlData *sd = (struct sdlData *) privateData;
  ALuint bytesPerFrame;

  if (!convertFormatFromAL (&bytesPerFrame, &sd->spec.format,
                            &sd->spec.channels, *format))
    {
      return AL_FALSE;
    }
  sd->spec.freq = *speed;
  /* Note that the field is called "samples", but SDL seems to mean "frames". */
  sd->spec.samples = *deviceBufferSizeInBytes / bytesPerFrame;
  sd->spec.callback = fillAudio;
  sd->spec.userdata = sd;
  sd->firstTime = AL_TRUE;

  if (pSDL_OpenAudio (&sd->spec, NULL) < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "opening audio failed: %s",
                pSDL_GetError ());
      return AL_FALSE;
    }

  sd->ringBufferSize = 2 * sd->spec.size;
  sd->ringBuffer = (Uint8 *) malloc (sd->ringBufferSize);
  if (sd->ringBuffer == NULL)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "failed to allocate ring buffer");
    }
  sd->readOffset = 0;
  sd->writeOffset = 0;
  memset (sd->ringBuffer, 0, sd->ringBufferSize);

  *deviceBufferSizeInBytes = sd->spec.size;
  return AL_TRUE;
}

static void
writeSDL (struct ALC_BackendPrivateData *privateData, const void *data,
          int bytesToWrite)
{
  struct sdlData *sd = (struct sdlData *) privateData;
  if (sd->firstTime == AL_TRUE)
    {
      sd->firstTime = AL_FALSE;
      memcpy (sd->ringBuffer + sd->writeOffset, data, (size_t) bytesToWrite);
      sd->writeOffset = bytesToWrite;
      /* start SDL callback mojo */
      pSDL_PauseAudio (0);
    }
  else
    {
      pSDL_LockAudio ();
      while (sd->writeOffset >= sd->ringBufferSize)
        {
          pSDL_UnlockAudio ();
          pSDL_Delay (1);
          pSDL_LockAudio ();
        }

      memcpy (sd->ringBuffer + sd->writeOffset, data, (size_t) bytesToWrite);
      sd->writeOffset += bytesToWrite;

      pSDL_UnlockAudio ();
    }
}

static ALsizei
readSDL (UNUSED (struct ALC_BackendPrivateData *privateData),
         UNUSED (void *capture_buffer), UNUSED (int bytesToRead))
{
  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "should never happen");
  return 0;
}

static ALfloat
getAudioChannelSDL (UNUSED (struct ALC_BackendPrivateData *privateData),
                    UNUSED (ALuint channel))
{
  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
            "SDL backend does not support getting the volume");
  return -1.0f;
}

static int
setAudioChannelSDL (UNUSED (struct ALC_BackendPrivateData *privateData),
                    UNUSED (ALuint channel), UNUSED (ALfloat volume))
{
  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
            "SDL backend does not support setting the volume");
  return -1;
}

static const ALCchar *getNameSDL(struct ALC_BackendPrivateData *privateData)
{
    return "Simple DirectMedia Layer (SDL)";
}

static ALC_BackendOps sdlOps = {
  closeSDL,
  pauseSDL,
  resumeSDL,
  setAttributesSDL,
  writeSDL,
  readSDL,
  getAudioChannelSDL,
  setAudioChannelSDL,
  getNameSDL
};

void
alcBackendOpenSDL_ (ALC_OpenMode mode, ALC_BackendOps **ops,
                    struct ALC_BackendPrivateData **privateData)
{
  struct sdlData *sd;

  if (!getAPIEntriesSDL ())
    {
      *privateData = NULL;
      return;
    }

  if (mode == ALC_OPEN_INPUT_)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "SDL backend does not support input");
      *privateData = NULL;
      return;
    }

  if (pSDL_Init (SDL_INIT_AUDIO) == -1)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "SDL audio initialization failed");
      *privateData = NULL;
      return;
    }

  sd = (struct sdlData *) malloc (sizeof *sd);
  if (sd == NULL)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "failed to allocate backend data");
      *privateData = NULL;
      return;
    }

  *ops = &sdlOps;
  *privateData = (struct ALC_BackendPrivateData *) sd;
  _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
            "SDL backend opened successfully");
}

#endif /* USE_BACKEND_SDL */
