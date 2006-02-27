/*
 * EsounD backend. Note that apart from the actual sources of libesd itself
 * there is very little documentation available, so let's hope for the best...
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

#include "al_debug.h"
#include "al_main.h"
#include "backends/alc_backend.h"

#include <esd.h>

#define ESD_LIBRARY "libesd.so"

/*
 * ESD library functions.
 */
static int (*pesd_play_stream) (esd_format_t format, int rate,
                                const char *host, const char *name);
static int (*pesd_record_stream) (esd_format_t format, int rate,
                                  const char *host, const char *name);
static int (*pesd_close) (int esd);
static int (*pesd_standby) (int esd);
static int (*pesd_resume) (int esd);

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

  OPENAL_LOAD_ESD_SYMBOL (handle, esd_play_stream);
  OPENAL_LOAD_ESD_SYMBOL (handle, esd_record_stream);
  OPENAL_LOAD_ESD_SYMBOL (handle, esd_close);
  OPENAL_LOAD_ESD_SYMBOL (handle, esd_standby);
  OPENAL_LOAD_ESD_SYMBOL (handle, esd_resume);

  _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
            "%s successfully loaded", ESD_LIBRARY);

  return 1;
}

#define INVALID_SOCKET (-1)

typedef struct ALC_BackendDataESD_struct
{
  ALC_OpenMode mode;            /* Is the device in input mode or output mode? */
  ALboolean paused;             /* Is the device paused? */
  int esd;                      /* the socket connected to the EsounD daemon */
  char name[ESD_NAME_MAX];      /* the name of the connection, openal-PID */
} ALC_BackendDataESD;

/*
 * Note that we can use esd_play_stream only when we know the format and speed
 * of the data to come, so we can't actually do much in openESD itself. The real
 * work happens in setAttributesESD below.
 */
static ALC_BackendPrivateData *
openESD (ALC_OpenMode mode)
{
  ALC_BackendDataESD *eh;

  _alDebug (ALD_CONTEXT, __FILE__, __LINE__, "opening ESD device");

  if (!loadLibraryESD ())
    {
      return NULL;
    }

  eh = (ALC_BackendDataESD *) malloc (sizeof (ALC_BackendDataESD));
  if (eh == NULL)
    {
      return NULL;
    }

  eh->mode = mode;
  eh->paused = AL_FALSE;
  eh->esd = INVALID_SOCKET;
  snprintf (eh->name, sizeof (eh->name), "openal-%d", (int) getpid ());

  _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
            "ESD device '%s' successfully opened", eh->name);
  return eh;
}

static void
closeESD (void *privateData)
{
  ALC_BackendDataESD *eh = (ALC_BackendDataESD *) privateData;
  _alDebug (ALD_CONTEXT, __FILE__, __LINE__, "closing ESD device '%s'",
            eh->name);
  pesd_close (eh->esd);
  free (privateData);
}

static void
pauseESD (void *privateData)
{
  ALC_BackendDataESD *eh = (ALC_BackendDataESD *) privateData;
  _alDebug (ALD_CONTEXT, __FILE__, __LINE__, "pausing ESD device '%s'",
            eh->name);
  eh->paused = AL_TRUE;
  /* pesd_standby (eh->esd); */
}

static void
resumeESD (void *privateData)
{
  ALC_BackendDataESD *eh = (ALC_BackendDataESD *) privateData;
  _alDebug (ALD_CONTEXT, __FILE__, __LINE__, "resuming ESD device, '%s'",
            eh->name);
  eh->paused = AL_FALSE;
  /* pesd_resume (eh->esd); */
}

