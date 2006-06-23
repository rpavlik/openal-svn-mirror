/*
   ALSA backend for OpenAL

    Dirk Ehlke
    EMail: dehlke@mip.informatik.uni-kiel.de

    Multimedia Information Processing
    Christian-Albrechts-University of Kiel

    2002/06/13
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

#undef DUMP_HW_PARAMS

#ifndef USE_BACKEND_ALSA

void
alcBackendOpenALSA_ (UNUSED (ALC_OpenMode mode),
                     UNUSED (ALC_BackendOps **ops),
                     ALC_BackendPrivateData **privateData)
{
  *privateData = NULL;
}

#else

#include <AL/alext.h>
#include <stdio.h>

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

static int (*psnd_pcm_open)(snd_pcm_t**, const char*, snd_pcm_stream_t, int) = NULL;
static int (*psnd_pcm_close)(snd_pcm_t*) = NULL;
static const char *(*psnd_strerror)(int) = NULL;
static int (*psnd_pcm_nonblock)(snd_pcm_t*, int) = NULL;
static int (*psnd_pcm_prepare)(snd_pcm_t*) = NULL;
static int (*psnd_pcm_resume)(snd_pcm_t*) = NULL;
static snd_pcm_sframes_t (*psnd_pcm_readi)(snd_pcm_t*, void*, snd_pcm_uframes_t) = NULL;
static snd_pcm_sframes_t (*psnd_pcm_writei)(snd_pcm_t*, const void*, snd_pcm_uframes_t) = NULL;
static size_t (*psnd_pcm_hw_params_sizeof)(void) = NULL;
static int (*psnd_pcm_hw_params_malloc)(snd_pcm_hw_params_t**) = NULL;
static void (*psnd_pcm_hw_params_free)(snd_pcm_hw_params_t*) = NULL;
static int (*psnd_pcm_hw_params)(snd_pcm_t*, snd_pcm_hw_params_t*) = NULL;
static int (*psnd_pcm_hw_params_any)(snd_pcm_t*, snd_pcm_hw_params_t*) = NULL;
static int (*psnd_pcm_hw_params_set_access)(snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_access_t) = NULL;
static int (*psnd_pcm_hw_params_set_buffer_size_near)(snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_uframes_t*) = NULL;
static int (*psnd_pcm_hw_params_set_channels)(snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int) = NULL;
static int (*psnd_pcm_hw_params_set_format)(snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_format_t) = NULL;
static int (*psnd_pcm_hw_params_set_periods_near)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir) = NULL;
static int (*psnd_pcm_hw_params_set_rate_near)(snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*) = NULL;
static int (*psnd_pcm_hw_params_get_buffer_size)(const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val) = NULL;
static int (*psnd_pcm_hw_params_get_rate)(const snd_pcm_hw_params_t *params, unsigned int *val, int *dir) = NULL;

static int
openal_load_alsa_library (void)
{
  static AL_DLHandle alsa_lib_handle = (AL_DLHandle)0;
#ifdef OPENAL_DLOPEN_ALSA
  const char *error = NULL;
#endif

  if (alsa_lib_handle != (AL_DLHandle)0)
    return 1;                   /* already loaded. */

#ifdef OPENAL_DLOPEN_ALSA
#define OPENAL_LOAD_ALSA_SYMBOL(t, x) \
	p##x = t alDLFunSym_ (alsa_lib_handle, #x); \
	error = alDLError_ (); \
	if (error != NULL) { \
		alDLClose_ (alsa_lib_handle); \
		alsa_lib_handle = (AL_DLHandle)0; \
		return 0; \
	}

  alDLError_ ();
  alsa_lib_handle = alDLOpen_ ("libasound.so");
  error = alDLError_ ();
  if (error != NULL)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "Could not open ALSA library: %s", error);
      return 0;
    }

#else
#define OPENAL_LOAD_ALSA_SYMBOL(t, x) p##x = x;
  alsa_lib_handle = (AL_DLHandle) 0xF00DF00D;
