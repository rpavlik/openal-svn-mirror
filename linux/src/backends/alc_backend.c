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

#define FALLBACK_DEVICE "native"

struct ALC_BackendAliasAndName
{
  const char *alias;
  const char *name;
};

static struct ALC_BackendAliasAndName aliases[] = {
  {"dsp", "native"},
  {"waveout", "wave"},
#ifdef USE_BACKEND_DMEDIA
  {"native", "dmedia"},
#endif
  {NULL, NULL}
};

static const char *
lookupRealName (const char *deviceName)
{
  struct ALC_BackendAliasAndName *current;
  for (current = aliases; current->alias != NULL; current++)
    {
      if (strcmp (deviceName, current->alias) == 0)
        {
          return current->name;
        }
    }
  return deviceName;
}

typedef void (*ALC_BackendOpen) (ALC_OpenMode mode, ALC_BackendOps **ops,
                                 ALC_BackendPrivateData **privateData);

struct ALC_BackendNameAndOpen
{
  const char *name;
  const ALC_BackendOpen open;
};

struct ALC_BackendNameAndOpen backends[] = {
#ifndef USE_BACKEND_DMEDIA
  {"native", alcBackendOpenNative_},
#endif
  {"alsa", alcBackendOpenALSA_},
  {"arts", alcBackendOpenARts_},
  {"dmedia", alcBackendOpenDMedia_},
  {"esd", alcBackendOpenESD_},
  {"sdl", alcBackendOpenSDL_},
  {"null", alcBackendOpenNull_},
  {"wave", alcBackendOpenWAVE_},
  {NULL, NULL}
};

static void
openNamedDevice (const char *deviceName, ALC_OpenMode mode,
                 ALC_BackendOps **ops, ALC_BackendPrivateData **privateData)
{
  struct ALC_BackendNameAndOpen *current;
  const char *realName = lookupRealName (deviceName);
  for (current = backends; current->name != NULL; current++)
    {
      if (strcmp (realName, current->name) == 0)
        {
          current->open (mode, ops, privateData);
	  return;
        }
    }
  *ops = NULL;
}

void
alcBackendOpen_ (ALC_OpenMode mode, ALC_BackendOps **ops,
                 ALC_BackendPrivateData **privateData)
{
  Rcvar device_params;
  Rcvar device_list;
  Rcvar device;
  char deviceName[64];          /* FIXME: magic number */

  device_list = rc_lookup ("devices");
  *privateData = NULL;
  while ((device_list != NULL) && (*privateData == NULL))
    {
      device = rc_car (device_list);
      device_list = rc_cdr (device_list);

      switch (rc_type (device))
        {
        case ALRC_STRING:
          rc_tostr0 (device, deviceName, 64);
          break;
        case ALRC_SYMBOL:
          rc_symtostr0 (device, deviceName, 64);
          break;
        case ALRC_CONSCELL:
          device_params = rc_cdr (device);
          if (device_params == NULL)
            {
              continue;
            }
          rc_define ("device-params", device_params);
          rc_symtostr0 (rc_car (device), deviceName, 64);
          break;
        default:
          _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                    "alcBackendOpen_: bad type %s for device",
                    rc_typestr (rc_type (device)));
          continue;
        }
      openNamedDevice (deviceName, mode, ops, privateData);
    }

  if (*privateData == NULL)
    {
      openNamedDevice (FALLBACK_DEVICE, mode, ops, privateData);
    }
}
