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
  AL_BACKEND_ESD_,              /* ESD backend */
  AL_BACKEND_SDL_,              /* SDL backend */
  AL_BACKEND_NULL_,             /* null backend */
  AL_BACKEND_WAVEOUT_           /* WAVE backend */
} ALC_BackendType;

/* represents which backend we are using */
static ALC_BackendType hardware_type = AL_BACKEND_NONE_;

void *
alcBackendOpen_ (ALC_OpenMode mode)
{
  Rcvar device_params;
  Rcvar device_list;
  Rcvar device;
  void *retval = NULL;
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
          retval = alcBackendOpenNative_ (mode);
          if (retval != NULL)
            {
              hardware_type = AL_BACKEND_NATIVE_;
              return retval;
            }
        }
      if (strcmp (adevname, "native") == 0)
        {
          retval = alcBackendOpenNative_ (mode);
          if (retval != NULL)
            {
              hardware_type = AL_BACKEND_NATIVE_;
              return retval;
            }
        }

      if (strcmp (adevname, "alsa") == 0)
        {
          retval = alcBackendOpenALSA_ (mode);
          if (retval != NULL)
            {
              hardware_type = AL_BACKEND_ALSA_;
              return retval;
            }
        }

      if (strcmp (adevname, "arts") == 0)
        {
          retval = alcBackendOpenARts_ (mode);
          if (retval != NULL)
            {
              hardware_type = AL_BACKEND_ARTS_;
              return retval;
            }
        }

      if (strcmp (adevname, "esd") == 0)
        {
          retval = alcBackendOpenESD_ (mode);
          if (retval != NULL)
            {
              hardware_type = AL_BACKEND_ESD_;
              return retval;
            }
        }

      if (strcmp (adevname, "sdl") == 0)
        {
          retval = alcBackendOpenSDL_ (mode);
          if (retval != NULL)
            {
              hardware_type = AL_BACKEND_SDL_;
              return retval;
            }
        }

      if (strcmp (adevname, "null") == 0)
        {
          retval = alcBackendOpenNull_ (mode);
          if (retval != NULL)
            {
              hardware_type = AL_BACKEND_NULL_;
              return retval;
            }
        }

      if (strcmp (adevname, "waveout") == 0)
        {
          retval = alcBackendOpenWAVE_ (mode);
          if (retval != NULL)
            {
              hardware_type = AL_BACKEND_WAVEOUT_;
              return retval;
            }
        }
    }

  /* no device list specified, try native or fail */
  retval = alcBackendOpenNative_ (mode);
  if (retval != NULL)
    {
      hardware_type = AL_BACKEND_NATIVE_;
      return retval;
    }

  return NULL;
}

ALboolean
alcBackendClose_ (void *handle)
{
  switch (hardware_type)
    {
    case AL_BACKEND_NATIVE_:
      release_native (handle);
      return AL_TRUE;
    case AL_BACKEND_ALSA_:
      release_alsa (handle);
      return AL_TRUE;
    case AL_BACKEND_ARTS_:
      release_arts (handle);
      return AL_TRUE;
    case AL_BACKEND_ESD_:
      release_esd (handle);
      return AL_TRUE;
    case AL_BACKEND_SDL_:
      release_sdl (handle);
      return AL_TRUE;
    case AL_BACKEND_NULL_:
      release_null (handle);
      return AL_TRUE;
    case AL_BACKEND_WAVEOUT_:
      release_waveout (handle);
      return AL_TRUE;
    default:
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                "alcBackendClose_: unknown backend %d\n", hardware_type);
      return AL_FALSE;
    }

}

void
alcBackendPause_ (void *handle)
{
  switch (hardware_type)
    {
    case AL_BACKEND_NATIVE_:
      pause_nativedevice (handle);
      break;
    case AL_BACKEND_ALSA_:
      pause_alsa (handle);
      break;
    case AL_BACKEND_ARTS_:
      pause_arts (handle);
      break;
    case AL_BACKEND_ESD_:
      pause_esd (handle);
      break;
    case AL_BACKEND_SDL_:
      pause_sdl (handle);
      break;
    case AL_BACKEND_NULL_:
      pause_null (handle);
      break;
    case AL_BACKEND_WAVEOUT_:
      pause_waveout (handle);
      break;
    default:
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                "alcBackendPause_: unknown backend %d\n", hardware_type);
      break;
    }
}

void
alcBackendResume_ (void *handle)
{
  switch (hardware_type)
    {
    case AL_BACKEND_NATIVE_:
      resume_nativedevice (handle);
      break;
    case AL_BACKEND_ALSA_:
      resume_alsa (handle);
      break;
    case AL_BACKEND_ARTS_:
      resume_arts (handle);
      break;
    case AL_BACKEND_ESD_:
      resume_esd (handle);
      break;
    case AL_BACKEND_SDL_:
      resume_sdl (handle);
      break;
    case AL_BACKEND_NULL_:
      resume_null (handle);
      break;
    case AL_BACKEND_WAVEOUT_:
      resume_waveout (handle);
      break;
    default:
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                "alcBackendResume_: unknown backend %d\n", hardware_type);
      break;
    }
}

