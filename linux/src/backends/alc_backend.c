/*
 * This file defines the high-level interface to the sound device. It
 * actually dispatches requests based on the architecture that we're
 * targetting. See alc_backend.h for more of a clue.
 */
#include "al_siteconfig.h"

#include <AL/al.h>
#include <stdlib.h>
#include <string.h>

#include "al_config.h"
#include "al_debug.h"
#include "al_main.h"
#include "backends/alc_backend.h"

typedef enum
{
  AL_BACKEND_NONE_,
  AL_BACKEND_NATIVE_,           /* native audio for platform */
  AL_BACKEND_ALSA_,             /* ALSA backend */
  AL_BACKEND_ARTS_,             /* aRts backend */
  AL_BACKEND_DMEDIA_,           /* Irix/DMedia back-end */
  AL_BACKEND_ESD_,              /* ESD backend */
  AL_BACKEND_SDL_,              /* SDL backend */
  AL_BACKEND_NULL_,             /* null backend */
  AL_BACKEND_WAVEOUT_           /* WAVE backend */
} ALC_BackendType;

struct ALC_BackendStruct
{
  ALC_BackendType type;
  ALC_BackendPrivateData *privateData;
};

ALC_Backend *
alcBackendOpen_ (ALC_OpenMode mode)
{
  Rcvar device_params;
  Rcvar device_list;
  Rcvar device;
  ALC_BackendType type = AL_BACKEND_NONE_;
  ALC_BackendPrivateData *privateData = NULL;
  ALC_Backend *backend;
  char adevname[64];            /* FIXME: magic number */

  device_list = rc_lookup ("devices");
  while (device_list != NULL)
    {
      device = rc_car (device_list);
      device_list = rc_cdr (device_list);

      switch (rc_type (device))
        {
        case ALRC_STRING:
          rc_tostr0 (device, adevname, 64);
          break;
        case ALRC_SYMBOL:
          rc_symtostr0 (device, adevname, 64);
          break;
        case ALRC_CONSCELL:
          device_params = rc_cdr (device);
          if (device_params == NULL)
            {
              continue;
            }
          rc_define ("device-params", device_params);
          rc_symtostr0 (rc_car (device), adevname, 64);
          break;
        default:
          _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                    "alcBackendOpen_: bad type %s for device",
                    rc_typestr (rc_type (device)));
          continue;
        }

      if (strcmp (adevname, "dsp") == 0)
        {
          _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                    "alcBackendOpen_: 'dsp' is a deprecated device name. Use 'native' instead.");
          privateData = alcBackendOpenNative_ (mode);
          if (privateData != NULL)
            {
              type = AL_BACKEND_NATIVE_;
              break;
            }
        }
      if (strcmp (adevname, "native") == 0)
        {
          privateData = alcBackendOpenNative_ (mode);
          if (privateData != NULL)
            {
              type = AL_BACKEND_NATIVE_;
              break;
            }
        }

      if (strcmp (adevname, "alsa") == 0)
        {
          privateData = alcBackendOpenALSA_ (mode);
          if (privateData != NULL)
            {
              type = AL_BACKEND_ALSA_;
              break;
            }
        }

      if (strcmp (adevname, "arts") == 0)
        {
          privateData = alcBackendOpenARts_ (mode);
          if (privateData != NULL)
            {
              type = AL_BACKEND_ARTS_;
              break;
            }
        }

      if (strcmp (adevname, "dmedia") == 0)
        {
          privateData = alcBackendOpenDMedia_ (mode);
          if (privateData != NULL)
            {
              type = AL_BACKEND_DMEDIA_;
              break;
            }
        }

      if (strcmp (adevname, "esd") == 0)
        {
          privateData = alcBackendOpenESD_ (mode);
          if (privateData != NULL)
            {
              type = AL_BACKEND_ESD_;
              break;
            }
        }

      if (strcmp (adevname, "sdl") == 0)
        {
          privateData = alcBackendOpenSDL_ (mode);
          if (privateData != NULL)
            {
              type = AL_BACKEND_SDL_;
              break;
            }
        }

      if (strcmp (adevname, "null") == 0)
        {
          privateData = alcBackendOpenNull_ (mode);
          if (privateData != NULL)
            {
              type = AL_BACKEND_NULL_;
              break;
            }
        }

      if (strcmp (adevname, "waveout") == 0)
        {
          privateData = alcBackendOpenWAVE_ (mode);
          if (privateData != NULL)
            {
              type = AL_BACKEND_WAVEOUT_;
              break;
            }
        }
    }

  /* fallback: native */
  if (type == AL_BACKEND_NONE_)
    {
      privateData = alcBackendOpenNative_ (mode);
      if (privateData != NULL)
        {
          type = AL_BACKEND_NATIVE_;
        }
    }

  if (type == AL_BACKEND_NONE_)
    {
      return NULL;
    }

  backend =
    (struct ALC_BackendStruct *) malloc (sizeof (struct ALC_BackendStruct));
  if (backend == NULL)
    {
      return NULL;
    }

  backend->type = type;
  backend->privateData = privateData;
  return backend;

}

