/*
 * WAVE file output. 
 */
#include "al_siteconfig.h"
#include "backends/alc_backend.h"
#include <stdlib.h>

#ifndef USE_BACKEND_WAVEOUT

void
alcBackendOpenWAVE_ (UNUSED (ALC_OpenMode mode),
                     UNUSED (ALC_BackendOps **ops),
                     ALC_BackendPrivateData **privateData)
{
  *privateData = NULL;
}

#else

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <AL/al.h>

#include "al_main.h"
#include "al_debug.h"
#include "audioconvert/ac_endian.h"

#define WAVEOUT_NAMELEN 16
#define HEADERSIZE      28
#define DATAADJUSTMENT  44

#ifdef WORDS_BIGENDIAN
#define RIFFMAGIC       0x52494646
#define WAVEMAGIC       0x57415645
#define FMTMAGIC        0x666D7420
#define DATAMAGIC	0x64617461
#else
#define RIFFMAGIC       0x46464952
#define WAVEMAGIC       0x45564157
#define FMTMAGIC        0x20746D66
#define DATAMAGIC	0x61746164
#endif /* WORDS_BIGENDIAN */

/* better wart needed? JIV FIXME */
#ifdef MAXNAMELEN
#undef MAXNAMELEN
#endif /* MAXNAMELEN */

#define MAXNAMELEN      1024

struct waveData
{
  FILE *fh;
  ALuint length;
  ALuint speed;
  ALuint channels;
  ALushort bitspersample;
};

static void
apply_header (struct waveData *wd)
{
  ALushort writer16;
  ALuint writer32;

  /* go to beginning */
  if (fseek (wd->fh, SEEK_SET, 0) != 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "failed to rewind");
      return;
    }

  /* 'RIFF' */
  writer32 = RIFFMAGIC;
  fwrite (&writer32, 1, sizeof writer32, wd->fh);

  /* total length */
  writer32 = swap32le (wd->length);
  fwrite (&writer32, 1, sizeof writer32, wd->fh);

  /* 'WAVE' */
  writer32 = WAVEMAGIC;
  fwrite (&writer32, 1, sizeof writer32, wd->fh);

  /* 'fmt ' */
  writer32 = FMTMAGIC;
  fwrite (&writer32, 1, sizeof writer32, wd->fh);

  /* fmt chunk length */
  writer32 = swap32le (16);
  fwrite (&writer32, 1, sizeof writer32, wd->fh);

  /* ALushort encoding */
  writer16 = swap16le (1);
  fwrite (&writer16, 1, sizeof writer16, wd->fh);

  /* Alushort channels */
  writer16 = swap16le (wd->channels);
  fwrite (&writer16, 1, sizeof writer16, wd->fh);

  /* ALuint frequency */
  writer32 = swap32le (wd->speed);
  fwrite (&writer32, 1, sizeof writer32, wd->fh);

  /* ALuint byterate  */
  writer32 = swap32le (wd->speed / sizeof (ALshort));   /* FIXME */
  fwrite (&writer32, 1, sizeof writer32, wd->fh);

  /* ALushort blockalign */
  writer16 = 0;
  fwrite (&writer16, 1, sizeof writer16, wd->fh);

  /* ALushort bitspersample */
  writer16 = swap16le (wd->bitspersample);
  fwrite (&writer16, 1, sizeof writer16, wd->fh);

  /* 'data' */
  writer32 = DATAMAGIC;
  fwrite (&writer32, 1, sizeof writer32, wd->fh);

  /* data length */
  writer32 = swap32le (wd->length - DATAADJUSTMENT);    /* samples */
  fwrite (&writer32, 1, sizeof writer32, wd->fh);
}

static void
closeWave (ALC_BackendPrivateData *privateData)
{
  struct waveData *wd = (struct waveData *) privateData;
  fflush (wd->fh);
  apply_header (wd);
  fclose (wd->fh);
  free (wd);
}

static void
pauseWave (UNUSED (ALC_BackendPrivateData *privateData))
{
}

static void
resumeWave (UNUSED (ALC_BackendPrivateData *privateData))
{
}

/*
 * convert_to_little_endian( ALuint bps, void *data, int nbytes )
 *
 * Convert data in place to little endian format.  bps is the bits per
 * sample in data, nbytes is the length of data in bytes.  If bps is 8,
 * or the machine is little endian, this is a nop.
 *
 * FIXME:
 *	We only handle 16-bit signed formats for now.  Should fix this.
 */