ALboolean
alcBackendSetAttributes_ (ALC_OpenMode mode, void *handle, ALuint *bufsiz,
                          ALenum *fmt, ALuint *speed)
{
  switch (hardware_type)
    {
    case AL_BACKEND_NATIVE_:
      return alcBackendSetAttributesNative_ (mode, handle, bufsiz, fmt,
                                             speed);
    case AL_BACKEND_ALSA_:
      return alcBackendSetAttributesALSA_ (mode, handle, bufsiz, fmt, speed);
    case AL_BACKEND_ARTS_:
      return alcBackendSetAttributesARts_ (mode, handle, bufsiz, fmt, speed);
    case AL_BACKEND_ESD_:
      return alcBackendSetAttributesESD_ (mode, handle, bufsiz, fmt, speed);
    case AL_BACKEND_SDL_:
      return alcBackendSetAttributesSDL_ (mode, handle, bufsiz, fmt, speed);
    case AL_BACKEND_NULL_:
      return alcBackendSetAttributesNull_ (mode, handle, bufsiz, fmt, speed);
    case AL_BACKEND_WAVEOUT_:
      return alcBackendSetAttributesWAVE_ (mode, handle, bufsiz, fmt, speed);
    default:
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                "alcBackendSetWrite_: unknown backend %d\n", hardware_type);
      return AL_FALSE;
    }
}

void
alcBackendWrite_ (void *handle, void *dataptr, int bytes_to_write)
{
  switch (hardware_type)
    {
    case AL_BACKEND_NATIVE_:
      native_blitbuffer (handle, dataptr, bytes_to_write);
      break;
    case AL_BACKEND_ALSA_:
      alsa_blitbuffer (handle, dataptr, bytes_to_write);
      break;
    case AL_BACKEND_ARTS_:
      arts_blitbuffer (handle, dataptr, bytes_to_write);
      break;
    case AL_BACKEND_ESD_:
      esd_blitbuffer (handle, dataptr, bytes_to_write);
      break;
    case AL_BACKEND_SDL_:
      sdl_blitbuffer (handle, dataptr, bytes_to_write);
      break;
    case AL_BACKEND_NULL_:
      null_blitbuffer (handle, dataptr, bytes_to_write);
      break;
    case AL_BACKEND_WAVEOUT_:
      waveout_blitbuffer (handle, dataptr, bytes_to_write);
      break;
    default:
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                "alcBackendWrite_: unknown backend %d\n", hardware_type);
      break;
    }
}

ALsizei
alcBackendRead_ (void *handle, void *capture_buffer, int bufsiz)
{
  switch (hardware_type)
    {
    case AL_BACKEND_NATIVE_:
      return capture_nativedevice (handle, capture_buffer, bufsiz);
    case AL_BACKEND_ALSA_:
      return capture_alsa (handle, capture_buffer, bufsiz);
    case AL_BACKEND_ARTS_:
      return capture_arts (handle, capture_buffer, bufsiz);
    case AL_BACKEND_ESD_:
      return capture_esd (handle, capture_buffer, bufsiz);
    case AL_BACKEND_SDL_:
      return capture_sdl (handle, capture_buffer, bufsiz);
    case AL_BACKEND_NULL_:
      return capture_null (handle, capture_buffer, bufsiz);
    case AL_BACKEND_WAVEOUT_:
      return capture_waveout (handle, capture_buffer, bufsiz);
    default:
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                "alcBackendRead_: unknown backend %d\n", hardware_type);
      return 0;
    }
}

ALfloat
alcBackendGetAudioChannel_ (void *handle, ALuint channel)
{
  switch (hardware_type)
    {
    case AL_BACKEND_NATIVE_:
      return get_nativechannel (handle, channel);
    case AL_BACKEND_ALSA_:
      return get_alsachannel (handle, channel);
    case AL_BACKEND_ARTS_:
      return get_artschannel (handle, channel);
    case AL_BACKEND_ESD_:
      return get_esdchannel (handle, channel);
    case AL_BACKEND_SDL_:
      return get_sdlchannel (handle, channel);
    case AL_BACKEND_NULL_:
      return get_nullchannel (handle, channel);
    case AL_BACKEND_WAVEOUT_:
      return get_waveoutchannel (handle, channel);
    default:
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                "alcBackendGetAudioChannel_: unknown backend %d\n",
                hardware_type);
      return 0;
    }
}

void
alcBackendSetAudioChannel_ (void *handle, ALuint channel, ALfloat volume)
{
  switch (hardware_type)
    {
    case AL_BACKEND_NATIVE_:
      set_nativechannel (handle, channel, volume);
      break;
    case AL_BACKEND_ALSA_:
      set_alsachannel (handle, channel, volume);
      break;
    case AL_BACKEND_ARTS_:
      set_artschannel (handle, channel, volume);
      break;
    case AL_BACKEND_ESD_:
      set_esdchannel (handle, channel, volume);
      break;
    case AL_BACKEND_SDL_:
      set_sdlchannel (handle, channel, volume);
      break;
    case AL_BACKEND_NULL_:
      set_nullchannel (handle, channel, volume);
      break;
    case AL_BACKEND_WAVEOUT_:
      set_waveoutchannel (handle, channel, volume);
      break;
    default:
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                "alcBackendSetAudioChannel_: unknown backend %d\n",
                hardware_type);
      break;
    }
}