static ALboolean
setAttributesESD (void *privateData, ALuint *bufferSize, ALenum *format,
                  ALuint *speed)
{
  ALC_BackendDataESD *eh = (ALC_BackendDataESD *) privateData;
  esd_format_t esd_format;
  _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
            "setting attributes for ESD device '%s': buffer size %u, format 0x%x, speed %d",
            eh->name, (unsigned int) *bufferSize, (int) *format,
            (unsigned int) *speed);

  /*
   * Open the socket to the EsounD daemon only once, i.e. at the first time
   * alcMakeContextCurrent(NON_NULL) is called.
   */
  if (eh->esd != INVALID_SOCKET)
    {
      return AL_TRUE;
    }

  esd_format = ESD_STREAM;

  switch (_alGetChannelsFromFormat (*format))
    {
    case 1:
      esd_format |= ESD_MONO;
      break;
    case 2:
      esd_format |= ESD_STEREO;
      break;
    default:
      return AL_FALSE;
    }

  switch (_alGetBitsFromFormat (*format))
    {
    case 8:
      esd_format |= ESD_BITS8;
      break;
    case 16:
      esd_format |= ESD_BITS16;
      break;
    default:
      return AL_FALSE;
    }

  if (eh->mode == ALC_OPEN_INPUT_)
    {
      esd_format |= ESD_RECORD;
      eh->esd = pesd_record_stream (esd_format, *speed, NULL, eh->name);
    }
  else
    {
      esd_format |= ESD_PLAY;
      eh->esd = pesd_play_stream (esd_format, *speed, NULL, eh->name);
    }
  return (eh->esd < 0) ? AL_FALSE : AL_TRUE;
}

static void
writeESD (void *privateData, const void *data, int size)
{
  fd_set esd_fd_set;
  ALC_BackendDataESD *eh = (ALC_BackendDataESD *) privateData;
  struct timeval tv = { 0, 800000 };    /* at most .8 secs */
  int bytesToWrite = size;
  int bytesWritten;
  SELECT_TYPE_ARG1 fd;

  _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
            "writing %d bytes to ESD device '%s'", size, eh->name);

  if (eh->paused == AL_TRUE)
    {
      /*
       * Don't write to paused audio devices, just sleep 10ms.
       * ToDo: Why do we do this??
       */
      tv.tv_usec = 10000;
      select (0, NULL, NULL, NULL, SELECT_TYPE_ARG5 &tv);
      return;
    }

  fd = eh->esd;
  while (bytesToWrite > 0)
    {
      FD_ZERO (&esd_fd_set);
      FD_SET (fd, &esd_fd_set);

      /* ToDo: Handle error */
      select (fd + 1, NULL, SELECT_TYPE_ARG234 &esd_fd_set, NULL,
              SELECT_TYPE_ARG5 &tv);

      if (FD_ISSET (fd, &esd_fd_set) == 0)
        {
          /* timeout occured, don't try and write */
          _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                    "writing timed out for ESD device '%s'", eh->name);
          return;
        }

      bytesWritten =
        write (fd, (const char *) data + size - bytesToWrite, bytesToWrite);
      if (bytesWritten < 0)
        {
          _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
                    "writing failed for ESD device '%s', error %d", eh->name,
                    errno);
          return;
        }

      bytesToWrite -= bytesWritten;
    }
}

/*
 * ToDo: Implement!
 */
static ALsizei
readESD (void *privateData, UNUSED (void *data), int size)
{
  ALC_BackendDataESD *eh = (ALC_BackendDataESD *) privateData;
  _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
            "reading up to %d bytes from ESD device '%s', %d bytes actually read",
            size, eh->name, 0);
  return 0;
}

/*
 * ToDo: Can this be implemented at all?
 */
static ALfloat
getAudioChannelESD (void *privateData, ALuint channel)
{
  ALC_BackendDataESD *eh = (ALC_BackendDataESD *) privateData;
  _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
            "getting volume for ESD device '%s', channel %u: %f", eh->name,
            (unsigned int) channel, 0.0);
  return 0.0f;
}

/*
 * ToDo: Can this be implemented via esd_set_stream_pan?
 */
static int
setAudioChannelESD (void *privateData, ALuint channel, ALfloat volume)
{
  ALC_BackendDataESD *eh = (ALC_BackendDataESD *) privateData;
  _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
            "setting volume for ESD device '%s', channel %u to %f", eh->name,
            (unsigned int) channel, (double) volume);
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
