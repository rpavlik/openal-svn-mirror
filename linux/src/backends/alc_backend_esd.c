/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * esd.c
 *
 * esd backend.
 */
#include "al_siteconfig.h"

#include <AL/al.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "al_main.h"
#include "al_debug.h"
#include "backends/alc_backend.h"
#include "alc/alc_context.h"

#include <esd.h>

#define DEF_SPEED       _ALC_CANON_SPEED
#define DEF_SIZE        _AL_DEF_BUFSIZ
#define DEF_SAMPLES     (DEF_SIZE / 2)
#define DEF_CHANNELS    2
#define DEF_FORMAT      AL_FORMAT_STEREO16

#define ESD_LIBRARY    "libesd.so"
#define ESD_KEY        "openal"
#define ESD_NAMELEN    1024

/*
 * ESD library functions.
 */
static int (*pesd_open_sound) (const char *host);
static int (*pesd_play_stream) (esd_format_t format, int rate,
                                const char *host, const char *name);
static int (*pesd_standby) (int esd);
static int (*pesd_resume) (int esd);
static int (*pesd_close) (int esd);

#ifdef OPENAL_DLOPEN_ESD
#include <dlfcn.h>
#define myDlopen(n,f) dlopen((n),(f))
#define myDlerror() dlerror()
#define myDlsym(h,s) dlsym((h),#s)
#define myDlclose(h) dlclose(h)
#else
#define myDlopen(n,f) ((void *)0xF00DF00D)
#define myDlerror() ""
#define myDlsym(h,s) (&s)
#define myDlclose(h)
#endif

#define OPENAL_LOAD_ESD_SYMBOL(h,s)  \
  p##s = myDlsym(h, s); \
  if (p##s == NULL) \
    { \
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__, \
                "could not resolve symbol '%s': %s", \
                #s, myDlerror ()); \
      myDlclose (h); \
      h = NULL; \
      return 0; \
    }

static int
loadLibraryESD (void)
{
  static void *handle = NULL;

  /* already loaded? */
  if (handle != NULL)
    {
      return 1;
    }

  /* clear error state */
  (void) myDlerror ();

  handle = myDlopen (ESD_LIBRARY, RTLD_LAZY | RTLD_GLOBAL);
  if (handle == NULL)
    {
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                "could not open '%s': %s", ESD_LIBRARY, myDlerror ());
      return 0;
    }

  OPENAL_LOAD_ESD_SYMBOL (handle, esd_open_sound);
  OPENAL_LOAD_ESD_SYMBOL (handle, esd_standby);
  OPENAL_LOAD_ESD_SYMBOL (handle, esd_resume);
  OPENAL_LOAD_ESD_SYMBOL (handle, esd_play_stream);
  OPENAL_LOAD_ESD_SYMBOL (handle, esd_close);

  _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
            "%s successfully loaded", ESD_LIBRARY);

  return 1;
}

typedef struct ALC_BackendDataESD_struct
{
  ALC_OpenMode mode;
  const char *espeaker;
  int esdhandle;
  esd_format_t fmt;
  ALuint speed;
  char name[ESD_NAMELEN];
  int socket;
  ALboolean paused;
} ALC_BackendDataESD;

static ALC_BackendPrivateData *
openESD (ALC_OpenMode mode)
{
  ALC_BackendDataESD *esd_info;

  if (mode == ALC_OPEN_INPUT_)
    {
      /* input mode not supported */
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                "esd backend does not support input");
      return NULL;
    }

  if (!loadLibraryESD ())
    {
      return NULL;
    }

  esd_info = (ALC_BackendDataESD *) malloc (sizeof (ALC_BackendDataESD));
  if (esd_info == NULL)
    {
      return NULL;
    }

  esd_info->paused = AL_FALSE;
  esd_info->mode = mode;
  esd_info->espeaker = getenv ("ESPEAKER");
  esd_info->esdhandle = pesd_open_sound (esd_info->espeaker);
  if (esd_info->esdhandle < 0)
    {
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__, "esd_open_sound failed");
      free (esd_info);
      return NULL;
    }

  esd_info->fmt = ESD_STREAM | ESD_PLAY;

  switch (DEF_CHANNELS)
    {
    case 1:
      esd_info->fmt |= ESD_MONO;
      break;
    case 2:
      esd_info->fmt |= ESD_STEREO;
      break;
    default:
      break;
    }

  switch (_alGetBitsFromFormat (DEF_FORMAT))
    {
    case 8:
      esd_info->fmt |= ESD_BITS8;
      break;
    case 16:
      esd_info->fmt |= ESD_BITS16;
      break;
    default:
      break;
    }

  esd_info->speed = DEF_SPEED;
  snprintf (esd_info->name, sizeof (esd_info->name), "openal-%d\n",
            (int) getpid ());
  esd_info->socket =
    pesd_play_stream (esd_info->fmt, DEF_SPEED, esd_info->espeaker,
                      esd_info->name);
  if (esd_info->socket < 0)
    {
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__, "esd_play_stream failed");
      free (esd_info);
      return NULL;
    }

  _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
            "ESD device successfully opened");
  return esd_info;
}

