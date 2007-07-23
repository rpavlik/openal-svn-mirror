/*
 * This file defines the high-level interface to the sound device. It
 * actually dispatches requests based on the architecture that we're
 * targetting. See alc_backend.h for more of a clue.
 */
#include "al_siteconfig.h"

#include <AL/al.h>
#include <stdlib.h>
#include <string.h>

#include "config/al_config.h"
#include "al_debug.h"
#include "al_main.h"
#include "backends/alc_backend.h"

#define FALLBACK_DEVICE "native"

#define SPECIFIER_LENGTH 2048
static char device_specifier_list[SPECIFIER_LENGTH];
static const char *default_device_specifier;
static char capture_device_specifier_list[SPECIFIER_LENGTH];
static const char *default_capture_device_specifier;

struct ALC_BackendAliasAndName
{
  const char *alias;
  const char *name;
};

static struct ALC_BackendAliasAndName aliases[] = {
  {"dsp", "oss"},
  {"waveout", "wave"},
#if defined(USE_BACKEND_ALSA)
  {"native", "alsa"},
#elif defined(USE_BACKEND_OSS)
  {"native", "oss"},
#elif defined(USE_BACKEND_DMEDIA)
  {"native", "dmedia"},
#else
  {"native", "null"},
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
                                 struct ALC_BackendPrivateData **privateData);

struct ALC_BackendNameAndOpen
{
  const char *name;
  const ALC_BackendOpen open;
  const ALCchar *pretty_name;
};

struct ALC_BackendNameAndOpen backends[] = {
#ifdef USE_BACKEND_OSS
  {"oss", alcBackendOpenOSS_},
#endif
#ifdef USE_BACKEND_ALSA
  {"alsa", alcBackendOpenALSA_},
#endif
#ifdef USE_BACKEND_DMEDIA
  {"dmedia", alcBackendOpenDMedia_},
#endif
#ifdef USE_BACKEND_ESD
  {"esd", alcBackendOpenESD_},
#endif
#ifdef USE_BACKEND_SDL
  {"sdl", alcBackendOpenSDL_},
#endif
#ifdef USE_BACKEND_NULL
  {"null", alcBackendOpenNull_},
#endif
#ifdef USE_BACKEND_WAVEOUT
  {"wave", alcBackendOpenWAVE_},
#endif
  {NULL, NULL}
};

static void openNamedDevice(const char *deviceName, ALC_OpenMode mode,
                            ALC_BackendOps **ops,
                            struct ALC_BackendPrivateData **privateData)
{
    struct ALC_BackendNameAndOpen *current;
    const char *realName = lookupRealName(deviceName);

    for(current = backends; current->name != NULL; current++)
    {
        if((current->pretty_name &&
            strcmp(realName, current->pretty_name) == 0) ||
           strcmp(realName, current->name) == 0)
        {
            current->open(mode, ops, privateData);
            return;
        }
    }
    *ops = NULL;
}

void alcBackendOpen_(const ALCchar *name, ALC_OpenMode mode, ALC_BackendOps **ops,
                     struct ALC_BackendPrivateData **privateData)
{
  Rcvar device_params;
  Rcvar device_list;
  Rcvar device;
  char deviceName[64];          /* FIXME: magic number */

    /* Make sure the specifier list is filled */
    _alcGetSpecifierList(mode);

    *privateData = NULL;
    if(name) {
        openNamedDevice (name, mode, ops, privateData);
        /* If a name was given, we must fail or succeed with it */
        return;
    }

    device_list = rc_lookup ("devices");
    if(device_list == NULL)
        openNamedDevice (FALLBACK_DEVICE, mode, ops, privateData);

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
                    "alcBackendOpen_: bad type for device");
          continue;
        }
      openNamedDevice (deviceName, mode, ops, privateData);
    }
}


const ALCchar *_alcGetDefaultSpecifier(ALC_OpenMode mode)
{
    const char **DefSpecifier = ((mode==ALC_OPEN_OUTPUT_) ?
                                 &default_device_specifier :
                                 &default_capture_device_specifier);
    const char *name;
    const char *list;
    int i;

    if(*DefSpecifier)
        return *DefSpecifier;

    /* This should hopefully set the defualt specifier */
    list = _alcGetSpecifierList(mode);

    /* If still nothing, just use the first in the list */
    if(!*DefSpecifier)
        *DefSpecifier = list;

    return *DefSpecifier;
}

const ALCchar *_alcGetSpecifierList(ALC_OpenMode mode)
{
    struct ALC_BackendNameAndOpen *current;
    struct ALC_BackendPrivateData *privateData;
    ALC_BackendOps *ops;
    char *name_ptr, *end_ptr;
    const char **default_ptr;
    Rcvar device_params;
    Rcvar device_list;
    Rcvar device;
    char deviceName[64] = FALLBACK_DEVICE;   /* FIXME: magic number */

    if(mode == ALC_OPEN_OUTPUT_) {
        name_ptr = device_specifier_list;
        end_ptr = device_specifier_list + SPECIFIER_LENGTH - 2;
        default_ptr = &default_device_specifier;
    }
    else {
        name_ptr = capture_device_specifier_list;
        end_ptr = capture_device_specifier_list + SPECIFIER_LENGTH - 2;
        default_ptr = &default_capture_device_specifier;
    }

    if(name_ptr[0])
        return name_ptr;

    /* Override the "native" default with the first device in 'devices' */
    _alParseConfig();
    device_list = rc_lookup("devices");
    while(device_list != NULL)
    {
        device = rc_car(device_list);
        device_list = rc_cdr(device_list);

        switch (rc_type(device))
        {
            case ALRC_STRING:
                rc_tostr0(device, deviceName, 64);
                device_list = NULL;
                break;
            case ALRC_SYMBOL:
                rc_symtostr0(device, deviceName, 64);
                device_list = NULL;
                break;
            case ALRC_CONSCELL:
                device_params = rc_cdr(device);
                if(device_params == NULL)
                   continue;
                rc_define("device-params", device_params);
                rc_symtostr0(rc_car(device), deviceName, 64);
                device_list = NULL;
                break;
            default:
                continue;
        }
    }

    for(current = backends;current->name && name_ptr < end_ptr;current++) {
        privateData = NULL;
        current->open(mode, &ops, &privateData);
        if(privateData) {
            const ALCchar *cur_name = ops->getName(privateData);
            ops->close(privateData);

            current->pretty_name = cur_name;
            strncpy(name_ptr, cur_name, (size_t)(end_ptr-name_ptr));

            if(!(*default_ptr) && strcmp(lookupRealName(deviceName),
                                         current->name) == 0)
                *default_ptr = name_ptr;

            name_ptr += strlen(name_ptr)+1;
        }
    }

    return ((mode == ALC_OPEN_OUTPUT_) ? device_specifier_list :
                                         capture_device_specifier_list);
}
