/*
 * ALSA backend for OpenAL
 *
 * Initial version by Dirk Ehlke <dehlke@mip.informatik.uni-kiel.de>,
 * Multimedia Information Processing, Christian-Albrechts-University of Kiel
 *
 * Almost complete rewrite by Sven Panne <sven.panne@aedion.de>
 *
 */

#ifndef _SVID_SOURCE
#define _SVID_SOURCE
#endif /* _SVID_SOURCE */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */

#include "al_siteconfig.h"
#include <stdlib.h>
#include "backends/alc_backend.h"

#ifndef USE_BACKEND_ALSA

void
alcBackendOpenALSA_ (UNUSED (ALC_OpenMode mode),
                     UNUSED (ALC_BackendOps **ops),
                     struct ALC_BackendPrivateData **privateData)
{
  *privateData = NULL;
}

#else

#include <AL/alext.h>

#include "config/al_config.h"
#include "al_debug.h"
#include "al_main.h"
#include "al_dlopen.h"

#define DEFAULT_DEVICE "default"

/*
 * NOTE: We exclusively use the new PCM API here, not the 0.9 one! dlsym will
 * always return the latest version, so let's hope that the ALSA API does not
 * get broken again. Symbol versioning is a hack!
 */
#include <alsa/asoundlib.h>

static int (*psnd_pcm_open) (snd_pcm_t **, const char *, snd_pcm_stream_t,
                             int);
static int (*psnd_pcm_close) (snd_pcm_t *);
static const char *(*psnd_strerror) (int);
static int (*psnd_pcm_nonblock) (snd_pcm_t *, int);
static int (*psnd_pcm_prepare) (snd_pcm_t *);
static int (*psnd_pcm_resume) (snd_pcm_t *);
static snd_pcm_sframes_t (*psnd_pcm_readi) (snd_pcm_t *, void *,
                                            snd_pcm_uframes_t);
static snd_pcm_sframes_t (*psnd_pcm_writei) (snd_pcm_t *, const void *,
                                             snd_pcm_uframes_t);
static size_t (*psnd_pcm_hw_params_sizeof) (void);
static int (*psnd_pcm_hw_params_malloc) (snd_pcm_hw_params_t **);
static void (*psnd_pcm_hw_params_free) (snd_pcm_hw_params_t *);
static int (*psnd_pcm_hw_params) (snd_pcm_t *, snd_pcm_hw_params_t *);
static int (*psnd_pcm_hw_params_any) (snd_pcm_t *, snd_pcm_hw_params_t *);
static int (*psnd_pcm_hw_params_set_access) (snd_pcm_t *,
                                             snd_pcm_hw_params_t *,
                                             snd_pcm_access_t);
static int (*psnd_pcm_hw_params_set_buffer_size_near) (snd_pcm_t *,
                                                       snd_pcm_hw_params_t *,
                                                       snd_pcm_uframes_t *);
static int (*psnd_pcm_hw_params_set_channels) (snd_pcm_t *,
                                               snd_pcm_hw_params_t *,
                                               unsigned int);
static int (*psnd_pcm_hw_params_set_format) (snd_pcm_t *,
                                             snd_pcm_hw_params_t *,
                                             snd_pcm_format_t);
static int (*psnd_pcm_hw_params_set_periods_near) (snd_pcm_t *,
                                                   snd_pcm_hw_params_t *,
                                                   unsigned int *, int *);
static int (*psnd_pcm_hw_params_set_rate_near) (snd_pcm_t *,
                                                snd_pcm_hw_params_t *,
                                                unsigned int *, int *);
static ssize_t (*psnd_pcm_format_size) (snd_pcm_format_t, size_t);

static ALboolean
getAPIEntriesALSA (void)
{
  static AL_DLHandle libHandle = (AL_DLHandle) 0;
#ifdef OPENAL_DLOPEN_ALSA
  const char *error = NULL;
#endif

  if (libHandle != (AL_DLHandle) 0)
    {
      /* already loaded. */
      return AL_TRUE;
    }

#ifdef OPENAL_DLOPEN_ALSA
#define AL_SYM_ALSA_(x, t)                          \
        p##x = t alDLFunSym_ (libHandle, #x); \
        error = alDLError_ (); \
        if (error != NULL) { \
                alDLClose_ (libHandle); \
                libHandle = (AL_DLHandle) 0; \
                return AL_FALSE; \
        }

  alDLError_ ();
  libHandle = alDLOpen_ ("libasound.so");
  error = alDLError_ ();
  if (error != NULL)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "could not open ALSA library: %s", error);
      return AL_FALSE;
    }