#endif

  OPENAL_LOAD_ALSA_SYMBOL((int (*)(snd_pcm_t**, const char*, snd_pcm_stream_t, int)), snd_pcm_open);
  OPENAL_LOAD_ALSA_SYMBOL((int (*)(snd_pcm_t*)), snd_pcm_close);
  OPENAL_LOAD_ALSA_SYMBOL((const char *(*)(int)), snd_strerror);
  OPENAL_LOAD_ALSA_SYMBOL((int (*)(snd_pcm_t*, int)), snd_pcm_nonblock);
  OPENAL_LOAD_ALSA_SYMBOL((int (*)(snd_pcm_t*)), snd_pcm_prepare);
  OPENAL_LOAD_ALSA_SYMBOL((int (*)(snd_pcm_t*)), snd_pcm_resume);
  OPENAL_LOAD_ALSA_SYMBOL((snd_pcm_sframes_t (*)(snd_pcm_t*, void*, snd_pcm_uframes_t)), snd_pcm_readi);
  OPENAL_LOAD_ALSA_SYMBOL((snd_pcm_sframes_t (*)(snd_pcm_t*, const void*, snd_pcm_uframes_t)), snd_pcm_writei);
  OPENAL_LOAD_ALSA_SYMBOL((size_t (*)(void)), snd_pcm_hw_params_sizeof);
  OPENAL_LOAD_ALSA_SYMBOL((int (*)(snd_pcm_hw_params_t**)), snd_pcm_hw_params_malloc);
  OPENAL_LOAD_ALSA_SYMBOL((void (*)(snd_pcm_hw_params_t*)), snd_pcm_hw_params_free);
  OPENAL_LOAD_ALSA_SYMBOL((int (*)(snd_pcm_t*, snd_pcm_hw_params_t*)), snd_pcm_hw_params);
  OPENAL_LOAD_ALSA_SYMBOL((int (*)(snd_pcm_t*, snd_pcm_hw_params_t*)), snd_pcm_hw_params_any);
  OPENAL_LOAD_ALSA_SYMBOL((int (*)(snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_access_t)), snd_pcm_hw_params_set_access);
  OPENAL_LOAD_ALSA_SYMBOL((int (*)(snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_uframes_t*)), snd_pcm_hw_params_set_buffer_size_near);
  OPENAL_LOAD_ALSA_SYMBOL((int (*)(snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int)), snd_pcm_hw_params_set_channels);
  OPENAL_LOAD_ALSA_SYMBOL((int (*)(snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_format_t)), snd_pcm_hw_params_set_format);
  OPENAL_LOAD_ALSA_SYMBOL((int (*)(snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*)), snd_pcm_hw_params_set_periods_near);
  OPENAL_LOAD_ALSA_SYMBOL((int (*)(snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*)), snd_pcm_hw_params_set_rate_near);
  OPENAL_LOAD_ALSA_SYMBOL((int (*)(const snd_pcm_hw_params_t*, snd_pcm_uframes_t*)), snd_pcm_hw_params_get_buffer_size);
  OPENAL_LOAD_ALSA_SYMBOL((int (*)(const snd_pcm_hw_params_t*, unsigned int*, int*)), snd_pcm_hw_params_get_rate);

  return 1;
}

static snd_pcm_format_t
convertFormatALToALSA (ALenum format)
{
  switch (format)
    {
    case AL_FORMAT_STEREO8:
      return SND_PCM_FORMAT_U8;
    case AL_FORMAT_MONO8:
      return SND_PCM_FORMAT_U8;
    case AL_FORMAT_QUAD8_LOKI:
      return SND_PCM_FORMAT_U8;
    case AL_FORMAT_STEREO16:
      return SND_PCM_FORMAT_S16;
    case AL_FORMAT_MONO16:
      return SND_PCM_FORMAT_S16;
    case AL_FORMAT_QUAD16_LOKI:
      return SND_PCM_FORMAT_S16;
    default:
      return -1;
    }
}

static unsigned int
calculateFrameSizeInBytes (snd_pcm_format_t alsaFormat, unsigned int numChannels)
{
  switch (alsaFormat)
    {
    case SND_PCM_FORMAT_U8:
      return numChannels;
    case SND_PCM_FORMAT_S16:
      return 2 * numChannels;
    default:
      return -1;
    }
}

struct alsa_info {
  snd_pcm_t *pcmHandle;
  snd_pcm_format_t alsaFormat;
  unsigned int numChannels;
  unsigned int frameSizeInBytes;
  snd_pcm_uframes_t bufferSizeInFrames;
  unsigned int rate;
  ALC_OpenMode mode;
};

static void
release_alsa (void *handle)
{
  struct alsa_info *ai = handle;
  if (handle == NULL)
    {
      return;
    }
  psnd_pcm_close (ai->pcmHandle);
  free (ai);
}