static void
closeESD (void *privateData)
{
  ALC_BackendDataESD *eh = (ALC_BackendDataESD *) privateData;
  pesd_close (eh->esdhandle);
  free (privateData);
  _alDebug (ALD_CONTEXT, __FILE__, __LINE__, "ESD device closed");
}

static void
pauseESD (void *privateData)
{
  ALC_BackendDataESD *eh = (ALC_BackendDataESD *) privateData;
  eh->paused = AL_TRUE;
  pesd_standby (eh->esdhandle);
}

static void
resumeESD (void *privateData)
{
  ALC_BackendDataESD *eh = (ALC_BackendDataESD *) privateData;
  eh->paused = AL_FALSE;
  pesd_resume (eh->esdhandle);
}

static ALboolean
setAttributesESD (void *privateData, UNUSED (ALuint *bufferSize),
                  ALenum *format, ALuint *speed)
{
  ALC_BackendDataESD *eh = (ALC_BackendDataESD *) privateData;

  close (eh->socket);

  eh->paused = AL_FALSE;

  eh->fmt = ESD_STREAM | ESD_PLAY;

  switch (_alGetChannelsFromFormat (*format))
    {
    case 1:
      eh->fmt |= ESD_MONO;
      break;
    case 2:
      eh->fmt |= ESD_STEREO;
      break;
    default:
      break;
    }

  switch (_alGetBitsFromFormat (*format))
    {
    case 8:
      eh->fmt |= ESD_BITS8;
      break;
    case 16:
      eh->fmt |= ESD_BITS16;
      break;
    default:
      break;
    }

  eh->speed = *speed;

  eh->socket = pesd_play_stream (eh->fmt, eh->speed, eh->espeaker, eh->name);
  if (eh->socket < 0)
    {
      return AL_FALSE;
    }

  return AL_TRUE;
}

static void
writeESD (void *privateData, const void *data, int size)
{
  fd_set esd_fd_set;
  ALC_BackendDataESD *eh;
  struct timeval tv = { 0, 9000000 };
  int iterator = 0;
  int err;
  SELECT_TYPE_ARG1 fd;

  if (privateData == NULL)
    {
      return;
    }

  eh = (ALC_BackendDataESD *) privateData;

  if (eh->paused == AL_TRUE)
    {
      /* don't write to paused audio devices, just sleep */
      tv.tv_usec = 10000;

      select (0, NULL, NULL, NULL, SELECT_TYPE_ARG5 &tv);

      return;
    }

  fd = eh->socket;

  for (iterator = size; iterator > 0;)
    {
      FD_ZERO (&esd_fd_set);
      FD_SET (fd, &esd_fd_set);

      err =
        select (fd + 1, NULL, SELECT_TYPE_ARG234 &esd_fd_set, NULL,
                SELECT_TYPE_ARG5 &tv);
      if (FD_ISSET (fd, &esd_fd_set) == 0)
        {
          /* timeout occured, don't try and write */
          _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                    "writeESD: timeout occured");
          return;
        }

      err = write (fd, (const char *) data + size - iterator, iterator);
      if (err < 0)
        {
          _alDebug (ALD_CONTEXT, __FILE__, __LINE__, "writeESD: error %d\n",
                    errno);
          return;
        }

      iterator -= err;
    };
}

static ALsizei
readESD (UNUSED (void *privateData), UNUSED (void *data), UNUSED (int size))
{
  return 0;
}

static ALfloat
getAudioChannelESD (UNUSED (void *privateData), UNUSED (ALuint channel))
{
  return 0.0f;
}

static int
setAudioChannelESD (UNUSED (void *privateData), UNUSED (ALuint channel),
                    UNUSED (ALfloat volume))
{
  return 0;
}

static ALC_BackendOps esdOps = {
  openESD,
  closeESD,
  pauseESD,
  resumeESD,
  setAttributesESD,
  writeESD,
  readESD,
  getAudioChannelESD,
  setAudioChannelESD
};

ALC_BackendOps *
alcGetBackendOpsESD_ (void)
{
  return &esdOps;
}