#else
#define AL_SYM_ALSA_(x, t) p##x = x;
  libHandle = (AL_DLHandle) 0xF00DF00D;
#endif

  AL_SYM_ALSA_ (snd_pcm_open,
                (int (*)(snd_pcm_t **, const char *, snd_pcm_stream_t, int)));
  AL_SYM_ALSA_ (snd_pcm_close, (int (*)(snd_pcm_t *)));
  AL_SYM_ALSA_ (snd_strerror, (const char *(*)(int)));
  AL_SYM_ALSA_ (snd_pcm_nonblock, (int (*)(snd_pcm_t *, int)));
  AL_SYM_ALSA_ (snd_pcm_prepare, (int (*)(snd_pcm_t *)));
  AL_SYM_ALSA_ (snd_pcm_resume, (int (*)(snd_pcm_t *)));
  AL_SYM_ALSA_ (snd_pcm_readi,
                (snd_pcm_sframes_t (*)
                 (snd_pcm_t *, void *, snd_pcm_uframes_t)));
  AL_SYM_ALSA_ (snd_pcm_writei,
                (snd_pcm_sframes_t (*)
                 (snd_pcm_t *, const void *, snd_pcm_uframes_t)));
  AL_SYM_ALSA_ (snd_pcm_hw_params_sizeof, (size_t (*)(void)));
  AL_SYM_ALSA_ (snd_pcm_hw_params_malloc, (int (*)(snd_pcm_hw_params_t **)));
  AL_SYM_ALSA_ (snd_pcm_hw_params_free, (void (*)(snd_pcm_hw_params_t *)));
  AL_SYM_ALSA_ (snd_pcm_hw_params,
                (int (*)(snd_pcm_t *, snd_pcm_hw_params_t *)));
  AL_SYM_ALSA_ (snd_pcm_hw_params_any,
                (int (*)(snd_pcm_t *, snd_pcm_hw_params_t *)));
  AL_SYM_ALSA_ (snd_pcm_hw_params_set_access,
                (int (*)
                 (snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_access_t)));
  AL_SYM_ALSA_ (snd_pcm_hw_params_set_buffer_size_near,
                (int (*)
                 (snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_uframes_t *)));
  AL_SYM_ALSA_ (snd_pcm_hw_params_set_channels,
                (int (*)(snd_pcm_t *, snd_pcm_hw_params_t *, unsigned int)));
  AL_SYM_ALSA_ (snd_pcm_hw_params_set_format,
                (int (*)
                 (snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_format_t)));
  AL_SYM_ALSA_ (snd_pcm_hw_params_set_periods_near,
                (int (*)
                 (snd_pcm_t *, snd_pcm_hw_params_t *, unsigned int *,
                  int *)));
  AL_SYM_ALSA_ (snd_pcm_hw_params_set_rate_near,
                (int (*)
                 (snd_pcm_t *, snd_pcm_hw_params_t *, unsigned int *,
                  int *)));
  AL_SYM_ALSA_ (snd_pcm_format_size, (ssize_t (*)(snd_pcm_format_t, size_t)));

  return AL_TRUE;
}

struct alsaData
{
  snd_pcm_t *pcmHandle;
  unsigned int frameSizeInBytes;
};

static ALboolean
ok (int error, const char *message)
{
  if (error < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "%s failed: %s", message,
                psnd_strerror (error));
      return AL_FALSE;
    }
  return AL_TRUE;
}

static void
closeALSA (struct ALC_BackendPrivateData *privateData)
{
  struct alsaData *ad = (struct alsaData *) privateData;
  ok (psnd_pcm_close (ad->pcmHandle), "close");
  free (ad);
}

static snd_pcm_format_t
convertFormatALToALSA (ALenum format)
{
  switch (format)
    {
    case AL_FORMAT_MONO8:
    case AL_FORMAT_STEREO8:
    case AL_FORMAT_QUAD8_LOKI:
      return SND_PCM_FORMAT_U8;
    case AL_FORMAT_MONO16:
    case AL_FORMAT_STEREO16:
    case AL_FORMAT_QUAD16_LOKI:
      return SND_PCM_FORMAT_S16;
    default:
      return SND_PCM_FORMAT_UNKNOWN;
    }
}