ALboolean
alcBackendClose_ (ALC_Backend *backend)
{
  switch (backend->type)
    {
    case AL_BACKEND_NATIVE_:
      release_native (backend->privateData);
      break;
    case AL_BACKEND_ALSA_:
      release_alsa (backend->privateData);
      break;
    case AL_BACKEND_ARTS_:
      release_arts (backend->privateData);
      break;
    case AL_BACKEND_DMEDIA_:
      release_dmedia (backend->privateData);
      break;
    case AL_BACKEND_ESD_:
      release_esd (backend->privateData);
      break;
    case AL_BACKEND_SDL_:
      release_sdl (backend->privateData);
      break;
    case AL_BACKEND_NULL_:
      release_null (backend->privateData);
      break;
    case AL_BACKEND_WAVEOUT_:
      release_waveout (backend->privateData);
      break;
    default:
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                "alcBackendClose_: unknown backend %d\n", backend->type);
      return AL_FALSE;
    }
  free (backend);
  return AL_TRUE;
}

void
alcBackendPause_ (ALC_Backend *backend)
{
  switch (backend->type)
    {
    case AL_BACKEND_NATIVE_:
      pause_nativedevice (backend->privateData);
      break;
    case AL_BACKEND_ALSA_:
      pause_alsa (backend->privateData);
      break;
    case AL_BACKEND_ARTS_:
      pause_arts (backend->privateData);
      break;
    case AL_BACKEND_DMEDIA_:
      pause_dmedia (backend->privateData);
      break;
    case AL_BACKEND_ESD_:
      pause_esd (backend->privateData);
      break;
    case AL_BACKEND_SDL_:
      pause_sdl (backend->privateData);
      break;
    case AL_BACKEND_NULL_:
      pause_null (backend->privateData);
      break;
    case AL_BACKEND_WAVEOUT_:
      pause_waveout (backend->privateData);
      break;
    default:
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                "alcBackendPause_: unknown backend %d\n", backend->type);
      break;
    }
}

void
alcBackendResume_ (ALC_Backend *backend)
{
  switch (backend->type)
    {
    case AL_BACKEND_NATIVE_:
      resume_nativedevice (backend->privateData);
      break;
    case AL_BACKEND_ALSA_:
      resume_alsa (backend->privateData);
      break;
    case AL_BACKEND_ARTS_:
      resume_arts (backend->privateData);
      break;
    case AL_BACKEND_DMEDIA_:
      resume_dmedia (backend->privateData);
      break;
    case AL_BACKEND_ESD_:
      resume_esd (backend->privateData);
      break;
    case AL_BACKEND_SDL_:
      resume_sdl (backend->privateData);
      break;
    case AL_BACKEND_NULL_:
      resume_null (backend->privateData);
      break;
    case AL_BACKEND_WAVEOUT_:
      resume_waveout (backend->privateData);
      break;
    default:
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                "alcBackendResume_: unknown backend %d\n", backend->type);
      break;
    }
}

ALboolean
alcBackendSetAttributes_ (ALC_Backend *backend, ALuint *bufsiz,
                          ALenum *fmt, ALuint *speed)
{
  switch (backend->type)
    {
    case AL_BACKEND_NATIVE_:
      return alcBackendSetAttributesNative_ (backend->privateData, bufsiz,
                                             fmt, speed);
    case AL_BACKEND_ALSA_:
      return alcBackendSetAttributesALSA_ (backend->privateData, bufsiz, fmt,
                                           speed);
    case AL_BACKEND_ARTS_:
      return alcBackendSetAttributesARts_ (backend->privateData, bufsiz, fmt,
                                           speed);
    case AL_BACKEND_DMEDIA_:
      return alcBackendSetAttributesDMedia_ (backend->privateData, bufsiz,
                                             fmt, speed);
    case AL_BACKEND_ESD_:
      return alcBackendSetAttributesESD_ (backend->privateData, bufsiz, fmt,
                                          speed);
    case AL_BACKEND_SDL_:
      return alcBackendSetAttributesSDL_ (backend->privateData, bufsiz, fmt,
                                          speed);
    case AL_BACKEND_NULL_:
      return alcBackendSetAttributesNull_ (backend->privateData, bufsiz, fmt,
                                           speed);
    case AL_BACKEND_WAVEOUT_:
      return alcBackendSetAttributesWAVE_ (backend->privateData, bufsiz, fmt,
                                           speed);
    default:
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                "alcBackendSetWrite_: unknown backend %d\n", backend->type);
      return AL_FALSE;
    }
}

