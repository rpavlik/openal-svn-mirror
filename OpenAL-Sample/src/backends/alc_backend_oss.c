/*
 * Open Sound System (OSS) backend for OpenAL
 */

#include "al_siteconfig.h"
#include "backends/alc_backend.h"
#include <stdlib.h>

#ifndef USE_BACKEND_OSS

void
alcBackendOpenOSS_ (UNUSED (ALC_OpenMode mode),
                    UNUSED (ALC_BackendOps **ops),
                    ALC_BackendPrivateData **privateData)
{
  *privateData = NULL;
}

#else

#define DEFAULT_DEVICE "/dev/dsp"

#include <string.h>
#include <fcntl.h>
#include <stropts.h>
#include <unistd.h>
#include <sys/soundcard.h>
#include <AL/alext.h>

#include "config/al_config.h"
#include "al_debug.h"

/*
 * The OSS documentation talks about SOUND_MIXER_READ, but the header
 * only contains MIXER_READ. Play safe. Same for WRITE.
 */
#ifndef SOUND_MIXER_READ
#define SOUND_MIXER_READ MIXER_READ
#endif
#ifndef SOUND_MIXER_WRITE
#define SOUND_MIXER_WRITE MIXER_WRITE
#endif

/* private data for this backend */
struct ossData
{
  int fd;                       /* file descriptor of the OSS device */
};

static ALboolean
convertFormatFromOSS (ALenum *alFormat, int ossFormat, int numChannels)
{
  switch (ossFormat)
    {
    case AFMT_U8:
      switch (numChannels)
        {
        case 1:
          *alFormat = AL_FORMAT_MONO8;
          return AL_TRUE;
        case 2:
          *alFormat = AL_FORMAT_STEREO8;
          return AL_TRUE;
        case 4:
          *alFormat = AL_FORMAT_QUAD8_LOKI;
          return AL_TRUE;
        }
      break;
    case AFMT_S16_NE:
      switch (numChannels)
        {
        case 1:
          *alFormat = AL_FORMAT_MONO16;
          return AL_TRUE;
        case 2:
          *alFormat = AL_FORMAT_STEREO16;
          return AL_TRUE;
        case 4:
          *alFormat = AL_FORMAT_QUAD16_LOKI;
          return AL_TRUE;
        }
      break;
    case AFMT_IMA_ADPCM:
      switch (numChannels)
        {
        case 1:
          *alFormat = AL_FORMAT_IMA_ADPCM_MONO16_EXT;
          return AL_TRUE;
        case 2:
          *alFormat = AL_FORMAT_IMA_ADPCM_STEREO16_EXT;
          return AL_TRUE;
        }
      break;
    }
  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
            "uknown OSS format 0x%x, channels %d", ossFormat, numChannels);
  return AL_FALSE;
}

static ALboolean
convertFormatFromAL (int *ossFormat, int *numChannels, ALenum alFormat)
{
  switch (alFormat)
    {
    case AL_FORMAT_MONO8:
      *ossFormat = AFMT_U8;
      *numChannels = 1;
      return AL_TRUE;
    case AL_FORMAT_STEREO8:
      *ossFormat = AFMT_U8;
      *numChannels = 2;
      return AL_TRUE;
    case AL_FORMAT_QUAD8_LOKI:
      *ossFormat = AFMT_U8;
      *numChannels = 4;
      return AL_TRUE;
    case AL_FORMAT_MONO16:
      *ossFormat = AFMT_S16_NE;
      *numChannels = 1;
      return AL_TRUE;
    case AL_FORMAT_STEREO16:
      *ossFormat = AFMT_S16_NE;
      *numChannels = 2;
      return AL_TRUE;
    case AL_FORMAT_QUAD16_LOKI:
      *ossFormat = AFMT_S16_NE;
      *numChannels = 4;
      return AL_TRUE;
    case AL_FORMAT_IMA_ADPCM_MONO16_EXT:
      *ossFormat = AFMT_IMA_ADPCM;
      *numChannels = 1;
      return AL_TRUE;
    case AL_FORMAT_IMA_ADPCM_STEREO16_EXT:
      *ossFormat = AFMT_IMA_ADPCM;
      *numChannels = 2;
      return AL_TRUE;
    default:
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "uknown OpenAL format 0x%x", alFormat);
      return AL_FALSE;
    }
}

static ALboolean
convertVolumeFromOSS (ALfloat *alVolume, int ossVolume)
{
  if (!((0 <= ossVolume) && (ossVolume <= 100)))
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "OSS volume out of range %d", ossVolume);
      return AL_FALSE;
    }

  /* Use volume of left channel only, which is correct for mono sources. */
  *alVolume = (ossVolume & 0xFF) / 100.0f;
  return AL_TRUE;
}

static ALboolean
convertVolumeFromAL (int *ossVolume, ALfloat alVolume)
{
  if (!((0.0f <= alVolume) && (alVolume <= 100.0f)))
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "OpenAL volume out of range: %f", alVolume);
      return AL_FALSE;
    }
  {
    int v = (int) (alVolume * 100.0f);
    *ossVolume = (v << 8) + v;
    return AL_TRUE;
  }
}