static ALboolean
setAttributesALSA (struct ALC_BackendPrivateData *privateData,
                   ALuint *bufferSizeInBytes, ALenum *format, ALuint *speed)
{
  struct alsaData *ad = (struct alsaData *) privateData;
  unsigned int periods = 4;
  snd_pcm_hw_params_t *p = NULL;
  ALboolean allOK;
  snd_pcm_format_t alsaFormat;
  unsigned int numChannels;
  snd_pcm_uframes_t bufferSizeInFrames;
  unsigned int rate;
  snd_pcm_t *h = ad->pcmHandle;;

  alsaFormat = convertFormatALToALSA (*format);
  numChannels = (unsigned int) _alGetChannelsFromFormat (*format);
  ad->frameSizeInBytes =
    (unsigned int) psnd_pcm_format_size (alsaFormat, numChannels);
  bufferSizeInFrames = *bufferSizeInBytes / ad->frameSizeInBytes;
  rate = (unsigned int) *speed;

  allOK =
    /* allocate HW parameter configuration */
    ok (psnd_pcm_hw_params_malloc (&p), "params malloc") &&
    /* start with the largest configuration space possible */
    ok (psnd_pcm_hw_params_any (h, p), "any") &&
    /* set interleaved access */
    ok (psnd_pcm_hw_params_set_access
        (h, p, SND_PCM_ACCESS_RW_INTERLEAVED), "set access") &&
    /* set format (implicitly sets sample bits) */
    ok (psnd_pcm_hw_params_set_format (h, p, alsaFormat), "set format") &&
    /* set channels (implicitly sets frame bits) */
    ok (psnd_pcm_hw_params_set_channels (h, p, numChannels),
        "set channels") &&
    /* set periods (implicitly constrains period/buffer parameters) */
    ok (psnd_pcm_hw_params_set_periods_near (h, p, &periods, NULL),
        "set periods near") &&
    /* set rate (implicitly constrains period/buffer parameters) */
    ok (psnd_pcm_hw_params_set_rate_near (h, p, &rate, NULL),
        "set rate near") &&
    /* set buffer size in frame units
       (implicitly sets period size/bytes/time and buffer time/bytes) */
    ok (psnd_pcm_hw_params_set_buffer_size_near
        (h, p, &bufferSizeInFrames), "set buffer size near") &&
    /* install and prepare hardware configuration */
    ok (psnd_pcm_hw_params (h, p), "set params");

  if (p != NULL)
    {
      psnd_pcm_hw_params_free (p);
    }

  if (!allOK)
    {
      return AL_FALSE;
    }

  *bufferSizeInBytes = bufferSizeInFrames * ad->frameSizeInBytes;
  *speed = rate;
  return AL_TRUE;
}

static void
writeALSA (struct ALC_BackendPrivateData *privateData, const void *data,
           int bytesToWrite)
{
  struct alsaData *ad = (struct alsaData *) privateData;
  const char *pdata = data;
  snd_pcm_uframes_t numFramesToWrite =
    (snd_pcm_uframes_t) bytesToWrite / ad->frameSizeInBytes;
  while (numFramesToWrite > 0)
    {
      int ret = psnd_pcm_writei (ad->pcmHandle, pdata, numFramesToWrite);
      switch (ret)
        {
        case -EAGAIN:
          continue;
        case -ESTRPIPE:
          do
            {
              ret = psnd_pcm_resume (ad->pcmHandle);
            }
          while (ret == -EAGAIN);
          break;
        case -EPIPE:
          break;
        default:
          if (ret < 0)
            {
              _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                        "could not write audio data: %s",
                        psnd_strerror (ret));
            }
          else
            {
              pdata += ret * ad->frameSizeInBytes;
              numFramesToWrite -= ret;
            }
          break;
        }
      if (ret < 0)
        {
          ret = psnd_pcm_prepare (ad->pcmHandle);
          if (ret < 0)
            {
              _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                        "PCM prepare failed: %s", psnd_strerror (ret));
              return;
            }
        }
    }
}

/* capture data from the audio device */
static ALsizei
readALSA (struct ALC_BackendPrivateData *privateData, void *data,
          int bytesToRead)
{
  struct alsaData *ad = (struct alsaData *) privateData;
  char *captureData = data;
  snd_pcm_uframes_t numFramesToRead =
    (snd_pcm_uframes_t) bytesToRead / ad->frameSizeInBytes;

  do
    {
      int ret = psnd_pcm_readi (ad->pcmHandle, captureData, numFramesToRead);
      if (ret == -EAGAIN)
        {
          return 0;
        }
      if (ret == -EPIPE)
        {
          _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                    "overrun occurred, trying to recover.");
          ret = psnd_pcm_prepare (ad->pcmHandle);
          if (ret < 0)
            {
              _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                        "readALSA: unable to recover: %s",
                        psnd_strerror (ret));
              return 0;
            }
          continue;
        }
      if (ret < 0)
        {
          _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                    "could not read audio data: %s", psnd_strerror (ret));
          return 0;
        }
      return ret * ad->frameSizeInBytes;
    }
  while (1);
}

