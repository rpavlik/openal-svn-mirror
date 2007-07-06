/*
 * WAVE file output. Not really a high-performance backend...
 */

#include "al_siteconfig.h"
#include "backends/alc_backend.h"
#include <stdlib.h>

#ifndef USE_BACKEND_WAVEOUT

void
alcBackendOpenWAVE_ (UNUSED (ALC_OpenMode mode),
                     UNUSED (ALC_BackendOps **ops),
                     struct ALC_BackendPrivateData **privateData)
{
  *privateData = NULL;
}

#else

#include <stdio.h>
#include <string.h>

#if HAVE_STDINT_H
#include <stdint.h>
#endif /* HAVE_STDINT_H */

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */

#include <fcntl.h>
#include <errno.h>
#include <AL/al.h>
#include <AL/alext.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef _WIN32
#include <io.h>
#define open(X,Y,Z) _open(X,Y)
#endif

#include "al_debug.h"

struct waveData
{
  char fileName[32];
  FILE *fh;
  ALuint length;
  int channels;
  ALuint speed;
  int bytesPerSample;
};

static void
writeInt16LE (FILE * fh, int16_t value)
{
  unsigned char buf[2];
  buf[0] = (unsigned char) (value & 0xFF);
  buf[1] = (unsigned char) ((value >> 8) & 0xFF);
  fwrite (buf, 1, sizeof (buf), fh);
}

static void
writeInt32LE (FILE * fh, int32_t value)
{
  unsigned char buf[4];
  buf[0] = (unsigned char) (value & 0xFF);
  buf[1] = (unsigned char) ((value >> 8) & 0xFF);
  buf[2] = (unsigned char) ((value >> 16) & 0xFF);
  buf[3] = (unsigned char) ((value >> 24) & 0xFF);
  fwrite (buf, 1, sizeof (buf), fh);
}

static ALboolean
writeWAVEHeader (struct waveData *wd)
{
  /******** 8 + 4 bytes RIFF/WAVE intro ********/

  /* 'RIFF' */
  writeInt32LE (wd->fh, 0x46464952);

  /* total length */
  writeInt32LE (wd->fh, 4 + 8 + 16 + 8 + wd->length);

  /* 'WAVE' */
  writeInt32LE (wd->fh, 0x45564157);

  /******** fmt chunk (8 bytes header, 16 bytes content) ********/

  /* 'fmt ' */
  writeInt32LE (wd->fh, 0x20746D66);

  /* fmt chunk length */
  writeInt32LE (wd->fh, 16);

  /* audio format = PCM */
  writeInt16LE (wd->fh, 1);

  /* number of channels */
  writeInt16LE (wd->fh, wd->channels);

  /* sample frequency */
  writeInt32LE (wd->fh, wd->speed);

  /* byte rate */
  writeInt32LE (wd->fh, wd->speed * wd->channels * wd->bytesPerSample);

  /* block align */
  writeInt16LE (wd->fh, wd->channels * wd->bytesPerSample);

  /* bits per sample */
  writeInt16LE (wd->fh, wd->bytesPerSample * 8);

  /******** data chunk (8 bytes header, wd->length bytes content) ********/

  /* 'data' */
  writeInt32LE (wd->fh, 0x61746164);

  /* data chunk length */
  writeInt32LE (wd->fh, wd->length);

  if (ferror (wd->fh))
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "error writing WAVE header");
      return AL_FALSE;
    }
  return AL_TRUE;
}

static void
closeWave (struct ALC_BackendPrivateData *privateData)
{
  struct waveData *wd = (struct waveData *) privateData;
  fflush (wd->fh);

  /* go to beginning */
  if (fseek (wd->fh, SEEK_SET, 0) == -1)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "failed to rewind");
      fclose (wd->fh);
      free (wd);
      return;
    }

  /* ToDo: Check for errors */
  writeWAVEHeader (wd);
  fclose (wd->fh);

  if(wd->length == 0)
    {
      remove(wd->fileName);
    }

  free (wd);
}

static void
pauseWave (UNUSED (struct ALC_BackendPrivateData *privateData))
{
}

static void
resumeWave (UNUSED (struct ALC_BackendPrivateData *privateData))
{
}