static void
get_device_name (const char *varName, char *retref, size_t retsize)
{
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

static void *
grab_read_alsa (void)
{
  struct alsa_info *retval;
  snd_pcm_t *pcmHandle;
  char card_name[256];
  int err;

  if (!openal_load_alsa_library ())
    {
      return NULL;
    }

  get_device_name ("alsa-out-device", card_name, sizeof (card_name));
  err =
    psnd_pcm_open (&pcmHandle, card_name, SND_PCM_STREAM_CAPTURE,
                   SND_PCM_NONBLOCK);
  if (err < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "grab_read_alsa: init failed: %s", psnd_strerror (err));
      return NULL;
    }

  retval = malloc (sizeof *retval);
  retval->pcmHandle = pcmHandle;
  retval->alsaFormat = 0;
  retval->numChannels = 0;
  retval->frameSizeInBytes = 0;
  retval->bufferSizeInFrames = 0;
  retval->rate = 0;
  retval->mode = ALC_OPEN_INPUT_;

  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
            "grab_read_alsa: init ok, using %s", card_name);

  return retval;
}

static void *
grab_write_alsa (void)
{
  struct alsa_info *retval;
  snd_pcm_t *pcmHandle;
  char card_name[256];
  int err;

  if (!openal_load_alsa_library ())
    return NULL;

  get_device_name ("alsa-in-device", card_name, sizeof (card_name));

  /* Try to open the device without blocking, so we can 
   * try other backends even if this would block.
   */
  err =
    psnd_pcm_open (&pcmHandle, card_name, SND_PCM_STREAM_PLAYBACK,
                   SND_PCM_NONBLOCK);
  if (err < 0)
    {
      /* We give a chance again after a short delay since dmix tends
       * to fail to open when the device is closed shortly ago.
       */
      usleep (200000);
      err = psnd_pcm_open (&pcmHandle, card_name,
                           SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
    }

  if (err < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "grab_write_alsa: init failed: %s", psnd_strerror (err));
      return NULL;
    }

  /* Now that we have successfully opened the device, 
   * we can put it into blocking mode.
   */
  err = psnd_pcm_nonblock (pcmHandle, 0);
  if (err < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "grab_write_alsa: could not put device into blocking mode: %s",
                psnd_strerror (err));
    }

  retval = malloc (sizeof *retval);
  retval->pcmHandle = pcmHandle;
  retval->alsaFormat = 0;
  retval->numChannels = 0;
  retval->frameSizeInBytes = 0;
  retval->bufferSizeInFrames = 0;
  retval->rate = 0;
  retval->mode = ALC_OPEN_OUTPUT_;

  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
            "grab_write_alsa: init ok, using %s", card_name);

  return retval;
}

static void
dumpHWParams(const char *message, snd_pcm_hw_params_t *setup)
{
#ifdef DUMP_HW_PARAMS
  AL_DLHandle hndl = alDLOpen_ ("libasound.so.2");
  int (*psnd_output_stdio_attach)(snd_output_t **, FILE *, int) =
    (int (*)(snd_output_t **, FILE *, int)) alDLFunSym_(hndl, "snd_output_stdio_attach");
  int (*psnd_pcm_hw_params_dump)(snd_pcm_hw_params_t *, snd_output_t *) =
    (int (*)(snd_pcm_hw_params_t *, snd_output_t *)) alDLFunSym_(hndl, "snd_pcm_hw_params_dump");
  int (*psnd_output_close)(snd_output_t *) =
    (int (*)(snd_output_t *))alDLFunSym_(hndl, "snd_output_close");
  snd_output_t *out;
  printf("------------------------------------------------------------\n");
  printf("-- %s\n", message);
  psnd_output_stdio_attach (&out, stdout, 0);
  psnd_pcm_hw_params_dump (setup, out);
  psnd_output_close (out);
#else
  (void)message;
  (void)setup;
#endif
}