static ALboolean
convertChannelFromAL (int *ossChannel, ALuint alChannel)
{
  switch (alChannel)
    {
    case ALC_CHAN_MAIN_LOKI:
      *ossChannel = SOUND_MIXER_VOLUME;
      return AL_TRUE;
    case ALC_CHAN_PCM_LOKI:
      *ossChannel = SOUND_MIXER_PCM;
      return AL_TRUE;
    case ALC_CHAN_CD_LOKI:
      *ossChannel = SOUND_MIXER_CD;
      return AL_TRUE;
    default:
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "unknown OpenAL channel 0x%x", alChannel);
      return AL_FALSE;
    }
}

static ALboolean
ok (int error, const char *message)
{
  if (error == -1)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "%s failed: %s",
                message, strerror (error));
      return AL_FALSE;
    }
  return AL_TRUE;
}

static void
closeOSS (ALC_BackendPrivateData *privateData)
{
  struct ossData *od = privateData;
  ok (close (od->fd), "close");
  free (od);
}

static ALboolean
setAttributesOSS (ALC_BackendPrivateData *privateData,
                  ALuint *bufferSizeInBytes, ALenum *format, ALuint *speed)
{
  struct ossData *od = privateData;
  int ossFormat;
  int numChannels;
  int ossSpeed = (int) *speed;

  if (!(convertFormatFromAL (&ossFormat, &numChannels, *format) &&
        ok (ioctl (od->fd, SNDCTL_DSP_SETFMT, &ossFormat),
            "set format") &&
        ok (ioctl (od->fd, SNDCTL_DSP_CHANNELS, &numChannels),
            "set channels") &&
        ok (ioctl (od->fd, SNDCTL_DSP_SPEED, &ossSpeed),
            "set speed") &&
        convertFormatFromOSS (format, ossFormat, numChannels)))
    {
      return AL_FALSE;
    }

  *speed = (ALuint) ossSpeed;
  return AL_TRUE;
}

static void
writeOSS (ALC_BackendPrivateData *privateData, const void *data,
          int bytesToWrite)
{
  struct ossData *od = privateData;
  write (od->fd, data, bytesToWrite);
}

static ALsizei
readOSS (ALC_BackendPrivateData *privateData, void *data, int bytesToRead)
{
  struct ossData *od = privateData;
  return read (od->fd, data, bytesToRead);
}

static void
pauseOSS (UNUSED (ALC_BackendPrivateData *privateData))
{
}

static void
resumeOSS (UNUSED (ALC_BackendPrivateData *privateData))
{
}

static ALfloat
getAudioChannelOSS (ALC_BackendPrivateData *privateData, ALuint channel)
{
  int ossChannel;
  int ossVolume;
  ALfloat alVolume;
  struct ossData *od = privateData;

  /*
   * Note that we use the file descriptor of our OSS device, which is
   * guaranteed to use correct mixer if there is one for that device.
   * Using /dev/mixer would be a lottery...
   */
  if (!(convertChannelFromAL (&ossChannel, channel) &&
        ok (ioctl (od->fd, SOUND_MIXER_READ (ossChannel), &ossVolume),
            "get volume") && convertVolumeFromOSS (&alVolume, ossVolume)))
    {
      return -1.0f;
    }
  return alVolume;
}

static int
setAudioChannelOSS (ALC_BackendPrivateData *privateData, ALuint channel,
                    ALfloat volume)
{
  int ossChannel;
  int ossVolume;
  struct ossData *od = privateData;

  /*
   * Note that we use the file descriptor of our OSS device, which is
   * guaranteed to use correct mixer if there is one for that device.
   * Using /dev/mixer would be a lottery...
   */
  if (!(convertChannelFromAL (&ossChannel, channel) &&
        convertVolumeFromAL (&ossVolume, volume) &&
        ok (ioctl (od->fd, SOUND_MIXER_WRITE (ossChannel), &ossVolume),
            "set volume")))
    {
      return -1;
    }

  return 0;
}

static ALC_BackendOps ossOps = {
  closeOSS,
  pauseOSS,
  resumeOSS,
  setAttributesOSS,
  writeOSS,
  readOSS,
  getAudioChannelOSS,
  setAudioChannelOSS
};

static void
getOSSDeviceName (char *retref, size_t retsize, ALC_OpenMode mode)
{
  const char *varName =
    (mode == ALC_OPEN_INPUT_) ? "oss-in-device" : "oss-out-device";
  Rcvar rcv = rc_lookup (varName);
  if (rcv == NULL)
    {
      rcv = rc_lookup ("oss-device");
    }
  if ((rcv == NULL) || (rc_type (rcv) != ALRC_STRING))
    {
      strncpy (retref, DEFAULT_DEVICE, retsize);
      retref[retsize - 1] = '\0';
      return;
    }
  rc_tostr0 (rcv, retref, retsize);
}

void
alcBackendOpenOSS_ (ALC_OpenMode mode, ALC_BackendOps **ops,
                    ALC_BackendPrivateData **privateData)
{
  char deviceName[256];
  int fd;
  struct ossData *od = (struct ossData *) malloc (sizeof *od);

  getOSSDeviceName (deviceName, sizeof (deviceName), mode);
  fd = open (deviceName, mode);
  if (!ok (fd, "open"))
    {
      *privateData = NULL;
      return;
    }

  od = (struct ossData *) malloc (sizeof *od);
  if (od == NULL)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "failed to allocate backend data");
      close (fd);
      *privateData = NULL;
      return;
    }

  od->fd = fd;

  *ops = &ossOps;
  *privateData = (ALC_BackendPrivateData *) od;
  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "using device \"%s\"",
            deviceName);
}

#endif /* USE_BACKEND_OSS */