static void
convert_to_little_endian (ALuint bps, void *data, int nbytes)
{
  if (bps == 8)
    {
      /* 8-bit samples don't need to be converted */
      return;
    }

#ifdef WORDS_BIGENDIAN
  /* do the conversion */
  {
    ALshort *outp = data;
    ALuint i;
    for (i = 0; i < nbytes / sizeof (ALshort); i++)
      {
        outp[i] = swap16le (outp[i]);
      }
  }
#else
  (void) data;
  (void) nbytes;
#endif /* WORDS_BIG_ENDIAN */
}

static void
writeWave (ALC_BackendPrivateData *privateData, const void *data,
           int bytesToWrite)
{
  struct waveData *wd = (struct waveData *) privateData;
  wd->length += bytesToWrite;

  /*
   * WAV files expect their PCM data in LE format.  If we are on
   * big endian host, we need to convert the data in place.
   * TODO: THIS BREAKS THE CONST CORRECTNESS!!!!!!!!!!!!!!!
   *
   */
  convert_to_little_endian (wd->bitspersample, (void *) data, bytesToWrite);

  fwrite (data, 1, bytesToWrite, wd->fh);
}

static const char *
waveout_unique_name (char *template)
{
  static char retval[MAXNAMELEN];
  int template_offset;
  static int sequence = 0;
  struct stat buf;

  strncpy (retval, template, MAXNAMELEN - 2);
  retval[MAXNAMELEN - 1] = '\0';

  template_offset = strlen (retval);

  if (template_offset >= MAXNAMELEN - 28)
    {                           /* kludgey */
      /* template too big */
      return NULL;
    }

  do
    {
      /* repeat until we have a unique name */
      snprintf (&retval[template_offset], sizeof (retval) - template_offset,
                "%d.wav", sequence++);
      strncpy (template, retval, MAXNAMELEN);
    }
  while (stat (retval, &buf) == 0);

  return retval;
}

static ALboolean
setAttributesWave (ALC_BackendPrivateData *privateData,
                   UNUSED (ALuint *deviceBufferSizeInBytes), ALenum *format,
                   ALuint *speed)
{
  struct waveData *wd = (struct waveData *) privateData;
  wd->speed = *speed;
  wd->channels = _alGetChannelsFromFormat (*format);
  wd->bitspersample = _alGetBitsFromFormat (*format);
  return AL_TRUE;
}

static ALsizei
readWave (UNUSED (ALC_BackendPrivateData *privateData), UNUSED (void *data),
          UNUSED (int bytesToRead))
{
  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "should never happen");
  return 0;
}

static ALfloat
getAudioChannelWave (UNUSED (ALC_BackendPrivateData *privateData),
                     UNUSED (ALuint channel))
{
  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
            "wave backend does not support getting the volume");
  return -1.0f;
}

static int
setAudioChannelWave (UNUSED (ALC_BackendPrivateData *privateData),
                     UNUSED (ALuint channel), UNUSED (ALfloat volume))
{
  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
            "wave backend does not support setting the volume");
  return -1;
}

static ALC_BackendOps waveOps = {
  closeWave,
  pauseWave,
  resumeWave,
  setAttributesWave,
  writeWave,
  readWave,
  getAudioChannelWave,
  setAudioChannelWave
};

void
alcBackendOpenWAVE_ (ALC_OpenMode mode, ALC_BackendOps **ops,
                     ALC_BackendPrivateData **privateData)
{
  FILE *fh;
  struct waveData *wd;
  char fileName[MAXNAMELEN] = "openal-";

  if (mode == ALC_OPEN_INPUT_)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "wave backend does not support input");
      *privateData = NULL;
      return;
    }

  if (waveout_unique_name (fileName) == NULL)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "failed to generate fresh filename");
      *privateData = NULL;
      return;
    }

  fh = fopen (fileName, "w+b");
  if (fh == NULL)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "failed to open \"%s\"", fileName);
      *privateData = NULL;
      return;
    }

  wd = (struct waveData *) malloc (sizeof *wd);
  if (wd == NULL)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "failed to allocate backend data");
      fclose (fh);
      *privateData = NULL;
      return;
    }

  wd->fh = fh;
  wd->length = 0;

  /* leave room for header */
  fseek (wd->fh, SEEK_SET, HEADERSIZE);

  *ops = &waveOps;
  *privateData = (ALC_BackendPrivateData *) wd;
  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "using file \"%s\"", fileName);
}

#endif /* USE_BACKEND_WAVEOUT */