static ALboolean
set_read_alsa (void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed)
{
#if 0
  struct alsa_info *ai = handle;
  snd_pcm_hw_params_t *setup;
  snd_pcm_t *phandle;
  int err;

  if ((ai == NULL) || (ai->handle == NULL))
    return AL_FALSE;

  if ((*fmt == AL_FORMAT_QUAD8_LOKI) || (*fmt == AL_FORMAT_STEREO8))
    *fmt = AL_FORMAT_MONO8;
  if ((*fmt == AL_FORMAT_QUAD16_LOKI) || (*fmt == AL_FORMAT_STEREO16))
    *fmt = AL_FORMAT_MONO16;

  ai->channels = 1;
  ai->format = convertFormatALToALSA (*fmt);
  ai->speed = (unsigned int) *speed;
  ai->frameSizeInBytes = calculateFrameSizeInBytes (ai->format, ai->channels);
  ai->bufferSize = (snd_pcm_uframes_t) 8192;
  ai->periodSize = 2;

  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
            "alsa info (read): channels: %u, format: %u, speed: %u, frameSize: %u, bufferSize: %lu,periodSize: %lu",
            ai->channels, ai->format, ai->speed, ai->frameSize,
            ai->bufferSize, ai->periodSize);

  phandle = ai->handle;

  psnd_pcm_hw_params_malloc (&setup);
  err = psnd_pcm_hw_params_any (phandle, setup);
  if (err < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "set_read_alsa: Could not query parameters: %s",
                psnd_strerror (err));

      psnd_pcm_hw_params_free (setup);
      return AL_FALSE;
    }

  /* set the interleaved read format */
  err =
    psnd_pcm_hw_params_set_access (phandle, setup,
                                   SND_PCM_ACCESS_RW_INTERLEAVED);
  if (err < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "set_read_alsa: Could not set access type: %s",
                psnd_strerror (err));
      psnd_pcm_hw_params_free (setup);
      return AL_FALSE;
    }

  /* set format (implicitly sets sample bits) */
  err = psnd_pcm_hw_params_set_format (phandle, setup, ai->format);
  if (err < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "set_read_alsa: could not set format: %s",
                psnd_strerror (err));

      psnd_pcm_hw_params_free (setup);
      return AL_FALSE;
    }

  /* channels (implicitly sets frame bits) */
  err = psnd_pcm_hw_params_set_channels (phandle, setup, ai->channels);
  if (err < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
		"set_read_alsa: could not set channels: %s",
		psnd_strerror (err));
      psnd_pcm_hw_params_free (setup);
      return AL_FALSE;
    }

  /* sampling rate */
  err = psnd_pcm_hw_params_set_rate_near (phandle, setup, &ai->speed, NULL);
  if (err < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "set_read_alsa: could not set speed: %s",
                psnd_strerror (err));

      psnd_pcm_hw_params_free (setup);
      return AL_FALSE;
    }

  /* set period size */
  err =
    psnd_pcm_hw_params_set_period_size_near (phandle, setup, &ai->periodSize, NULL);
  if (err < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "set_read_alsa: %s", psnd_strerror (err));
      psnd_pcm_hw_params_free (setup);
      return AL_FALSE;
    }

  err =
    psnd_pcm_hw_params_set_buffer_size_near (phandle, setup,
                                             &ai->bufferSize);
  if (err < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "set_read_alsa: %s, size: %lu, speed: %d",
                psnd_strerror (err), ai->bufferSize, ai->speed);
      psnd_pcm_hw_params_free (setup);
      return AL_FALSE;
    }

  *bufsiz = ai->bufferSize * ai->frameSize;

  err = psnd_pcm_hw_params (phandle, setup);
  if (err < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "set_read_alsa: %s", psnd_strerror (err));
      psnd_pcm_hw_params_free (setup);
      return AL_FALSE;
    }

  err = psnd_pcm_prepare (phandle);
  if (err < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "set_read_alsa %s", psnd_strerror (err));
      psnd_pcm_hw_params_free (setup);
      return AL_FALSE;
    }

  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
            "set_read_alsa: handle: %p, phandle: %p", handle,
            (void *) phandle);

  psnd_pcm_hw_params_free (setup);
#endif
  return AL_TRUE;
}