static ALboolean
setAttributesWave (struct ALC_BackendPrivateData *privateData,
                   UNUSED (ALuint *deviceBufferSizeInBytes), ALenum *format,
                   ALuint *speed)
{
  struct waveData *wd = (struct waveData *) privateData;
  wd->speed = *speed;

  switch (*format)
    {
    case AL_FORMAT_MONO8:
      wd->channels = 1;
      wd->bytesPerSample = 1;
      break;
    case AL_FORMAT_STEREO8:
      wd->channels = 2;
      wd->bytesPerSample = 1;
      break;
    case AL_FORMAT_QUAD8_LOKI:
      wd->channels = 4;
      wd->bytesPerSample = 1;
      break;
    case AL_FORMAT_MONO16:
      wd->channels = 1;
      wd->bytesPerSample = 2;
      break;
    case AL_FORMAT_STEREO16:
      wd->channels = 2;
      wd->bytesPerSample = 2;
      break;
    case AL_FORMAT_QUAD16_LOKI:
      wd->channels = 4;
      wd->bytesPerSample = 2;
      break;
    default:
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "uknown OpenAL format 0x%x", *format);
      return AL_FALSE;
    }

  return AL_TRUE;
}

typedef enum
{
  LittleEndian,
  BigEndian,
  UnknwonEndian                 /* has anybody still a PDP11? :-) */
} Endianess;

/* test from Harbison & Steele, "C - A Reference Manual", section 6.1.2 */
static Endianess
endianess (void)
{
  union
  {
    long l;
    char c[sizeof (long)];
  } u;

  u.l = 1;
  return (u.c[0] == 1) ? LittleEndian :
    ((u.c[sizeof (long) - 1] == 1) ? BigEndian : UnknwonEndian);
}

static void
writeWave (struct ALC_BackendPrivateData *privateData, const void *data,
           int bytesToWrite)
{
  struct waveData *wd = (struct waveData *) privateData;
  int samplesToWrite = bytesToWrite / wd->bytesPerSample;

  if ((bytesToWrite % wd->bytesPerSample) != 0)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "%d bytes are not a multiple of %d", bytesToWrite,
                wd->bytesPerSample);
      return;
    }

  wd->length += bytesToWrite;

  if ((wd->bytesPerSample == 1) || (endianess () == LittleEndian))
    {
      fwrite (data, 1, bytesToWrite, wd->fh);
    }
  else if (wd->bytesPerSample == 2)
    {
      const int16_t *p = (const int16_t *) data;
      while (samplesToWrite > 0)
        {
          writeInt16LE (wd->fh, *p++);
          samplesToWrite--;
        }
    }
  else if (wd->bytesPerSample == 4)
    {
      const int32_t *p = (const int32_t *) data;
      while (samplesToWrite > 0)
        {
          writeInt32LE (wd->fh, *p++);
          samplesToWrite--;
        }
    }
  else
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "should never happen");
      return;
    }

  if (ferror (wd->fh))
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "error writing to WAVE file");
      return;
    }
}

static ALsizei
readWave (UNUSED (struct ALC_BackendPrivateData *privateData),
          UNUSED (void *data), UNUSED (int bytesToRead))
{
  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "should never happen");
  return 0;
}

static ALfloat
getAudioChannelWave (UNUSED (struct ALC_BackendPrivateData *privateData),
                     UNUSED (ALuint channel))
{
  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
            "wave backend does not support getting the volume");
  return -1.0f;
}

static int
setAudioChannelWave (UNUSED (struct ALC_BackendPrivateData *privateData),
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
                     struct ALC_BackendPrivateData **privateData)
{
  char fileName[32];
  int sequence, fd;
  FILE *fh;
  struct waveData *wd;

  if (mode == ALC_OPEN_INPUT_)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "wave backend does not support input");
      *privateData = NULL;
      return;
    }

  sequence = 0;
  do
    {
      snprintf (fileName, sizeof (fileName), "openal-%d.wav", sequence++);
      fd = open (fileName, O_WRONLY | O_CREAT | O_EXCL | O_BINARY, S_IRUSR | S_IWUSR);
    }
  while ((fd == -1) && (errno == EEXIST));

  if (fd == -1)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "failed to open fresh file");
      *privateData = NULL;
      return;
    }

  fh = fdopen (fd, "wb");
  if (fh == NULL)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "fdopen failed");
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

  strncpy(wd->fileName, fileName, sizeof(wd->fileName));
  wd->fh = fh;
  wd->length = 0;
  wd->channels = 0;
  wd->speed = 0;
  wd->bytesPerSample = 0;

  /* write dummy header, will be patched on closing */
  if (!writeWAVEHeader (wd))
    {
      fclose (fh);
      free (wd);
      *privateData = NULL;
      return;
    }

  *ops = &waveOps;
  *privateData = (struct ALC_BackendPrivateData *) wd;
  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "using file descriptor %d", fd);
}

#endif /* USE_BACKEND_WAVEOUT */