void
alcBackendWrite_ (ALC_Backend *backend, void *dataptr, int bytes_to_write)
{
  switch (backend->type)
    {
    case AL_BACKEND_NATIVE_:
      native_blitbuffer (backend->privateData, dataptr, bytes_to_write);
      break;
    case AL_BACKEND_ALSA_:
      alsa_blitbuffer (backend->privateData, dataptr, bytes_to_write);
      break;
    case AL_BACKEND_ARTS_:
      arts_blitbuffer (backend->privateData, dataptr, bytes_to_write);
      break;
    case AL_BACKEND_DMEDIA_:
      dmedia_blitbuffer (backend->privateData, dataptr, bytes_to_write);
      break;
    case AL_BACKEND_ESD_:
      esd_blitbuffer (backend->privateData, dataptr, bytes_to_write);
      break;
    case AL_BACKEND_SDL_:
      sdl_blitbuffer (backend->privateData, dataptr, bytes_to_write);
      break;
    case AL_BACKEND_NULL_:
      null_blitbuffer (backend->privateData, dataptr, bytes_to_write);
      break;
    case AL_BACKEND_WAVEOUT_:
      waveout_blitbuffer (backend->privateData, dataptr, bytes_to_write);
      break;
    default:
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                "alcBackendWrite_: unknown backend %d\n", backend->type);
      break;
    }
}

ALsizei
alcBackendRead_ (ALC_Backend *backend, void *capture_buffer, int bufsiz)
{
  switch (backend->type)
    {
    case AL_BACKEND_NATIVE_:
      return capture_nativedevice (backend->privateData, capture_buffer,
                                   bufsiz);
    case AL_BACKEND_ALSA_:
      return capture_alsa (backend->privateData, capture_buffer, bufsiz);
    case AL_BACKEND_ARTS_:
      return capture_arts (backend->privateData, capture_buffer, bufsiz);
    case AL_BACKEND_DMEDIA_:
      return capture_dmedia (backend->privateData, capture_buffer, bufsiz);
    case AL_BACKEND_ESD_:
      return capture_esd (backend->privateData, capture_buffer, bufsiz);
    case AL_BACKEND_SDL_:
      return capture_sdl (backend->privateData, capture_buffer, bufsiz);
    case AL_BACKEND_NULL_:
      return capture_null (backend->privateData, capture_buffer, bufsiz);
    case AL_BACKEND_WAVEOUT_:
      return capture_waveout (backend->privateData, capture_buffer, bufsiz);
    default:
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                "alcBackendRead_: unknown backend %d\n", backend->type);
      return 0;
    }
}

ALfloat
alcBackendGetAudioChannel_ (ALC_Backend *backend, ALuint channel)
{
  switch (backend->type)
    {
    case AL_BACKEND_NATIVE_:
      return get_nativechannel (backend->privateData, channel);
    case AL_BACKEND_ALSA_:
      return get_alsachannel (backend->privateData, channel);
    case AL_BACKEND_ARTS_:
      return get_artschannel (backend->privateData, channel);
    case AL_BACKEND_DMEDIA_:
      return get_dmediachannel (backend->privateData, channel);
    case AL_BACKEND_ESD_:
      return get_esdchannel (backend->privateData, channel);
    case AL_BACKEND_SDL_:
      return get_sdlchannel (backend->privateData, channel);
    case AL_BACKEND_NULL_:
      return get_nullchannel (backend->privateData, channel);
    case AL_BACKEND_WAVEOUT_:
      return get_waveoutchannel (backend->privateData, channel);
    default:
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                "alcBackendGetAudioChannel_: unknown backend %d\n",
                backend->type);
      return 0;
    }
}

void
alcBackendSetAudioChannel_ (ALC_Backend *backend, ALuint channel,
                            ALfloat volume)
{
  switch (backend->type)
    {
    case AL_BACKEND_NATIVE_:
      set_nativechannel (backend->privateData, channel, volume);
      break;
    case AL_BACKEND_ALSA_:
      set_alsachannel (backend->privateData, channel, volume);
      break;
    case AL_BACKEND_ARTS_:
      set_artschannel (backend->privateData, channel, volume);
      break;
    case AL_BACKEND_DMEDIA_:
      set_dmediachannel (backend->privateData, channel, volume);
      break;
    case AL_BACKEND_ESD_:
      set_esdchannel (backend->privateData, channel, volume);
      break;
    case AL_BACKEND_SDL_:
      set_sdlchannel (backend->privateData, channel, volume);
      break;
    case AL_BACKEND_NULL_:
      set_nullchannel (backend->privateData, channel, volume);
      break;
    case AL_BACKEND_WAVEOUT_:
      set_waveoutchannel (backend->privateData, channel, volume);
      break;
    default:
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                "alcBackendSetAudioChannel_: unknown backend %d\n",
                backend->type);
      break;
    }
}