static ALboolean
set_write_alsa (void *handle, ALuint *bufferSizeInBytes, ALenum *fmt, ALuint *speed)
{
  struct alsa_info *ai = handle;
  snd_pcm_hw_params_t *setup;
  int err;
  unsigned int periods;

  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
            "set_write_alsa: requesting buffer size in bytes %u, speed: %u",
            *bufferSizeInBytes, *speed);

  if ((ai == NULL) || (ai->pcmHandle == NULL))
    {
      return AL_FALSE;
    }

  ai->alsaFormat = convertFormatALToALSA (*fmt);
  ai->numChannels = (unsigned int) _alGetChannelsFromFormat (*fmt);
  ai->frameSizeInBytes = calculateFrameSizeInBytes (ai->alsaFormat, ai->numChannels);
  ai->bufferSizeInFrames = *bufferSizeInBytes / ai->frameSizeInBytes;
  ai->rate = (unsigned int) *speed;
  periods = 2;

  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
            "set_write_alsa: requesting ALSA format %u, number of channels: %u, frame size in bytes: %u, buffer size in frames: %lu, rate: %u",
            ai->alsaFormat, ai->numChannels, ai->frameSizeInBytes, ai->bufferSizeInFrames, ai->rate);

  psnd_pcm_hw_params_malloc (&setup);
  err = psnd_pcm_hw_params_any (ai->pcmHandle, setup);
  if (err < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "set_write_alsa: could not initialize parameters: %s",
                psnd_strerror (err));
      psnd_pcm_hw_params_free (setup);
      return AL_FALSE;
    }
  dumpHWParams("initial values", setup);

  /* set interleaved access */
  err =
    psnd_pcm_hw_params_set_access (ai->pcmHandle, setup,
                                   SND_PCM_ACCESS_RW_INTERLEAVED);
  if (err < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "set_write_alsa: could not set access: %s",
                psnd_strerror (err));
      psnd_pcm_hw_params_free (setup);
      return AL_FALSE;
    }
  dumpHWParams("access", setup);

  /* set format (implicitly sets sample bits) */
  err = psnd_pcm_hw_params_set_format (ai->pcmHandle, setup, ai->alsaFormat);
  if (err < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "set_write_alsa: could not set format: %s",
                psnd_strerror (err));
      psnd_pcm_hw_params_free (setup);
      return AL_FALSE;
    }
  dumpHWParams("format", setup);

  /* set channels (implicitly sets frame bits) */
  err = psnd_pcm_hw_params_set_channels (ai->pcmHandle, setup, ai->numChannels);
  if (err < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
		"set_write_alsa: could not set channels: %s",
		psnd_strerror (err));
      psnd_pcm_hw_params_free (setup);
      return AL_FALSE;
    }
  dumpHWParams("channels", setup);

  /* set rate (implictly constrains period size and buffer size) */
  err = psnd_pcm_hw_params_set_rate_near (ai->pcmHandle, setup, &ai->rate, NULL);
  if (err < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "set_write_alsa: could not set rate: %s",
                psnd_strerror (err));
      psnd_pcm_hw_params_free (setup);
      return AL_FALSE;
    }
  dumpHWParams("rate", setup);

  /* set buffer size in frame units (implicitly sets buffer size in bytes and buffer time) */
  err =
    psnd_pcm_hw_params_set_buffer_size_near (ai->pcmHandle, setup,
                                             &ai->bufferSizeInFrames);
  if (err < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "set_write_alsa: could not set buffer size: %s",
                psnd_strerror (err));
      psnd_pcm_hw_params_free (setup);
      return AL_FALSE;
    }
  dumpHWParams("buffer size", setup);

  /* set periods (implicitly sets period size in frames, persiod size in bytes, and period time) */
  err =
    psnd_pcm_hw_params_set_periods_near (ai->pcmHandle, setup, &periods, NULL);
  if (err < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "set_write_alsa: could not set period size: %s",
		psnd_strerror (err));
      psnd_pcm_hw_params_free (setup);
      return AL_FALSE;
    }
  dumpHWParams("periods", setup);

  /* install and prepare hardware configuration */
  err = psnd_pcm_hw_params (ai->pcmHandle, setup);
  if (err < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "set_alsa: could not set parameters: %s",
		psnd_strerror (err));
      psnd_pcm_hw_params_free (setup);
      return AL_FALSE;
    }
  dumpHWParams("params", setup);

  /* perhaps the rate has changed a bit */
  err = psnd_pcm_hw_params_get_rate(setup, &ai->rate, NULL);  
  if (err < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "set_write_alsa: could not get rate: %s",
		psnd_strerror (err));
      psnd_pcm_hw_params_free (setup);
      return AL_FALSE;
    }
  *bufferSizeInBytes = ai->bufferSizeInFrames * ai->frameSizeInBytes;

  /* perhaps the buffer size has changed a bit */
  err = psnd_pcm_hw_params_get_buffer_size(setup, &ai->bufferSizeInFrames);  
  if (err < 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "set_write_alsa: could not get buffer size: %s",
		psnd_strerror (err));
      psnd_pcm_hw_params_free (setup);
      return AL_FALSE;
    }
  *speed = ai->rate;

  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
            "set_write_alsa: using buffer size in bytes %u, speed: %u",
            *bufferSizeInBytes, *speed);

  psnd_pcm_hw_params_free (setup);
  return AL_TRUE;
}

