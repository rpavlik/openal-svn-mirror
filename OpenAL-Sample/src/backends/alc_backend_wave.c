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
                     ALC_BackendPrivateData **privateData)
{
  *privateData = NULL;
}

#else

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <AL/al.h>
#include <AL/alext.h>

#include "al_debug.h"

struct waveData
{
  FILE *fh;
  ALuint length;
  int channels;
  ALuint speed;
  int bytesPerSample;
};

#include "al_main.h"
#include "audioconvert/ac_endian.h"

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

/*
 *  FIXME: make endian correct
 */
static void writeWAVEHeader(struct waveData *wave) {
	ALushort writer16;
	ALuint   writer32;

        /* 'RIFF' */
	writer32 = RIFFMAGIC;
	fwrite(&writer32, 1, sizeof writer32, wave->fh);

	/* total length */
	writer32 = swap32le(wave->length);
	fwrite(&writer32, 1, sizeof writer32, wave->fh);

        /* 'WAVE' */
	writer32 = WAVEMAGIC;
	fwrite(&writer32, 1, sizeof writer32, wave->fh);

        /* 'fmt ' */
	writer32 = FMTMAGIC;
	fwrite(&writer32, 1, sizeof writer32, wave->fh);

        /* fmt chunk length */
	writer32 = swap32le(16);
	fwrite(&writer32, 1, sizeof writer32, wave->fh);

        /* ALushort encoding */
	writer16 = swap16le(1);
	fwrite(&writer16, 1, sizeof writer16, wave->fh);

	/* Alushort channels */
	writer16 = swap16le(wave->channels);
	fwrite(&writer16, 1, sizeof writer16, wave->fh);

	/* ALuint frequency */
	writer32 = swap32le(wave->speed);
	fwrite(&writer32, 1, sizeof writer32, wave->fh);

	/* ALuint byterate  */
	writer32 = swap32le(wave->speed / sizeof (ALshort)); /* FIXME */
	fwrite(&writer32, 1, sizeof writer32, wave->fh);

	/* ALushort blockalign */
	writer16 = 0;
	fwrite(&writer16, 1, sizeof writer16, wave->fh);

	/* ALushort bitspersample */
	writer16 = swap16le(wave->bytesPerSample * 8);
	fwrite(&writer16, 1, sizeof writer16, wave->fh);

        /* 'data' */
	writer32 = DATAMAGIC;
	fwrite(&writer32, 1, sizeof writer32, wave->fh);

	/* data length */
	writer32 = swap32le(wave->length - 44); /* samples */
	fwrite(&writer32, 1, sizeof writer32, wave->fh);

	fprintf(stderr, "waveout length %d\n", wave->length);

	return;
}

static void
closeWave (ALC_BackendPrivateData *privateData)
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

  writeWAVEHeader (wd);
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

static ALboolean
setAttributesWave (ALC_BackendPrivateData *privateData,
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
static void convert_to_little_endian( ALuint bps, void *data, int nbytes )
{
	assert( data );
	assert( nbytes > 0 );

	if( bps == 8 ) {
		/* 8-bit samples don't need to be converted */
		return;
	}

	assert( bps == 16 );

#ifdef WORDS_BIGENDIAN
	/* do the conversion */
	{
		ALshort *outp = data;
		ALuint i;
		for( i = 0; i < nbytes/sizeof(ALshort); i++ ) {
			outp[i] = swap16le( outp[i] );
		}
	}
#else
	(void)data; (void)nbytes;
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
	convert_to_little_endian( wd->bytesPerSample * 8,
				  (void *)data,
				  bytesToWrite );

	fwrite(data, 1, bytesToWrite, wd->fh);

	_alMicroSleep(1000000.0 * bytesToWrite / wd->speed);
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
      fd = open (fileName, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR);
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

  wd->fh = fh;
  wd->length = 0;
  wd->channels = 0;
  wd->speed = 0;
  wd->bytesPerSample = 0;

  /* write dummy header, will be patched on closing */
  writeWAVEHeader (wd);

  *ops = &waveOps;
  *privateData = (ALC_BackendPrivateData *) wd;
  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "using file descriptor %d", fd);
}

#endif /* USE_BACKEND_WAVEOUT */