static void
pauseALSA (UNUSED (struct ALC_BackendPrivateData *privateData))
{
}

static void
resumeALSA (UNUSED (struct ALC_BackendPrivateData *privateData))
{
}

static ALfloat
getAudioChannelALSA (UNUSED (struct ALC_BackendPrivateData *privateData),
                     UNUSED (ALuint channel))
{
  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
            "ALSA backend does not support getting the volume");
  return -1.0f;
}

static int
setAudioChannelALSA (UNUSED (struct ALC_BackendPrivateData *privateData),
                     UNUSED (ALuint channel), UNUSED (ALfloat volume))
{
  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
            "ALSA backend does not support getting the volume");
  return -1;
}

static const ALCchar *getNameALSA(struct ALC_BackendPrivateData *privateData)
{
    return "Advanced Linux Sound Architecture (ALSA)";
}

static ALC_BackendOps alsaOps = {
  closeALSA,
  pauseALSA,
  resumeALSA,
  setAttributesALSA,
  writeALSA,
  readALSA,
  getAudioChannelALSA,
  setAudioChannelALSA,
  getNameALSA
};

static void
getALSADeviceName (char *retref, size_t retsize, ALC_OpenMode mode)
{
  const char *varName =
    (mode == ALC_OPEN_INPUT_) ? "alsa-in-device" : "alsa-out-device";
  Rcvar rcv = rc_lookup (varName);
  if (rcv == NULL)
    {
      rcv = rc_lookup ("alsa-device");
    }
  if ((rcv == NULL) || (rc_type (rcv) != ALRC_STRING))
    {
      strncpy (retref, DEFAULT_DEVICE, retsize);
      retref[retsize - 1] = '\0';
      return;
    }
  rc_tostr0 (rcv, retref, retsize);
}

/*
 * Try to open the device without blocking, so we can try other backends even if
 * this would block. After we have successfully opened the device, we can put it
 * into blocking mode.
 *
 * NOTE: dmix tends to fail to open when the device is closed shortly ago, so we
 * give a 2nd chance below after a short delay.
 */
static int
pcmOpen (snd_pcm_t **pcmHandle, const char *deviceName, ALC_OpenMode mode)
{
  snd_pcm_stream_t streamType =
    (mode == ALC_OPEN_INPUT_) ?
    SND_PCM_STREAM_CAPTURE : SND_PCM_STREAM_PLAYBACK;
  int error =
    psnd_pcm_open (pcmHandle, deviceName, streamType, SND_PCM_NONBLOCK);
  if (error < 0)
    {
      usleep (200000);
      error =
        psnd_pcm_open (pcmHandle, deviceName, streamType, SND_PCM_NONBLOCK);
    }
  if ((error < 0) || (mode == ALC_OPEN_INPUT_))
    {
      return error;
    }
  return psnd_pcm_nonblock (*pcmHandle, 0);
}

void
alcBackendOpenALSA_ (ALC_OpenMode mode, ALC_BackendOps **ops,
                     struct ALC_BackendPrivateData **privateData)
{
  char deviceName[256];
  snd_pcm_t *pcmHandle;
  struct alsaData *ad;

  if (!getAPIEntriesALSA ())
    {
      *privateData = NULL;
      return;
    }

  getALSADeviceName (deviceName, sizeof (deviceName), mode);
  if (!ok (pcmOpen (&pcmHandle, deviceName, mode), "open"))
    {
      *privateData = NULL;
      return;
    }

  ad = (struct alsaData *) malloc (sizeof *ad);
  if (ad == NULL)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "failed to allocate backend data");
      psnd_pcm_close (pcmHandle);
      *privateData = NULL;
      return;
    }

  ad->pcmHandle = pcmHandle;
  ad->frameSizeInBytes = 0;

  *ops = &alsaOps;
  *privateData = (struct ALC_BackendPrivateData *) ad;
  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "using device \"%s\"",
            deviceName);
}

#endif /* USE_BACKEND_ALSA */
