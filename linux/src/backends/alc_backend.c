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

void
alcBackendOpen_ (ALC_OpenMode mode, ALC_BackendOps **theOps,
                 ALC_BackendPrivateData **thePrivateData)
{
  Rcvar device_params;
  Rcvar device_list;
  Rcvar device;
  ALC_BackendOps *ops = NULL;
  ALC_BackendPrivateData *privateData = NULL;
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
          ops = alcGetBackendOpsNative_ ();
          if (ops != NULL)
            {
              privateData = ops->open (mode);
              if (privateData != NULL)
                {
                  break;
                }
            }
        }
      if (strcmp (adevname, "native") == 0)
        {
          ops = alcGetBackendOpsNative_ ();
          if (ops != NULL)
            {
              privateData = ops->open (mode);
              if (privateData != NULL)
                {
                  break;
                }
            }
        }

      if (strcmp (adevname, "alsa") == 0)
        {
          ops = alcGetBackendOpsALSA_ ();
          if (ops != NULL)
            {
              privateData = ops->open (mode);
              if (privateData != NULL)
                {
                  break;
                }
            }
        }

      if (strcmp (adevname, "arts") == 0)
        {
          ops = alcGetBackendOpsARts_ ();
          if (ops != NULL)
            {
              privateData = ops->open (mode);
              if (privateData != NULL)
                {
                  break;
                }
            }
        }

      if (strcmp (adevname, "dmedia") == 0)
        {
          ops = alcGetBackendOpsDMedia_ ();
          if (ops != NULL)
            {
              privateData = ops->open (mode);
              if (privateData != NULL)
                {
                  break;
                }
            }
        }

      if (strcmp (adevname, "esd") == 0)
        {
          ops = alcGetBackendOpsESD_ ();
          if (ops != NULL)
            {
              privateData = ops->open (mode);
              if (privateData != NULL)
                {
                  break;
                }
            }
        }

      if (strcmp (adevname, "sdl") == 0)
        {
          ops = alcGetBackendOpsSDL_ ();
          if (ops != NULL)
            {
              privateData = ops->open (mode);
              if (privateData != NULL)
                {
                  break;
                }
            }
        }

      if (strcmp (adevname, "null") == 0)
        {
          ops = alcGetBackendOpsNull_ ();
          if (ops != NULL)
            {
              privateData = ops->open (mode);
              if (privateData != NULL)
                {
                  break;
                }
            }
        }

      if (strcmp (adevname, "waveout") == 0)
        {
          ops = alcGetBackendOpsWAVE_ ();
          if (ops != NULL)
            {
              privateData = ops->open (mode);
              if (privateData != NULL)
                {
                  break;
                }
            }
        }
    }

  /* fallback: native */
  if (privateData == NULL)
    {
      ops = alcGetBackendOpsNative_ ();
      if (ops == NULL)
        {
          *theOps = NULL;
          return;
        }
      privateData = ops->open (mode);
      if (privateData == NULL)
        {
          *theOps = NULL;
          return;
        }
    }

  *theOps = ops;
  *thePrivateData = privateData;
}