static ALboolean
alcBackendSetAttributesALSA_ (void *handle, ALuint *bufsiz, ALenum *fmt,
                              ALuint *speed)
{
  return ((struct alsa_info *) handle)->mode == ALC_OPEN_INPUT_ ?
    set_read_alsa (handle, bufsiz, fmt, speed) :
    set_write_alsa (handle, bufsiz, fmt, speed);
}

static void
alsa_blitbuffer (void *handle, const void *data, int bytesToWrite)
{
  struct alsa_info *ai = handle;
  const char *pdata = data;
  snd_pcm_uframes_t numFramesToWrite;

  if ((ai == NULL) || (ai->pcmHandle == NULL))
    {
      return;
    }

  numFramesToWrite = (snd_pcm_uframes_t) bytesToWrite / ai->frameSizeInBytes;
  while (numFramesToWrite > 0)
    {
      int ret = psnd_pcm_writei (ai->pcmHandle, pdata, numFramesToWrite);
      switch (ret)
        {
        case -EAGAIN:
          continue;
        case -ESTRPIPE:
          do
            {
              ret = psnd_pcm_resume (ai->pcmHandle);
            }
          while (ret == -EAGAIN);
          break;
        case -EPIPE:
          break;
        default:
          if (ret < 0)
            {
	      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                       "alsa_blitbuffer: Could not write audio data to sound device: %s",
                       psnd_strerror (ret));
            }
	  else
	    {
	      pdata += ret * ai->frameSizeInBytes;
	      numFramesToWrite -= ret;
	    }
          break;
        }
      if (ret < 0)
        {
          ret = psnd_pcm_prepare (ai->pcmHandle);
          if (ret < 0)
            {
              _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                        "alsa_blitbuffer: %s",
			psnd_strerror (ret));
              return;
            }
        }
    }
}

/* capture data from the audio device */
static ALsizei
capture_alsa (void *handle, void *capture_buffer, int bytesToRead)
{
  struct alsa_info *ai = handle;
  char *pdata = capture_buffer;
  snd_pcm_uframes_t numFramesToRead;

  if ((ai == NULL) || (ai->pcmHandle == NULL)) {
    return 0;
  }

  numFramesToRead = (snd_pcm_uframes_t) bytesToRead / ai->frameSizeInBytes;

  do
    {
      int ret = psnd_pcm_readi (ai->pcmHandle, pdata, numFramesToRead);
      if (ret == -EAGAIN)
	{
	 return 0;
	}
      if (ret == -EPIPE)
	{
	  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
		    "capture_alsa: overrun occurred, trying to recover.");
	  ret = psnd_pcm_prepare (ai->pcmHandle);
	  if (ret < 0)
	    {
	      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
			"capture_alsa: unable to recover: %s", psnd_strerror(ret));
	      return 0;
	    }
	  continue;
	}
      if (ret < 0)
	{
	  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
		    "capture_alsa: error occurred: %s", psnd_strerror(ret));
	 return 0;
	}
      return ret * ai->frameSizeInBytes;
    }
  while (1);
}

static void
pause_alsa (UNUSED (void *handle))
{
}

static void
resume_alsa (UNUSED (void *handle))
{
}

static ALfloat
get_alsachannel (UNUSED (void *handle), UNUSED (ALuint channel))
{
  return 0.0;
}

static int
set_alsachannel (UNUSED (void *handle), UNUSED (ALuint channel),
                 UNUSED (ALfloat volume))
{
  return 0;
}

static ALC_BackendOps alsaOps = {
  release_alsa,
  pause_alsa,
  resume_alsa,
  alcBackendSetAttributesALSA_,
  alsa_blitbuffer,
  capture_alsa,
  get_alsachannel,
  set_alsachannel
};

void
alcBackendOpenALSA_ (ALC_OpenMode mode, ALC_BackendOps **ops,
                     ALC_BackendPrivateData **privateData)
{
  *privateData =
    (mode == ALC_OPEN_INPUT_) ? grab_read_alsa () : grab_write_alsa ();
  if (*privateData != NULL)
    {
      *ops = &alsaOps;
    }
}

#endif /* USE_BACKEND_ALSA */
