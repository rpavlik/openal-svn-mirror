/*
 * OpenAL Test
 *
 * Copyright (C) 1999-2000 by authors.
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundatio n, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA  02111-1307, USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */

/** notes for gendocs.py
 OpenAL Test Notes

 If there are any core OpenAL API shortcomings exposed by these
 tests at the code level, they are marked in this codebase by
 a comment string "*** API Issue ***".

 There are three classes of tests supported by this program:

    1) Fully Automated Testing
         These tests run entirely by themseves and require no
       user input.
    2) Semi-Automated Testing
         These tests require some analysis by the user. 
    3) Interactive Testing
         These tests allow a user to create their own OpenAL
       scenarios.

 To find the menu in the code, search for the string 
 "*** Main Menu ***".
  */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#define FILENAME_BOOM      "sounds/boom.ogg"
#define FILENAME_DING      "sounds/ding.wav"
#define FILENAME_FOOTADPCM "sounds/footadpcm.wav"
#define FILENAME_FOOTSTEPS "sounds/footsteps.wav"
#define FILENAME_STEREO    "sounds/stereo.wav"
#define FILENAME_WAVE1     "sounds/wave1.wav"
#define FILENAME_WAVE2     "sounds/wave2.wav"
#define FILENAME_WAVE3     "sounds/wave3.wav"
#define FILENAME_WAVE4     "sounds/wave4.wav"

#define FILENAME_RECORDING "recording.wav"

#define _CRT_SECURE_NO_DEPRECATE        /* get rid of sprintf security warnings on VS2005 */

#define INITGUID
#define OPENAL

#define TEST_EAX    0           /* enable for EAX testing */

/* get OpenAL header via obscure and fragile platform conditionals */
#if defined(_MSC_VER)
#include <alc.h>
#include <al.h>
#elif defined(__APPLE__)
#include <OpenAL/alc.h>
#include <OpenAL/al.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

/* get header for stat and friends, used for getting the file length only */
#include <sys/types.h>
#include <sys/stat.h>
#if HAVE_STAT
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#define structStat struct stat
#elif HAVE__STAT
#define stat(p,b) _stat((p),(b))
#define structStat struct _stat
#else
#error No stat-like function on this platform
#endif

#if HAVE_CONIO_H
#include <conio.h>
#define KBHIT() _kbhit()
#else
#define KBHIT() 1
#endif

#if HAVE_NANOSLEEP && HAVE_TIME_H
#include <time.h>
#include <errno.h>
#elif HAVE_USLEEP && HAVE_UNISTD_H
#include <unistd.h>
#elif HAVE_SLEEP && HAVE_WINDOWS_H
#include <windows.h>
#else
#error No way to sleep on this platform
#endif

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if TEST_EAX
#include <eax.h>
#endif

#define NUM_BUFFERS 8           /* Number of buffers to be Generated */

#ifndef LINUX
#ifndef MAC_OS_X
#pragma pack (push,1)           /* Turn off alignment */
#endif
#endif

#if HAVE_WINDOWS_H
#include <windows.h>
#else
typedef struct
{
  ALshort wFormatTag;
  ALshort nChannels;
  ALuint nSamplesPerSec;
  ALuint nAvgBytesPerSec;
  ALshort nBlockAlign;
  ALshort wBitsPerSample;
  ALshort cbSize;
} WAVEFORMATEX;
#endif /* _WIN32 */

typedef struct
{
  ALchar riff[4];               /* 'RIFF' */
  ALsizei riffSize;
  ALchar wave[4];               /* 'WAVE' */
  ALchar fmt[4];                /* 'fmt ' */
  ALuint fmtSize;
  ALushort Format;
  ALushort Channels;
  ALuint SamplesPerSec;
  ALuint BytesPerSec;
  ALushort BlockAlign;
  ALushort BitsPerSample;
  ALchar data[4];               /* 'data' */
  ALuint dataSize;
} WAVE_Struct;

#pragma pack (push,1)
typedef struct
{
  char szRIFF[4];
  long lRIFFSize;
  char szWave[4];
  char szFmt[4];
  long lFmtSize;
  WAVEFORMATEX wfex;
  char szData[4];
  long lDataSize;
} WAVEHEADER;
#pragma pack (pop)

#define QUEUEBUFFERSIZE         2048
#define QUEUEBUFFERCOUNT        4

typedef struct ALCenum_struct
{
  const ALCchar *enumName;
  ALCenum expectedValue;
} ALCenums;

typedef struct ALenum_struct
{
  const ALchar *enumName;
  ALenum expectedValue;
} ALenums;

#ifndef LINUX
#ifndef MAC_OS_X
#pragma pack (pop)
#endif
#endif

/* Global variables */
ALuint g_Buffers[NUM_BUFFERS];  /* Array of Buffers */
ALboolean g_bNewDistModels = AL_TRUE;
ALboolean g_bOffsetExt = AL_TRUE;
ALboolean g_bCaptureExt = AL_TRUE;

#if TEST_EAX
EAXSet eaxSet;                  /* EAXSet function, retrieved if EAX Extension is supported */
EAXGet eaxGet;                  /* EAXGet function, retrieved if EAX Extension is supported */
ALboolean g_bEAX;               /* Boolean variable to indicate presence of EAX Extension */
#endif

unsigned int g_ovSize;

static ALCenums enumerationALC[] = {
  /* Types */
  {(const ALCchar *) "ALC_FALSE", ALC_FALSE},
  {(const ALCchar *) "ALC_TRUE", ALC_TRUE},

  /* ALC Properties */
  {(const ALCchar *) "ALC_MAJOR_VERSION", ALC_MAJOR_VERSION},
  {(const ALCchar *) "ALC_MINOR_VERSION", ALC_MINOR_VERSION},
  {(const ALCchar *) "ALC_ATTRIBUTES_SIZE", ALC_ATTRIBUTES_SIZE},
  {(const ALCchar *) "ALC_ALL_ATTRIBUTES", ALC_ALL_ATTRIBUTES},
  {(const ALCchar *) "ALC_DEFAULT_DEVICE_SPECIFIER",
   ALC_DEFAULT_DEVICE_SPECIFIER},
  {(const ALCchar *) "ALC_DEVICE_SPECIFIER", ALC_DEVICE_SPECIFIER},
  {(const ALCchar *) "ALC_EXTENSIONS", ALC_EXTENSIONS},
  {(const ALCchar *) "ALC_FREQUENCY", ALC_FREQUENCY},
  {(const ALCchar *) "ALC_REFRESH", ALC_REFRESH},
  {(const ALCchar *) "ALC_SYNC", ALC_SYNC},
  {(const ALCchar *) "ALC_MONO_SOURCES", ALC_MONO_SOURCES},
  {(const ALCchar *) "ALC_STEREO_SOURCES", ALC_STEREO_SOURCES},
  {(const ALCchar *) "ALC_CAPTURE_DEVICE_SPECIFIER",
   ALC_CAPTURE_DEVICE_SPECIFIER},
  {(const ALCchar *) "ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER",
   ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER},
  {(const ALCchar *) "ALC_CAPTURE_SAMPLES", ALC_CAPTURE_SAMPLES},

  /* ALC Error Message */
  {(const ALCchar *) "ALC_NO_ERROR", ALC_NO_ERROR},
  {(const ALCchar *) "ALC_INVALID_DEVICE", ALC_INVALID_DEVICE},
  {(const ALCchar *) "ALC_INVALID_CONTEXT", ALC_INVALID_CONTEXT},
  {(const ALCchar *) "ALC_INVALID_ENUM", ALC_INVALID_ENUM},
  {(const ALCchar *) "ALC_INVALID_VALUE", ALC_INVALID_VALUE},
  {(const ALCchar *) "ALC_OUT_OF_MEMORY", ALC_OUT_OF_MEMORY},

  /* Default */
  {(const ALCchar *) NULL, (ALCenum) 0}
};

static ALenums enumerationAL[] = {
  /* Types */
  {(const ALchar *) "AL_NONE", AL_NONE},
  {(const ALchar *) "AL_FALSE", AL_FALSE},
  {(const ALchar *) "AL_TRUE", AL_TRUE},

  /* Source and Listener Properties */
  {(const ALchar *) "AL_SOURCE_RELATIVE", AL_SOURCE_RELATIVE},
  {(const ALchar *) "AL_CONE_INNER_ANGLE", AL_CONE_INNER_ANGLE},
  {(const ALchar *) "AL_CONE_OUTER_ANGLE", AL_CONE_OUTER_ANGLE},
  {(const ALchar *) "AL_PITCH", AL_PITCH},
  {(const ALchar *) "AL_POSITION", AL_POSITION},
  {(const ALchar *) "AL_DIRECTION", AL_DIRECTION},
  {(const ALchar *) "AL_VELOCITY", AL_VELOCITY},
  {(const ALchar *) "AL_LOOPING", AL_LOOPING},
  {(const ALchar *) "AL_BUFFER", AL_BUFFER},
  {(const ALchar *) "AL_GAIN", AL_GAIN},
  {(const ALchar *) "AL_MIN_GAIN", AL_MIN_GAIN},
  {(const ALchar *) "AL_MAX_GAIN", AL_MAX_GAIN},
  {(const ALchar *) "AL_ORIENTATION", AL_ORIENTATION},
  {(const ALchar *) "AL_REFERENCE_DISTANCE", AL_REFERENCE_DISTANCE},
  {(const ALchar *) "AL_ROLLOFF_FACTOR", AL_ROLLOFF_FACTOR},
  {(const ALchar *) "AL_CONE_OUTER_GAIN", AL_CONE_OUTER_GAIN},
  {(const ALchar *) "AL_MAX_DISTANCE", AL_MAX_DISTANCE},
  {(const ALchar *) "AL_SEC_OFFSET", AL_SEC_OFFSET},
  {(const ALchar *) "AL_SAMPLE_OFFSET", AL_SAMPLE_OFFSET},
  {(const ALchar *) "AL_BYTE_OFFSET", AL_BYTE_OFFSET},
  {(const ALchar *) "AL_SOURCE_TYPE", AL_SOURCE_TYPE},
  {(const ALchar *) "AL_STATIC", AL_STATIC},
  {(const ALchar *) "AL_STREAMING", AL_STREAMING},
  {(const ALchar *) "AL_UNDETERMINED", AL_UNDETERMINED},

  /* Source State information */
  {(const ALchar *) "AL_SOURCE_STATE", AL_SOURCE_STATE},
  {(const ALchar *) "AL_INITIAL", AL_INITIAL},
  {(const ALchar *) "AL_PLAYING", AL_PLAYING},
  {(const ALchar *) "AL_PAUSED", AL_PAUSED},
  {(const ALchar *) "AL_STOPPED", AL_STOPPED},

  /* Queue information */
  {(const ALchar *) "AL_BUFFERS_QUEUED", AL_BUFFERS_QUEUED},
  {(const ALchar *) "AL_BUFFERS_PROCESSED", AL_BUFFERS_PROCESSED},

  /* Buffer Formats */
  {(const ALchar *) "AL_FORMAT_MONO8", AL_FORMAT_MONO8},
  {(const ALchar *) "AL_FORMAT_MONO16", AL_FORMAT_MONO16},
  {(const ALchar *) "AL_FORMAT_STEREO8", AL_FORMAT_STEREO8},
  {(const ALchar *) "AL_FORMAT_STEREO16", AL_FORMAT_STEREO16},

  /* Buffer attributes */
  {(const ALchar *) "AL_FREQUENCY", AL_FREQUENCY},
  {(const ALchar *) "AL_BITS", AL_BITS},
  {(const ALchar *) "AL_CHANNELS", AL_CHANNELS},
  {(const ALchar *) "AL_SIZE", AL_SIZE},

  /* Buffer States (not supported yet) */
  {(const ALchar *) "AL_UNUSED", AL_UNUSED},
  {(const ALchar *) "AL_PENDING", AL_PENDING},
  {(const ALchar *) "AL_PROCESSED", AL_PROCESSED},

  /* AL Error Messages */
  {(const ALchar *) "AL_NO_ERROR", AL_NO_ERROR},
  {(const ALchar *) "AL_INVALID_NAME", AL_INVALID_NAME},
  {(const ALchar *) "AL_INVALID_ENUM", AL_INVALID_ENUM},
  {(const ALchar *) "AL_INVALID_VALUE", AL_INVALID_VALUE},
  {(const ALchar *) "AL_INVALID_OPERATION", AL_INVALID_OPERATION},
  {(const ALchar *) "AL_OUT_OF_MEMORY", AL_OUT_OF_MEMORY},

  /* Context strings */
  {(const ALchar *) "AL_VENDOR", AL_VENDOR},
  {(const ALchar *) "AL_VERSION", AL_VERSION},
  {(const ALchar *) "AL_RENDERER", AL_RENDERER},
  {(const ALchar *) "AL_EXTENSIONS", AL_EXTENSIONS},

  /* Global states */
  {(const ALchar *) "AL_DOPPLER_FACTOR", AL_DOPPLER_FACTOR},
  {(const ALchar *) "AL_DOPPLER_VELOCITY", AL_DOPPLER_VELOCITY},
  {(const ALchar *) "AL_DISTANCE_MODEL", AL_DISTANCE_MODEL},
  {(const ALchar *) "AL_SPEED_OF_SOUND", AL_SPEED_OF_SOUND},

  /* Distance Models */
  {(const ALchar *) "AL_INVERSE_DISTANCE", AL_INVERSE_DISTANCE},
  {(const ALchar *) "AL_INVERSE_DISTANCE_CLAMPED",
   AL_INVERSE_DISTANCE_CLAMPED},
  {(const ALchar *) "AL_LINEAR_DISTANCE", AL_LINEAR_DISTANCE},
  {(const ALchar *) "AL_LINEAR_DISTANCE_CLAMPED", AL_LINEAR_DISTANCE_CLAMPED},
  {(const ALchar *) "AL_EXPONENT_DISTANCE", AL_EXPONENT_DISTANCE},
  {(const ALchar *) "AL_EXPONENT_DISTANCE_CLAMPED",
   AL_EXPONENT_DISTANCE_CLAMPED},

  /* Default */
  {(const ALchar *) NULL, (ALenum) 0}
};

/* Function prototypes */
ALvoid DisplayALError (ALchar *szText, ALint errorcode);

char getUpperCh (void);
int CRToContinue (void);
void CRForNextTest (void);
int ContinueOrSkip (void);
ALvoid FullAutoTests (ALvoid);
ALvoid SemiAutoTests (ALvoid);

#ifdef __MACOS__
void ALTestSwapWords (unsigned int *puint);
void ALTestSwapBytes (unsigned short *pshort);
#endif
#ifdef MAC_OS_X
void ALTestSwapWords (unsigned int *puint);
void ALTestSwapBytes (unsigned short *pshort);
#endif

/* Test Function prototypes */
ALvoid I_PositionTest (ALvoid);
ALvoid I_LoopingTest (ALvoid);
ALvoid I_QueueTest (ALvoid);
ALvoid I_BufferTest (ALvoid);
ALvoid I_FreqTest (ALvoid);
ALvoid I_StereoTest (ALvoid);
ALvoid I_GainTest (ALvoid);
ALvoid I_StreamingTest (ALvoid);
ALvoid I_MultipleSourcesTest (ALvoid);
ALvoid I_SourceRelativeTest (ALvoid);
ALvoid I_SourceRolloffTest (ALvoid);
ALvoid I_ADPCMTest (ALvoid);
ALvoid I_VelocityTest (ALvoid);
ALvoid I_GetSourceOffsetTest (ALvoid);
ALvoid I_SetSourceOffsetTest (ALvoid);
ALvoid I_DistanceModelTest (ALvoid);
ALvoid I_CaptureTest (ALvoid);
ALvoid I_CaptureAndPlayTest (ALvoid);
ALvoid FA_RequestObjectNames (ALvoid);
ALvoid FA_ReleaseObjectNames (ALvoid);
ALvoid FA_ValidateObjectNames (ALvoid);
ALvoid FA_StateTransition (ALvoid);
ALvoid FA_VectorStateTransition (ALvoid);
ALvoid FA_GetBufferProperties (ALvoid);
ALvoid FA_EnumerationValue (ALvoid);
ALvoid FA_QueuingUnderrunStates (ALvoid);
ALvoid SA_StringQueries (ALvoid);
ALvoid SA_SourceGain (ALvoid);
ALvoid SA_ListenerGain (ALvoid);
ALvoid SA_Position (ALvoid);
ALvoid SA_SourceRelative (ALvoid);
ALvoid SA_ListenerOrientation (ALvoid);
ALvoid SA_SourceCone (ALvoid);
ALvoid SA_MinMaxGain (ALvoid);
ALvoid SA_ReferenceDistance (ALvoid);
ALvoid SA_RolloffFactor (ALvoid);
ALvoid SA_DistanceModel (ALvoid);
ALvoid SA_Doppler (ALvoid);
ALvoid SA_Frequency (ALvoid);
ALvoid SA_Stereo (ALvoid);
ALvoid SA_Streaming (ALvoid);
ALvoid SA_QueuingUnderrunPerformance (ALvoid);
ALvoid I_VorbisTest (ALvoid);

#if TEST_EAX
ALvoid I_EAXTest (ALvoid);
#endif

/* cut-n-paste from freealut plus some simplifications */
static void
sleepSeconds (ALfloat duration)
{
  if (duration < 0)
    {
      return;
    }

  {
    ALuint seconds = (ALuint) duration;
    ALfloat rest = duration - (ALfloat) seconds;

#if HAVE_NANOSLEEP && HAVE_TIME_H

    ALuint microSecs = (ALuint) (rest * 1000000);
    struct timespec t, remainingTime;
    t.tv_sec = (time_t) seconds;
    t.tv_nsec = ((long) microSecs) * 1000;

    /* At least the interaction of nanosleep and signals is specified! */
    while (nanosleep (&t, &remainingTime) < 0)
      {
        if (errno != EINTR)
          {
            return;
          }
        /* If we received a signal, let's try again with the remaining time. */
        t.tv_sec = remainingTime.tv_sec;
        t.tv_nsec = remainingTime.tv_nsec;
      }

#elif HAVE_USLEEP && HAVE_UNISTD_H

    while (seconds > 0)
      {
        usleep (1000000);
        seconds--;
      }
    usleep ((unsigned int) (rest * 1000000));

#elif HAVE_SLEEP && HAVE_WINDOWS_H

    while (seconds > 0)
      {
        Sleep (1000);
        seconds--;
      }
    Sleep ((DWORD) (rest * 1000));

#endif

  }
}

/*
        Display AL Error message
*/
ALvoid
DisplayALError (ALchar *szText, ALint errorcode)
{
  printf ("%s%s\n", szText, alGetString (errorcode));
}

/* ***** GH -- OS X is going to need some byte-swaps and whatnot... */

#if defined _MSC_VER
#pragma pack (push,1)           /* Turn off alignment */
#elif defined __GNUC__
#define PADOFF_VAR __attribute__((packed))
#endif

#ifndef PADOFF_VAR
#define PADOFF_VAR
#endif

typedef struct                  /* WAV File-header */
{
  ALubyte Id[4] PADOFF_VAR;
  ALsizei Size PADOFF_VAR;
  ALubyte Type[4] PADOFF_VAR;
} WAVFileHdr_Struct;

typedef struct                  /* WAV Fmt-header */
{
  ALushort Format PADOFF_VAR;
  ALushort Channels PADOFF_VAR;
  ALuint SamplesPerSec PADOFF_VAR;
  ALuint BytesPerSec PADOFF_VAR;
  ALushort BlockAlign PADOFF_VAR;
  ALushort BitsPerSample PADOFF_VAR;
} WAVFmtHdr_Struct;

typedef struct                  /* WAV FmtEx-header */
{
  ALushort Size PADOFF_VAR;
  ALushort SamplesPerBlock PADOFF_VAR;
} WAVFmtExHdr_Struct;

typedef struct                  /* WAV Smpl-header */
{
  ALuint Manufacturer PADOFF_VAR;
  ALuint Product PADOFF_VAR;
  ALuint SamplePeriod PADOFF_VAR;
  ALuint Note PADOFF_VAR;
  ALuint FineTune PADOFF_VAR;
  ALuint SMPTEFormat PADOFF_VAR;
  ALuint SMPTEOffest PADOFF_VAR;
  ALuint Loops PADOFF_VAR;
  ALuint SamplerData PADOFF_VAR;
  struct
  {
    ALuint Identifier PADOFF_VAR;
    ALuint Type PADOFF_VAR;
    ALuint Start PADOFF_VAR;
    ALuint End PADOFF_VAR;
    ALuint Fraction PADOFF_VAR;
    ALuint Count PADOFF_VAR;
  } Loop[1] PADOFF_VAR;
} WAVSmplHdr_Struct;

typedef struct                  /* WAV Chunk-header */
{
  ALubyte Id[4] PADOFF_VAR;
  ALuint Size PADOFF_VAR;
} WAVChunkHdr_Struct;

#ifdef PADOFF_VAR               /* Default alignment */
#undef PADOFF_VAR
#endif

#if defined _MSC_VER
#pragma pack (pop)
#endif

ALvoid
getWAVData (const ALbyte *file, ALenum *format, ALvoid **data, ALsizei *size,
            ALsizei *freq, ALboolean *loop)
{
  WAVChunkHdr_Struct ChunkHdr;
  WAVFmtExHdr_Struct FmtExHdr;
  WAVFileHdr_Struct FileHdr;
  WAVSmplHdr_Struct SmplHdr;
  WAVFmtHdr_Struct FmtHdr;
  FILE *Stream;

  *format = AL_FORMAT_MONO16;
  *data = NULL;
  *size = 0;
  *freq = 22050;
  *loop = AL_FALSE;
  if (file)
    {
      Stream = fopen (file, "rb");
      if (Stream)
        {
          fread (&FileHdr, 1, sizeof (WAVFileHdr_Struct), Stream);
          FileHdr.Size = ((FileHdr.Size + 1) & ~1) - 4;
          while ((FileHdr.Size != 0)
                 &&
                 (fread (&ChunkHdr, 1, sizeof (WAVChunkHdr_Struct), Stream)))
            {
              if (!memcmp (ChunkHdr.Id, "fmt ", 4))
                {
                  fread (&FmtHdr, 1, sizeof (WAVFmtHdr_Struct), Stream);
                  if ((FmtHdr.Format == 0x0001) || (FmtHdr.Format == 0xFFFE))
                    {
                      if (FmtHdr.Channels == 1)
                        *format =
                          (FmtHdr.BitsPerSample ==
                           4 ? alGetEnumValue ("AL_FORMAT_MONO_IMA4")
                           : (FmtHdr.BitsPerSample ==
                              8 ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16));
                      else if (FmtHdr.Channels == 2)
                        *format =
                          (FmtHdr.BitsPerSample ==
                           4 ? alGetEnumValue ("AL_FORMAT_STEREO_IMA4")
                           : (FmtHdr.BitsPerSample ==
                              8 ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16));
                      *freq = FmtHdr.SamplesPerSec;
                      fseek (Stream,
                             ChunkHdr.Size - sizeof (WAVFmtHdr_Struct),
                             SEEK_CUR);
                    }
                  else if (FmtHdr.Format == 0x0011)
                    {
                      if (FmtHdr.Channels == 1)
                        *format = alGetEnumValue ("AL_FORMAT_MONO_IMA4");
                      else if (FmtHdr.Channels == 2)
                        *format = alGetEnumValue ("AL_FORMAT_STEREO_IMA4");
                      *freq = FmtHdr.SamplesPerSec;
                      fseek (Stream,
                             ChunkHdr.Size - sizeof (WAVFmtHdr_Struct),
                             SEEK_CUR);
                    }
                  else if (FmtHdr.Format == 0x0055)
                    {
                      *format = alGetEnumValue ("AL_FORMAT_MP3");
                      *freq = FmtHdr.SamplesPerSec;
                      fseek (Stream,
                             ChunkHdr.Size - sizeof (WAVFmtHdr_Struct),
                             SEEK_CUR);
                    }
                  else
                    {
                      fread (&FmtExHdr, 1, sizeof (WAVFmtExHdr_Struct),
                             Stream);
                      fseek (Stream,
                             ChunkHdr.Size - sizeof (WAVFmtHdr_Struct) -
                             sizeof (WAVFmtExHdr_Struct), SEEK_CUR);
                    }
                }
              else if (!memcmp (ChunkHdr.Id, "data", 4))
                {
                  *size = ChunkHdr.Size;
                  *data = malloc (ChunkHdr.Size + 31);
                  if (*data)
                    fread (*data, FmtHdr.BlockAlign,
                           ChunkHdr.Size / FmtHdr.BlockAlign, Stream);
                  memset (((char *) *data) + ChunkHdr.Size, 0, 31);
                }
              else if (!memcmp (ChunkHdr.Id, "smpl", 4))
                {
                  fread (&SmplHdr, 1, sizeof (WAVSmplHdr_Struct), Stream);
                  *loop = (SmplHdr.Loops ? AL_TRUE : AL_FALSE);
                  fseek (Stream, ChunkHdr.Size - sizeof (WAVSmplHdr_Struct),
                         SEEK_CUR);
                }
              else
                fseek (Stream, ChunkHdr.Size, SEEK_CUR);
              fseek (Stream, ChunkHdr.Size & 1, SEEK_CUR);
              FileHdr.Size -= (((ChunkHdr.Size + 1) & ~1) + 8);
            }
          fclose (Stream);
        }

    }
}

ALvoid
unloadWAVData (ALvoid *data)
{
  if (data)
    free (data);
}

/*
        Loads the wave file into the given Buffer ID
*/
ALboolean
LoadWave (char *szWaveFile, ALuint BufferID)
{
  ALint error;
  ALsizei size, freq;
  ALenum format;
  ALvoid *data = 0;
  ALboolean loop;

  if (!szWaveFile)
    return AL_FALSE;

  getWAVData (szWaveFile, &format, &data, &size, &freq, &loop);
  if (!data)
    {
      printf ("Failed to load %s\n", szWaveFile);
      return AL_FALSE;
    }

  /* Copy data into ALBuffer */
  alBufferData (BufferID, format, data, size, freq);
  if ((error = alGetError ()) != AL_NO_ERROR)
    {
      DisplayALError ("alBufferData : ", error);
      return AL_FALSE;
    }

  unloadWAVData (data);

  return AL_TRUE;
}

/*
        Main application - Initializes Open AL, Sets up Listener, Generates Buffers, and loads in audio data.
        Displays Test Menu and calls appropriate Test functions.  On exit, buffers are destroyed and Open AL
        is shutdown.

        Each Test function is responsible for creating and destroying whatever Sources they require. All test
        applications use the same set of Buffers (created in this function).
*/
int
main (int argc, char *argv[])
{
  ALbyte ch;
  ALint error;
  ALCcontext *Context;
  ALCdevice *Device;
  ALfloat listenerPos[] = { 0.0, 0.0, 0.0 };
  ALfloat listenerVel[] = { 0.0, 0.0, 0.0 };
  ALfloat listenerOri[] = { 0.0, 0.0, -1.0, 0.0, 1.0, 0.0 };    /* Listener facing into the screen */

#if TEST_EAX
  ALchar szFnName[128];
  ALchar szEAX[] = "EAX";
#endif

  /* Initialize Open AL manually */
  /*Open device */
  char deviceName[256];
  char *defaultDevice;
  char *deviceList;
  char *devices[12];
  int numDevices, numDefaultDevice = 0, i;
  ALint major, minor;
  ALboolean g_bCaptureExt;

  printf ("OpenAL Test Application\n");
  printf ("=======================\n\n");

  strcpy (deviceName, "");
  if (alcIsExtensionPresent (NULL, "ALC_ENUMERATION_EXT") == AL_TRUE)
    {                           /* try out enumeration extension */
      defaultDevice =
        (char *) alcGetString (NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
      if ((defaultDevice != NULL) && (strlen (defaultDevice) > 0))
        {
          deviceList = (char *) alcGetString (NULL, ALC_DEVICE_SPECIFIER);
          for (numDevices = 0; numDevices < 12; numDevices++)
            {
              devices[numDevices] = NULL;
            }
          for (numDevices = 0; numDevices < 12; numDevices++)
            {
              devices[numDevices] = deviceList;
              if (strcmp (devices[numDevices], defaultDevice) == 0)
                {
                  numDefaultDevice = numDevices;
                }
              deviceList += strlen (deviceList);
              if (deviceList[0] == 0)
                {
                  if (deviceList[1] == 0)
                    {
                      break;
                    }
                  else
                    {
                      deviceList += 1;
                    }
                }
            }
          if (devices[numDevices] != NULL)
            {
              numDevices++;
              printf
                ("\nEnumeration extension found -- select an output device:\n");
              printf ("0. NULL Device (Default)\n");
              for (i = 0; i < numDevices; i++)
                {
                  printf ("%d. %s", i + 1, devices[i]);
                  if (i == numDefaultDevice)
                    {
                      printf ("  (default device)");
                    }
                  printf ("\n");
                }
              printf ("\n\n");
              do
                {
                  ch = getUpperCh ();
                  i = atoi (&ch);
                }
              while ((i < 0) || (i > numDevices));
              if ((i != 0) && (strlen (devices[i - 1]) < 256))
                {
                  strcpy (deviceName, devices[i - 1]);
                }
            }
        }
    }

  if (strlen (deviceName) == 0)
    {
      Device = alcOpenDevice (NULL);    /* this is supposed to select the "preferred device" */
    }
  else
    {
      Device = alcOpenDevice ((ALchar *) deviceName);   /* have a name from enumeration process above, so use it... */
    }
  /*Create context(s) */
  Context = alcCreateContext (Device, NULL);
  /*Set active context */
  alcMakeContextCurrent (Context);

  /* Clear Error Codes */
  alGetError ();
  alcGetError (Device);

  /* Check what version of Open AL we are using */
  alcGetIntegerv (Device, ALC_MAJOR_VERSION, 1, &major);
  alcGetIntegerv (Device, ALC_MINOR_VERSION, 1, &minor);
  printf ("\nOpen AL Version %d.%d\n", major, minor);

  /* Check for all the AL 1.1 Extensions (they may be present on AL 1.0 implementations too) */
  g_bOffsetExt = alIsExtensionPresent ("al_ext_offset");
  if (g_bOffsetExt)
    printf ("AL_EXT_OFFSET support found !\n");

  g_bNewDistModels &= alIsExtensionPresent ("AL_EXT_LINEAR_DISTANCE");
  g_bNewDistModels &= alIsExtensionPresent ("AL_EXT_EXPONENT_DISTANCE");
  if (g_bNewDistModels)
    printf
      ("AL_EXT_LINEAR_DISTANCE and AL_EXT_EXPONENT_DISTANCE support found !\n");

  g_bCaptureExt = alcIsExtensionPresent (Device, "alc_EXT_capTure");
  if (g_bCaptureExt)
    printf ("ALC_EXT_CAPTURE support found !\n");

  /* Set Listener attributes */

  /* Position ... */
  alListenerfv (AL_POSITION, listenerPos);
  if ((error = alGetError ()) != AL_NO_ERROR)
    {
      DisplayALError ((ALbyte *) "alListenerfv POSITION : ", error);
      exit (-1);
    }

  /* Velocity ... */
  alListenerfv (AL_VELOCITY, listenerVel);
  if ((error = alGetError ()) != AL_NO_ERROR)
    {
      DisplayALError ((ALbyte *) "alListenerfv VELOCITY : ", error);
      exit (-1);
    }

  /* Orientation ... */
  alListenerfv (AL_ORIENTATION, listenerOri);
  if ((error = alGetError ()) != AL_NO_ERROR)
    {
      DisplayALError ((ALbyte *) "alListenerfv ORIENTATION : ", error);
      exit (-1);
    }

  /* Generate Buffers */
  alGenBuffers (NUM_BUFFERS, g_Buffers);
  if ((error = alGetError ()) != AL_NO_ERROR)
    {
      DisplayALError ((ALbyte *) "alGenBuffers :", error);
      exit (-1);
    }

  /* Load in samples to be used by Test functions */
  if ((!LoadWave (FILENAME_FOOTSTEPS, g_Buffers[0])) ||
      (!LoadWave (FILENAME_DING, g_Buffers[1])) ||
      (!LoadWave (FILENAME_WAVE1, g_Buffers[2])) ||
      (!LoadWave (FILENAME_WAVE2, g_Buffers[3])) ||
      (!LoadWave (FILENAME_WAVE3, g_Buffers[4])) ||
      (!LoadWave (FILENAME_WAVE4, g_Buffers[5])) ||
      (!LoadWave (FILENAME_STEREO, g_Buffers[6])))
    {
      alDeleteBuffers (NUM_BUFFERS, g_Buffers);
      exit (-1);
    }

  if (alIsExtensionPresent ((ALchar *) "AL_EXT_vorbis") == AL_TRUE)
    {
      void *ovdata;

      FILE *fh;
      printf ("Ogg Vorbis extension available.\n");
      fh = fopen (FILENAME_BOOM, "rb");
      if (fh != NULL)
        {
          structStat sbuf;
          if (stat (FILENAME_BOOM, &sbuf) != -1)
            {
              g_ovSize = sbuf.st_size;
              ovdata = malloc (g_ovSize);

              if (ovdata != NULL)
                {
                  ALenum formatVorbis =
                    alGetEnumValue ("AL_FORMAT_VORBIS_EXT");
                  fread (ovdata, 1, g_ovSize, fh);

                  /* Copy boom data into AL Buffer 7 */
                  alGetError ();
                  alBufferData (g_Buffers[7], formatVorbis, ovdata, g_ovSize,
                                1);
                  error = alGetError ();
                  if (error != AL_NO_ERROR)
                    {
                      DisplayALError ((ALbyte *) "alBufferData buffer 7 : ",
                                      error);
                    }
                  free (ovdata);
                }
              fclose (fh);
            }
        }
    }

#if TEST_EAX
  /* Check for EAX extension */
  g_bEAX = alIsExtensionPresent (szEAX);

  if (g_bEAX)
    printf ("EAX Extensions available\n");

  sprintf ((char *) szFnName, "EAXSet");

  eaxSet = (EAXSet) alGetProcAddress (szFnName);

  if (eaxSet == NULL)
    g_bEAX = AL_FALSE;

  sprintf ((char *) szFnName, "EAXGet");

  eaxGet = (EAXGet) alGetProcAddress (szFnName);

  if (eaxGet == NULL)
    g_bEAX = AL_FALSE;
#endif

  /* *** Main Menu *** */
  do
    {
      printf ("\n\n\nAutomated Test Series:\n\n");
      printf ("A) Run Fully Automated Tests\n");
      printf ("B) Run Semi-Automated Tests\n");
      printf ("\nInteractive Tests:\n\n");
      printf ("1) Position Test\n");
      printf ("2) Looping Test\n");
      printf ("3) Queue Test\n");
      printf ("4) Buffer Test\n");
      printf ("5) Frequency Test\n");
      printf ("6) Stereo Test\n");
      printf ("7) Gain Test\n");
      printf ("8) Streaming Test\n");
      printf ("9) Multiple Sources Test\n");
      printf ("C) Source Relative Test\n");
      printf ("D) Source Rolloff Test\n");
      printf ("E) ADPCM Play Test\n");
      printf ("F) Velocity Test\n");
      printf ("G) Get Source Offset Test\n");
      printf ("H) Set Source Offset Test\n");
      printf ("I) Distance Model Test\n");
      printf ("J) Capture Test\n");
      printf ("K) Capture and Play Test\n");
#if TEST_EAX
      printf ("L) EAX Test\n");
#endif
      printf ("M) Ogg Vorbis Test\n");
      printf ("\nQ) to quit\n\n\n");

      ch = getUpperCh ();

      switch (ch)
        {
        case 'A':
          FullAutoTests ();
          break;
        case 'B':
          SemiAutoTests ();
          break;
        case '1':
          I_PositionTest ();
          break;
        case '2':
          I_LoopingTest ();
          break;
        case '3':
          I_QueueTest ();
          break;
        case '4':
          I_BufferTest ();
          break;
        case '5':
          I_FreqTest ();
          break;
        case '6':
          I_StereoTest ();
          break;
        case '7':
          I_GainTest ();
          break;
        case '8':
          I_StreamingTest ();
          break;
        case '9':
          I_MultipleSourcesTest ();
          break;
        case 'C':
          I_SourceRelativeTest ();
          break;
        case 'D':
          I_SourceRolloffTest ();
          break;
        case 'E':
          I_ADPCMTest ();
          break;
        case 'F':
          I_VelocityTest ();
          break;
        case 'G':
          if (g_bOffsetExt)
            {
              I_GetSourceOffsetTest ();
            }
          else
            {
              printf ("Offset Extension not supported.\n\n");
            }
          break;
        case 'H':
          if (g_bOffsetExt)
            {
              I_SetSourceOffsetTest ();
            }
          else
            {
              printf ("Offset Extension not supported.\n\n");
            }
          break;
        case 'I':
          I_DistanceModelTest ();
          break;
        case 'J':
          if (g_bCaptureExt)
            {
              I_CaptureTest ();
            }
          else
            {
              printf ("Capture Extension not supported.\n\n");
            }
          break;
        case 'K':
          if (g_bCaptureExt)
            {
              I_CaptureAndPlayTest ();
            }
          else
            {
              printf ("Capture Extension not supported.\n\n");
            }
          break;
#if TEST_EAX
        case 'L':
          if (g_bEAX)
            I_EAXTest ();
          break;
#endif
        case 'M':
          I_VorbisTest ();
          break;
        default:
          break;
        }
    }
  while (ch != 'Q');

  alGetError ();                /* clear error state */
  alDeleteBuffers (NUM_BUFFERS, g_Buffers);
  if ((error = alGetError ()) != AL_NO_ERROR)
    {
      DisplayALError ((ALbyte *) "alDeleteBuffers : ", error);
      exit (-1);
    }

  /*Get active context */
  Context = alcGetCurrentContext ();
  /*Get device for active context */
  Device = alcGetContextsDevice (Context);
  /*Release context(s) */
  alcDestroyContext (Context);
  /*Close device */
  alcCloseDevice (Device);

  printf ("\nProgram Terminated\n");

  return 0;
}

/*
        Fully Automatic Tests
*/
ALvoid
FullAutoTests (ALvoid)
{
  FA_RequestObjectNames ();     /* Request Object Names */
  FA_ReleaseObjectNames ();     /* Release Object Names */
  FA_ValidateObjectNames ();    /* Validating Object Names */
  FA_StateTransition ();        /* State Transistion Testing */
  FA_VectorStateTransition ();  /* Vector State Transistion Testing */
  FA_GetBufferProperties ();    /* Get Buffer Properties */
  FA_EnumerationValue ();       /* Enumeration Value Test */
  FA_QueuingUnderrunStates ();  /* test underrun while queuing */

  printf ("\n\n");
  sleepSeconds (1.0f);
}

char
getUpperCh (void)
{
  char ch;

#ifdef _WIN32
  ch = _getch ();
#else
  int bs;
  ch = '\0';
  bs = getchar ();
  while (bs != 10)
    {
      ch = bs;
      bs = getchar ();
    }
#endif

  ch = toupper (ch);

  return ch;
}

int
CRToContinue ()
{
  ALchar ch = 0;
  ALchar lastchar = 0;

  do
    {
      lastchar = ch;
      ch = getchar ();
    }
  while (ch != 10);

  return lastchar;
}

void
CRForNextTest ()
{
  printf ("\nPress Return to continue on to the next test.\n");

  CRToContinue ();
}

int
ContinueOrSkip ()
{
  ALchar ch;

  printf ("\nPress Return to run this test, or 'S' to skip:\n");

  while (1)
    {
      ch = CRToContinue ();
      if ((ch == 'S') || (ch == 's'))
        {
          return 0;
        }
      if (ch == 0)
        {
          return 1;
        }
    }
}

/*
        Semi Automatic Tests
*/
ALvoid
SemiAutoTests (ALvoid)
{
  SA_StringQueries ();          /* String Queries Test */
  SA_SourceGain ();             /* Source Gain Test */
  SA_ListenerGain ();           /* Listener Gain Test */
  SA_Position ();               /* Position Test */
  SA_SourceRelative ();         /* Source Relative Test */
  SA_ListenerOrientation ();    /* Listener Orientation Test */
  SA_SourceCone ();             /* Source Cone Test */
  SA_MinMaxGain ();             /* MIN/MAX Gain Test */
  SA_ReferenceDistance ();      /* Reference Distance Test */
  SA_RolloffFactor ();          /* Rolloff Factor Test */
  SA_DistanceModel ();          /* Distance Model Test */
  SA_Doppler ();                /* Doppler Test */
  SA_Frequency ();              /* Frequency Test */
  SA_Stereo ();                 /* Stereo Test */
  SA_Streaming ();              /* Streaming Test */
  SA_QueuingUnderrunPerformance ();     /* test underrun performance */

  printf ("\nDone with this series of tests. Going back to the main menu...");
  sleepSeconds (1.0f);
}

/** used by gendocs.py
$SECTION Fully Automatic Tests
$SUBTITLE Request Object Names
This test checks that OpenAL responds correctly to the creation of
zero objects.  The two object types are Buffers and Sources.  When
zero objects are requested, the call should be the equivalent of a NOP.
*/
ALvoid
FA_RequestObjectNames (ALvoid)
{
  ALboolean localResultOK;
  ALuint testSources[4], testBuffers[4];
  ALint error;

  printf ("\nRequest Object Names Test. ");
  alGetError ();                /* clear error state */
  localResultOK = AL_TRUE;
  alGenBuffers (0, testBuffers);        /* should be equivalent to NOP */
  error = alGetError ();
  if (error != AL_NO_ERROR)
    {
      localResultOK = AL_FALSE;
    }
  alGenSources (0, testSources);        /* should be equivalent to NOP */
  error = alGetError ();
  if (error != AL_NO_ERROR)
    {
      localResultOK = AL_FALSE;
    }
  if (localResultOK == AL_TRUE)
    {
      printf ("PASSED.");
    }
  else
    {
      printf ("FAILED.");
    }
}

/** used by gendocs.py
$SECTION Fully Automatic Tests
$SUBTITLE Release Object Names
This test checks that OpenAL responds correctly to the deletion of -1 objects.
The two object types are Buffers and Sources.  When -1 objects are requested, an
AL_INVALID_VALUE error should be generated.
*/
ALvoid
FA_ReleaseObjectNames (ALvoid)
{
  ALboolean localResultOK;
  ALuint testSources[4], testBuffers[4];
  ALint error;

  printf ("\nReleasing Object Names Test. ");
  alGetError ();
  localResultOK = AL_TRUE;
  alDeleteBuffers ((ALuint) -1, testBuffers);   /* invalid -- make sure error code comes back */
  error = alGetError ();
  if (error == AL_NO_ERROR)
    {
      localResultOK = AL_FALSE;
    }
  alDeleteSources ((ALuint) -1, testSources);   /* invalid -- make sure error code comes back */
  error = alGetError ();
  if (error == AL_NO_ERROR)
    {
      localResultOK = AL_FALSE;
    }
  if (localResultOK == AL_TRUE)
    {
      printf ("PASSED.");
    }
  else
    {
      printf ("FAILED.");
    }
}

/** used by gendocs.py
$SECTION Fully Automatic Tests
$SUBTITLE Validating Object Names
This test checks that OpenAL can test the validity of a source or buffer.  A
check is made on valid objects for a positive result, and a check is made on
invalid objects to confirm a negative result.
*/
ALvoid
FA_ValidateObjectNames (ALvoid)
{
  ALboolean localResultOK;
  ALuint testSources[4], testBuffers[4];
  ALint error;

  printf ("\nValidating Object Names Test. ");
  alGetError ();
  localResultOK = AL_TRUE;
  alGenBuffers (1, testBuffers);
  alGenSources (1, testSources);
  error = alGetError ();
  if (error != AL_NO_ERROR)
    {
      localResultOK = AL_FALSE;
    }
  else
    {
      if (alIsBuffer (testBuffers[0]) == AL_FALSE)      /* this buffer should test as valid */
        {
          localResultOK = AL_FALSE;
        }
      if (alIsSource (testSources[0]) == AL_FALSE)      /* this source should test as valid */
        {
          localResultOK = AL_FALSE;
        }
      if (alIsBuffer (testBuffers[0] + 1) == AL_TRUE)   /* this buffer should be invalid */
        {
          localResultOK = AL_FALSE;
        }
      if (alIsSource (testSources[0] + 1) == AL_TRUE)   /* this source should be invalid */
        {
          localResultOK = AL_FALSE;
        }
      alDeleteBuffers (1, testBuffers);
      alDeleteSources (1, testSources);
    }
  if (localResultOK == AL_TRUE)
    {
      printf ("PASSED.");
    }
  else
    {
      printf ("FAILED.");
    }
}

/** used by gendocs.py
$SECTION Fully Automatic Tests
$SUBTITLE State Transition Testing
This test checks that OpenAL can monitor the state of a source properly.  The
source is tested to make sure it can run through all its possible states --
AL_INITIAL, AL_PLAYING, AL_PAUSED, and AL_STOPPED.
*/
ALvoid
FA_StateTransition (ALvoid)
{
  ALboolean localResultOK;
  ALuint testSources[4];
  ALint sourceState;

  printf ("\nState Transition Test. ");
  alGetError ();
  localResultOK = AL_TRUE;
  alGenSources (1, testSources);
  alSourcei (testSources[0], AL_BUFFER, g_Buffers[0]);
  alSourcei (testSources[0], AL_LOOPING, AL_TRUE);
  alGetSourcei (testSources[0], AL_SOURCE_STATE, &sourceState);
  if (sourceState != AL_INITIAL)
    {
      localResultOK = AL_FALSE;
    }
  alSourcePlay (testSources[0]);
  sleepSeconds (0.5f);
  alGetSourcei (testSources[0], AL_SOURCE_STATE, &sourceState);
  if (sourceState != AL_PLAYING)
    {
      localResultOK = AL_FALSE;
    }
  alSourcePause (testSources[0]);
  sleepSeconds (0.5f);
  alGetSourcei (testSources[0], AL_SOURCE_STATE, &sourceState);
  if (sourceState != AL_PAUSED)
    {
      localResultOK = AL_FALSE;
    }
  alSourcePlay (testSources[0]);
  sleepSeconds (0.5f);
  alGetSourcei (testSources[0], AL_SOURCE_STATE, &sourceState);
  if (sourceState != AL_PLAYING)
    {
      localResultOK = AL_FALSE;
    }
  alSourceStop (testSources[0]);
  sleepSeconds (0.5f);
  alGetSourcei (testSources[0], AL_SOURCE_STATE, &sourceState);
  if (sourceState != AL_STOPPED)
    {
      localResultOK = AL_FALSE;
    }
  if (localResultOK == AL_TRUE)
    {
      printf ("PASSED.");
    }
  else
    {
      printf ("FAILED.");
    }
  alDeleteSources (1, testSources);
}

/** used by gendocs.py
$SECTION Fully Automatic Tests
$SUBTITLE Vector State Transition Testing
This test checks that OpenAL can monitor the state of multiple sources which
are being controlled using vectors.  The sources are tested to make sure
they properly run through all its possible states -- AL_INITIAL, AL_PLAYING,
AL_PAUSED, and AL_STOPPED.
*/
ALvoid
FA_VectorStateTransition (ALvoid)
{
  ALboolean localResultOK;
  ALuint testSources[4];
  ALenum sourceState;

  printf ("\nVector State Transition Test. ");
  alGetError ();
  localResultOK = AL_TRUE;
  alGenSources (2, testSources);
  alSourcei (testSources[0], AL_BUFFER, g_Buffers[0]);
  alSourcei (testSources[1], AL_BUFFER, g_Buffers[1]);
  alSourcei (testSources[0], AL_LOOPING, AL_TRUE);
  alSourcei (testSources[1], AL_LOOPING, AL_TRUE);
  alGetSourcei (testSources[0], AL_SOURCE_STATE, &sourceState);
  if (sourceState != AL_INITIAL)
    {
      localResultOK = AL_FALSE;
      printf ("FAILED -- AL_INITIAL 0");
    }
  alGetSourcei (testSources[1], AL_SOURCE_STATE, &sourceState);
  if (sourceState != AL_INITIAL)
    {
      localResultOK = AL_FALSE;
      printf ("FAILED -- AL_INITIAL 1");
    }
  alSourcePlayv (2, &testSources[0]);
  sleepSeconds (0.5f);
  alGetSourcei (testSources[0], AL_SOURCE_STATE, &sourceState);
  if (sourceState != AL_PLAYING)
    {
      localResultOK = AL_FALSE;
      printf ("FAILED -- AL_PLAYING 0");
    }
  alGetSourcei (testSources[1], AL_SOURCE_STATE, &sourceState);
  if (sourceState != AL_PLAYING)
    {
      localResultOK = AL_FALSE;
      printf ("FAILED -- AL_PLAYING 1");
    }
  alSourcePausev (2, &testSources[0]);
  sleepSeconds (0.5f);
  alGetSourcei (testSources[0], AL_SOURCE_STATE, &sourceState);
  if (sourceState != AL_PAUSED)
    {
      localResultOK = AL_FALSE;
      printf ("FAILED -- AL_PAUSED 0");
    }
  alGetSourcei (testSources[1], AL_SOURCE_STATE, &sourceState);
  if (sourceState != AL_PAUSED)
    {
      localResultOK = AL_FALSE;
      printf ("FAILED -- AL_PAUSED 1");
    }
  alSourcePlayv (2, &testSources[0]);
  sleepSeconds (0.5f);
  alGetSourcei (testSources[0], AL_SOURCE_STATE, &sourceState);
  if (sourceState != AL_PLAYING)
    {
      localResultOK = AL_FALSE;
      printf ("FAILED -- AL_PLAYING 0A");
    }
  alGetSourcei (testSources[1], AL_SOURCE_STATE, &sourceState);
  if (sourceState != AL_PLAYING)
    {
      localResultOK = AL_FALSE;
      printf ("FAILED -- AL_PLAYING 1A");
    }
  alSourceStopv (2, &testSources[0]);
  sleepSeconds (0.5f);
  alGetSourcei (testSources[0], AL_SOURCE_STATE, &sourceState);
  if (sourceState != AL_STOPPED)
    {
      localResultOK = AL_FALSE;
      printf ("FAILED -- AL_STOPPED 0");
    }
  alGetSourcei (testSources[1], AL_SOURCE_STATE, &sourceState);
  if (sourceState != AL_STOPPED)
    {
      localResultOK = AL_FALSE;
      printf ("FAILED -- AL_STOPPED 1");
    }
  if (localResultOK == AL_TRUE)
    {
      printf ("PASSED.");
    }
  else
    {
      printf ("FAILED.");
    }
  alDeleteSources (2, testSources);
}

/** used by gendocs.py
$SECTION Fully Automatic Tests
$SUBTITLE Get Buffer Properties Test
This test checks that OpenAL can retrieve buffer properties properly.
*/
ALvoid
FA_GetBufferProperties (ALvoid)
{
  ALint freq, size;
  ALboolean passNULL;

  printf ("\nGet Buffer Properties Test. ");
  alGetBufferi (g_Buffers[0], AL_FREQUENCY, &freq);
  alGetBufferi (g_Buffers[0], AL_SIZE, &size);

  passNULL = alIsBuffer (0);    /* the NULL buffer should cause alIsBuffer to be TRUE (non-annotated 1.0 spec, pg 26) */

  if ((freq == 44100) && (size == 282626) && (passNULL == AL_TRUE))
    {
      printf ("PASSED.");
    }
  else
    {
      printf ("FAILED.");
    }
}

/** used by gendocs.py
$SECTION Fully Automatic Tests
$SUBTITLE Enumeration Value Test
This test checks that the implementation's enumeration values are correct.
*/
ALvoid
FA_EnumerationValue (ALvoid)
{
  int i;
  ALboolean result = AL_TRUE;
  ALCcontext *context;
  ALCdevice *device;
  ALCenum actualValueALC;
  ALenum actualValueAL;

  printf ("\nEnumeration Value Test. ");

  context = alcGetCurrentContext ();
  device = alcGetContextsDevice (context);

  /* Check ALC enums. */
  for (i = 0; enumerationALC[i].enumName; i++)
    {
      const ALCchar *enumName = enumerationALC[i].enumName;
      ALCenum expectedValue = enumerationALC[i].expectedValue;

      /* Check if the ALC enum can be retrieved at all. */
      alcGetError (device);
      actualValueALC = alcGetEnumValue (device, enumName);
      if (alcGetError (device) != ALC_NO_ERROR)
        {
          printf ("\ngetting %s vial alcGetEnumValue failed",
                  (const char *) enumName);
          result = AL_FALSE;
        }

      /* Check if the ALC enum has the expected value. */
      if (expectedValue != actualValueALC)
        {
          printf ("\n%s has an invalid enum value: expected %d, got %d",
                  (const char *) enumName, (int) expectedValue,
                  (int) actualValueALC);
          result = AL_FALSE;
        }

      /* Check that the ALC enum can not be retrieved via alGetEnumValue. Note
         that the OpenAL 1.1 spec is a bit strange here: For alGetEnumValue it
         doesn't specify an error code, but for a similar call of
         alcGetEnumValue it does. Hmmm... */
      alGetError ();
      actualValueAL = alGetEnumValue (enumName);
      if (actualValueAL != 0)
        {
          printf
            ("\n%s could be retrieved vial alGetEnumValue (dubious behaviour)",
             (const char *) enumName);
          result = AL_FALSE;
        }
      alGetError ();
    }

  /* Check AL enums */
  for (i = 0; enumerationAL[i].enumName; i++)
    {
      const ALchar *enumName = enumerationAL[i].enumName;
      ALenum expectedValue = enumerationAL[i].expectedValue;

      /* Check if the AL enum can be retrieved at all. */
      alGetError ();
      actualValueAL = alGetEnumValue (enumName);
      if (alGetError () != AL_NO_ERROR)
        {
          printf ("\ngetting %s vial alGetEnumValue failed",
                  (const char *) enumName);
          result = AL_FALSE;
        }

      /* Check if the AL enum has the expected value. */
      if (expectedValue != actualValueAL)
        {
          printf ("\n%s has an invalid enum value: expected %d, got %d",
                  (const char *) enumName, (int) expectedValue,
                  (int) actualValueAL);
          result = AL_FALSE;
        }

      /* Check that retrieving an AL enum via alcGetEnumValue yields an
         error. */
      alcGetError (device);
      actualValueALC = alcGetEnumValue (device, enumName);
      if (alcGetError (device) != ALC_INVALID_VALUE)
        {
          printf
            ("\nwhen retrieving %s via alcGetEnumValue, no ALC_INVALID_VALUE error was reported (dubious behaviour)",
             (const char *) enumName);
          result = AL_FALSE;
        }

      /* Check that retrieving an AL enum via alcGetEnumValue returns 0. */
      if (actualValueALC != 0)
        {
          printf
            ("\nwhen retrieving %s via alcGetEnumValue: expected 0, got %d (dubious behaviour)",
             (const char *) enumName, (int) actualValueALC);
          result = AL_FALSE;
        }
    }

  printf (result == AL_TRUE ? "PASSED." : "FAILED.");
}

/** used by gendocs.py
$SECTION Fully Automatic Tests
$SUBTITLE Queuing Underrun States
This test checks that OpenAL handles state progressions properly during a streaming underrun.
*/
ALvoid
FA_QueuingUnderrunStates (ALvoid)
{
  ALuint testSources[1];
  ALuint bufferName;
  ALuint error;
  ALint tempInt;
  ALboolean localResultOK;

  printf ("\nQueuing Underrun States Test. ");
  localResultOK = AL_TRUE;
  alGetError ();
  alGenSources (1, testSources);
  alSourcei (testSources[0], AL_BUFFER, 0);
  alSourcei (testSources[0], AL_LOOPING, AL_FALSE);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "Init error : ", error);
  alSourceQueueBuffers (testSources[0], 1, &g_Buffers[1]);
  if ((error = alGetError ()) != AL_NO_ERROR)
    localResultOK = AL_FALSE;
  alSourcePlay (testSources[0]);
  sleepSeconds (1.0f);
  alGetSourcei (testSources[0], AL_SOURCE_STATE, &tempInt);
  if (tempInt != AL_STOPPED)
    localResultOK = AL_FALSE;
  alGetSourcei (testSources[0], AL_BUFFERS_PROCESSED, &tempInt);
  if (tempInt != 1)
    {
      localResultOK = AL_FALSE;
    }
  else
    {
      alSourceUnqueueBuffers (testSources[0], tempInt, &bufferName);
    }
  alSourceQueueBuffers (testSources[0], 1, &g_Buffers[1]);
  if ((error = alGetError ()) != AL_NO_ERROR)
    localResultOK = AL_FALSE;
  alSourcePlay (testSources[0]);
  sleepSeconds (0.1f);
  alGetSourcei (testSources[0], AL_SOURCE_STATE, &tempInt);
  if (tempInt != AL_PLAYING)
    localResultOK = AL_FALSE;

  /* cleanup */
  alSourcei (testSources[0], AL_BUFFER, 0);
  alDeleteSources (1, testSources);

  /* display result */
  if (localResultOK == AL_TRUE)
    {
      printf ("PASSED.");
    }
  else
    {
      printf ("FAILED.");
    }
}

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE String Queries Test
This test outputs the renderer, version #, vendor, and extensions string so that
the user can confirm that the string values are correct.
*/
ALvoid
SA_StringQueries (ALvoid)
{
  printf ("String Queries Test:");
  if (ContinueOrSkip ())
    {
      const ALchar *tempString;
      ALCcontext *pContext;
      ALCdevice *pDevice;
      printf ("Check that the following values are reasonable:\n");
      pContext = alcGetCurrentContext ();
      pDevice = alcGetContextsDevice (pContext);
      tempString = alcGetString (pDevice, ALC_DEVICE_SPECIFIER);
      printf ("OpenAL Context Device Specifier is '%s'\n", tempString);
      tempString = alGetString (AL_RENDERER);
      printf ("OpenAL Renderer is '%s'\n", tempString);
      tempString = alGetString (AL_VERSION);
      printf ("OpenAL Version is '%s'\n", tempString);
      tempString = alGetString (AL_VENDOR);
      printf ("OpenAL Vendor is '%s'\n", tempString);
      tempString = alGetString (AL_EXTENSIONS);
      printf ("OpenAL Extensions supported are :\n%s\n", tempString);
      printf ("\nError Codes are :-\n");
      printf ("AL_NO_ERROR : '%s'\n", tempString = alGetString (AL_NO_ERROR));
      printf ("AL_INVALID_NAME : '%s'\n", tempString =
              alGetString (AL_INVALID_NAME));
      printf ("AL_INVALID_ENUM : '%s'\n", tempString =
              alGetString (AL_INVALID_ENUM));
      printf ("AL_INVALID_VALUE : '%s'\n", tempString =
              alGetString (AL_INVALID_VALUE));
      printf ("AL_INVALID_OPERATION : '%s'\n", tempString =
              alGetString (AL_INVALID_OPERATION));
      printf ("AL_OUT_OF_MEMORY : '%s'\n", tempString =
              alGetString (AL_OUT_OF_MEMORY));
      CRForNextTest ();
    }
}

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Source Gain Test
This test outputs a source at multiple gain values for testing by the user.
*/
ALvoid
SA_SourceGain (ALvoid)
{
  ALuint testSources[2];

  printf ("Source Gain Test:");
  if (ContinueOrSkip ())
    {
      /* load up sources */
      alGenSources (1, testSources);
      alSourcei (testSources[0], AL_BUFFER, g_Buffers[1]);

      printf
        ("The following sound effect will be played at full source gain (Press Return):\n");
      CRToContinue ();
      alSourcef (testSources[0], AL_GAIN, 1.0f);
      alSourcePlay (testSources[0]);
      printf
        ("The following sound effect will be played at half source gain (Press Return):\n");
      CRToContinue ();
      alSourcef (testSources[0], AL_GAIN, 0.5f);
      alSourcePlay (testSources[0]);
      printf
        ("The following sound effect will be played at quarter source gain (Press Return):\n");
      CRToContinue ();
      alSourcef (testSources[0], AL_GAIN, 0.25f);
      alSourcePlay (testSources[0]);
      printf
        ("The following sound effect will be played at 1/20th source gain (Press Return):\n");
      CRToContinue ();
      alSourcef (testSources[0], AL_GAIN, 0.05f);
      alSourcePlay (testSources[0]);
      CRForNextTest ();
      alSourcef (testSources[0], AL_GAIN, 1.0f);

      /* dispose of sources */
      alSourcei (testSources[0], AL_BUFFER, 0);
      alDeleteSources (1, testSources);
    }
}

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Listener Gain Test
This test outputs a source at a fixed gain level, and tests various listener gain levels.
*/
ALvoid
SA_ListenerGain (ALvoid)
{
  ALuint testSources[2];
  ALfloat f;

  printf ("Listener Gain Test:");
  if (ContinueOrSkip ())
    {
      /* load up sources */
      alGenSources (1, testSources);
      alSourcei (testSources[0], AL_BUFFER, g_Buffers[1]);

      printf
        ("The following sound effect will be played at full listener gain (Press Return):\n");
      CRToContinue ();
      alListenerf (AL_GAIN, 1.0f);
      alSourcePlay (testSources[0]);
      printf
        ("The following sound effect will be played at half listener gain (Press Return):\n");
      CRToContinue ();
      alListenerf (AL_GAIN, 0.5f);
      alSourcePlay (testSources[0]);
      printf
        ("The following sound effect will be played at quarter listener gain (Press Return):\n");
      CRToContinue ();
      alListenerf (AL_GAIN, 0.25f);
      alSourcePlay (testSources[0]);
      printf
        ("The following sound effect will be played at 1/20th listener gain (Press Return):\n");
      CRToContinue ();
      alListenerf (AL_GAIN, 0.05f);
      alSourcePlay (testSources[0]);
      CRForNextTest ();
      alListenerf (AL_GAIN, 1.0f);
      alGetListenerf (AL_GAIN, &f);
      if (f != 1.0)
        {
          printf ("ERROR:  alGetListenerf failed.\n");
        }
      alSourceStop (testSources[0]);

      /* dispose of sources */
      alSourcei (testSources[0], AL_BUFFER, 0);
      alDeleteSources (1, testSources);
    }
}

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Position Test
This tests various source/listener positions, as well as the AL_POSITION get functions.
*/
ALvoid
SA_Position (ALvoid)
{
  ALuint testSources[2];
  int i;
  ALfloat tempFVect[6];

  printf ("Position Test:");
  if (ContinueOrSkip ())
    {
      /* load up sources */
      alGenSources (1, testSources);
      alSourcei (testSources[0], AL_BUFFER, g_Buffers[1]);
      alSource3f (testSources[0], AL_POSITION, 0.0, 0.0, 0.0);

      printf
        ("Trying Left-to-Right sweep by moving listener(Press Return):\n");
      CRToContinue ();
      alSourcei (testSources[0], AL_LOOPING, AL_TRUE);
      alListener3f (AL_POSITION, 100.0, 0.0, 0.0);
      alGetListener3f (AL_POSITION, &tempFVect[0], &tempFVect[1],
                       &tempFVect[2]);
      if ((tempFVect[0] != 100.0) || (tempFVect[1] != 0.0)
          || (tempFVect[2] != 0.0))
        {
          printf ("ERROR: alGetListener3f(AL_POSITION, ...).\n");
        }
      alGetListenerfv (AL_POSITION, tempFVect);
      if ((tempFVect[0] != 100.0) || (tempFVect[1] != 0.0)
          || (tempFVect[2] != 0.0))
        {
          printf ("ERROR: alGetListenerfv(AL_POSITION, ...).\n");
        }
      alSourcePlay (testSources[0]);
      for (i = -100; i < 100; i++)
        {
          alListener3f (AL_POSITION, (float) -i, 0.0, 0.0);
          sleepSeconds (0.1f);
        }
      alListener3f (AL_POSITION, 0.0, 0.0, 0.0);
      alSourceStop (testSources[0]);
      printf
        ("Trying Left-to-Right sweep by moving source (Press Return):\n");
      CRToContinue ();
      alSourcei (testSources[0], AL_LOOPING, AL_TRUE);
      alSource3f (testSources[0], AL_POSITION, -100.0, 0.0, 0.0);
      alSourcePlay (testSources[0]);
      for (i = -100; i < 100; i++)
        {
          alSource3f (testSources[0], AL_POSITION, (float) i, 0.0, 0.0);
          sleepSeconds (0.1f);
        }
      alSource3f (testSources[0], AL_POSITION, 0.0, 0.0, 0.0);
      alSourceStop (testSources[0]);
      printf ("Trying Back-to-Front sweep (Press Return):\n");
      CRToContinue ();
      alSourcei (testSources[0], AL_LOOPING, AL_TRUE);
      alSource3f (testSources[0], AL_POSITION, 0.0, 0.0, -100.0);
      alGetSource3f (testSources[0], AL_POSITION, &tempFVect[0],
                     &tempFVect[1], &tempFVect[2]);
      if ((tempFVect[0] != 0.0) || (tempFVect[1] != 0.0)
          || (tempFVect[2] != -100.0))
        {
          printf ("ERROR: alGetSource3f(..., AL_POSITION, ...).\n");
        }
      alGetSourcefv (testSources[0], AL_POSITION, tempFVect);
      if ((tempFVect[0] != 0.0) || (tempFVect[1] != 0.0)
          || (tempFVect[2] != -100.0))
        {
          printf ("ERROR: alGetSourcefv(..., AL_POSITION, ...).\n");
        }
      alSourcePlay (testSources[0]);
      for (i = -100; i < 100; i++)
        {
          alSource3f (testSources[0], AL_POSITION, 0.0, 0.0, (float) -i);
          sleepSeconds (0.1f);
        }
      alSource3f (testSources[0], AL_POSITION, 0.0, 0.0, 0.0);
      alSourceStop (testSources[0]);
      CRForNextTest ();

      /* dispose of sources */
      alSourcei (testSources[0], AL_BUFFER, 0);
      alDeleteSources (1, testSources);
    }
}

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Source Relative Test
This tests the source relative mode.
*/
ALvoid
SA_SourceRelative (ALvoid)
{
  ALuint testSources[2];
  int i;
  ALfloat listenerOri[] = { 0.0, 0.0, -1.0, 0.0, 1.0, 0.0 };

  printf ("Source Relative Test:");
  if (ContinueOrSkip ())
    {
      /* load up sources */
      alGenSources (1, testSources);
      alSourcei (testSources[0], AL_BUFFER, g_Buffers[1]);

      printf
        ("Placing Listener at (100, 0, 0) and sweeping source from (0, 0, 0) to (100, 0, 0).  The sound should pan from left to center (Press Return):\n");
      CRToContinue ();
      alListener3f (AL_POSITION, 100.0, 0.0, 0.0);
      alListenerfv (AL_ORIENTATION, listenerOri);
      alSourcei (testSources[0], AL_LOOPING, AL_TRUE);
      alSource3f (testSources[0], AL_POSITION, -10.0, 0.0, 0.0);
      alSourcePlay (testSources[0]);
      for (i = 00; i < 100; i++)
        {
          alSource3f (testSources[0], AL_POSITION, (float) i, 0.0, 0.0);
          sleepSeconds (0.1f);
        }
      alSource3f (testSources[0], AL_POSITION, 0.0, 0.0, 0.0);
      alSourceStop (testSources[0]);

      printf
        ("Turning on source relative mode, placing Listener at (100, 0, 0), and sweeping source from (0, 0, 0) to (100, 0, 0).  The sound should pan from center to right (Press Return):\n");
      CRToContinue ();
      alSourcei (testSources[0], AL_SOURCE_RELATIVE, AL_TRUE);
      alSourcei (testSources[0], AL_LOOPING, AL_TRUE);
      alSource3f (testSources[0], AL_POSITION, -100.0, 0.0, 0.0);
      alSourcePlay (testSources[0]);
      for (i = 0; i < 100; i++)
        {
          alSource3f (testSources[0], AL_POSITION, (float) i, 0.0, 0.0);
          sleepSeconds (0.1f);
        }

      alListener3f (AL_POSITION, 0.0, 0.0, 0.0);
      alSourcei (testSources[0], AL_SOURCE_RELATIVE, AL_FALSE);
      alSource3f (testSources[0], AL_POSITION, 0.0, 0.0, 0.0);
      alSourceStop (testSources[0]);
      CRForNextTest ();

      /* dispose of sources */
      alSourcei (testSources[0], AL_BUFFER, 0);
      alDeleteSources (1, testSources);
    }
}

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Listener Orientation Test
This test moves and orients the listener around a fixed source.
*/
ALvoid
SA_ListenerOrientation (ALvoid)
{
  ALuint testSources[2];
  ALfloat listenerOri[] = { 0.0, 0.0, -1.0, 0.0, 1.0, 0.0 };
  ALfloat f;

  printf ("Listener Orientation Test:");
  if (ContinueOrSkip ())
    {
      /* load up sources */
      alGenSources (1, testSources);
      alSourcei (testSources[0], AL_BUFFER, g_Buffers[1]);

      printf
        ("The listener will be placed at (1, 0, 0) and will face the -X direction.  The sound should be centered. (Press Return):\n");
      CRToContinue ();
      alSource3f (testSources[0], AL_POSITION, 0.0, 0.0, 0.0);
      alListenerf (AL_GAIN, 1.0f);
      alGetSourcef (testSources[0], AL_GAIN, &f);
      if (f != 1.0)
        {
          printf ("ERROR: alGetSourcef(..., AL_GAIN, ...).\n");
        }
      alListener3f (AL_POSITION, 1.0, 0.0, 0.0);
      listenerOri[0] = -1.0;
      listenerOri[1] = 0.0;
      listenerOri[2] = 0.0;
      listenerOri[3] = 0.0;
      listenerOri[4] = 1.0;
      listenerOri[5] = 0.0;
      alListenerfv (AL_ORIENTATION, listenerOri);
      alSourcei (testSources[0], AL_LOOPING, AL_TRUE);
      alSourcePlay (testSources[0]);
      sleepSeconds (4.0f);
      alSourceStop (testSources[0]);

      printf
        ("The listener will now be oriented down the -Z axis.  The sound should be to the left. (Press Return):\n");
      CRToContinue ();
      listenerOri[0] = 0.0;
      listenerOri[1] = 0.0;
      listenerOri[2] = -1.0;
      listenerOri[3] = 0.0;
      listenerOri[4] = 1.0;
      listenerOri[5] = 0.0;
      alListenerfv (AL_ORIENTATION, listenerOri);
      alSourcePlay (testSources[0]);
      sleepSeconds (4.0f);
      alSourceStop (testSources[0]);

      printf
        ("The listener will now be turned upside-down (the 'up' direction will be (0, -1, 0)).  The sound should be to the right. (Press Return):\n");
      CRToContinue ();
      listenerOri[0] = 0.0;
      listenerOri[1] = 0.0;
      listenerOri[2] = -1.0;
      listenerOri[3] = 0.0;
      listenerOri[4] = -1.0;
      listenerOri[5] = 0.0;
      alListenerfv (AL_ORIENTATION, listenerOri);
      alSourcePlay (testSources[0]);
      sleepSeconds (4.0f);
      alSourceStop (testSources[0]);

      printf
        ("The listener will now be oriented down the +Z axis (and the 'up' direction is now (0, 1, 0) again).  The sound should be to the right. (Press Return):\n");
      CRToContinue ();
      listenerOri[0] = 0.0;
      listenerOri[1] = 0.0;
      listenerOri[2] = 1.0;
      listenerOri[3] = 0.0;
      listenerOri[4] = 1.0;
      listenerOri[5] = 0.0;
      alListenerfv (AL_ORIENTATION, listenerOri);
      alSourcePlay (testSources[0]);
      sleepSeconds (4.0f);
      alSourceStop (testSources[0]);

      printf
        ("The listener will now be oriented down the +Y axis and with a +Z 'up' vector.  The sound should be to the left. (Press Return):\n");
      CRToContinue ();
      listenerOri[0] = 0.0;
      listenerOri[1] = 1.0;
      listenerOri[2] = 0.0;
      listenerOri[3] = 0.0;
      listenerOri[4] = 0.0;
      listenerOri[5] = 1.0;
      alListenerfv (AL_ORIENTATION, listenerOri);
      alSourcePlay (testSources[0]);
      sleepSeconds (4.0f);
      alSourceStop (testSources[0]);

      printf
        ("The listener will now be oriented down the -Y axis and with a +Z 'up' vector.  The sound should be to the right. (Press Return):\n");
      CRToContinue ();
      listenerOri[0] = 0.0;
      listenerOri[1] = -1.0;
      listenerOri[2] = 0.0;
      listenerOri[3] = 0.0;
      listenerOri[4] = 0.0;
      listenerOri[5] = 1.0;
      alListenerfv (AL_ORIENTATION, listenerOri);
      alSourcePlay (testSources[0]);
      sleepSeconds (4.0f);
      alSourceStop (testSources[0]);

      CRForNextTest ();
      alListenerf (AL_GAIN, 1.0f);
      alListener3f (AL_POSITION, 0.0, 0.0, 0.0);
      listenerOri[0] = 0.0;
      listenerOri[1] = 0.0;
      listenerOri[2] = -1.0;
      listenerOri[3] = 0.0;
      listenerOri[4] = 1.0;
      listenerOri[5] = 0.0;
      alListenerfv (AL_ORIENTATION, listenerOri);

      /* dispose of sources */
      alSourcei (testSources[0], AL_BUFFER, 0);
      alDeleteSources (1, testSources);
    }
}

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Source Cone Test
This test exercises source cones.
*/
ALvoid
SA_SourceCone (ALvoid)
{
  ALuint testSources[2];
  ALfloat listenerOri[] = { 0.0, 0.0, -1.0, 0.0, 1.0, 0.0 };
  ALfloat sourceDir[] = { 0.0, 0.0, 1.0 };
  ALfloat sourceDir2[] = { 1.0, 0.0, 1.0 };
  ALfloat sourceDir3[] = { 0.0, 0.0, -1.0 };

  printf ("Source Cone Test:");
  if (ContinueOrSkip ())
    {
      /* load up sources */
      alGenSources (1, testSources);
      alSourcei (testSources[0], AL_BUFFER, g_Buffers[1]);

      printf
        ("The listener will be at (0,0,0).  The source will be at (0,0,-1).  The source will be directly facing the listener and should be loud. (Press Return):\n");
      CRToContinue ();
      alListener3f (AL_POSITION, 0.0, 0.0, 0.0);
      alListenerfv (AL_ORIENTATION, listenerOri);
      alSourcef (testSources[0], AL_CONE_INNER_ANGLE, 10.0);
      alSourcef (testSources[0], AL_CONE_OUTER_ANGLE, 270.0);
      alSourcef (testSources[0], AL_CONE_OUTER_GAIN, (float) 0.01);
      alSource3f (testSources[0], AL_POSITION, 0.0, 0.0, -1.0);
      alSourcefv (testSources[0], AL_DIRECTION, sourceDir);
      alSourcePlay (testSources[0]);

      printf
        ("The source will now point between the inner and outer cones, and should be at medium volume. (Press Return):\n");
      CRToContinue ();
      alSourcefv (testSources[0], AL_DIRECTION, sourceDir2);
      alSourcePlay (testSources[0]);

      printf
        ("The source will now point behind the outer cone and will be at low volume. (Press Return):\n");
      CRToContinue ();
      alSourcefv (testSources[0], AL_DIRECTION, sourceDir3);
      alSourcePlay (testSources[0]);

      CRForNextTest ();

      /* dispose of sources */
      alSourcei (testSources[0], AL_BUFFER, 0);
      alDeleteSources (1, testSources);
    }
}

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE MIN/MAX Gain Test
This test checks if minimum and maximum gain settings are working.
*/
ALvoid
SA_MinMaxGain (ALvoid)
{
  ALuint testSources[2];

  printf ("MIN/MAX Gain Test:");
  if (ContinueOrSkip ())
    {
      /* load up sources */
      alGenSources (1, testSources);
      alSourcei (testSources[0], AL_BUFFER, g_Buffers[1]);

      printf
        ("The source will be played at GAIN 1.0 with MAX gain set to 1.0. This should be high volume. (Press Return):\n");
      CRToContinue ();
      alSourcef (testSources[0], AL_GAIN, 1.0);
      alSourcef (testSources[0], AL_MAX_GAIN, 1.0);
      alSourcei (testSources[0], AL_LOOPING, AL_FALSE);
      alSourcePlay (testSources[0]);

      printf
        ("The source will be played at GAIN 0.1 with MIN gain set to 0.6.  This should be at medium volume. (Press Return):\n");
      CRToContinue ();
      alSourcef (testSources[0], AL_GAIN, (float) 0.1);
      alSourcef (testSources[0], AL_MIN_GAIN, (float) 0.6);
      alSourcePlay (testSources[0]);

      printf
        ("The source will be played at GAIN 1.0 with MAX gain set to 0.1.  This should be low volume. (Press Return):\n");
      CRToContinue ();
      alSourcef (testSources[0], AL_GAIN, 1.0);
      alSourcef (testSources[0], AL_MAX_GAIN, (float) 0.1);
      alSourcePlay (testSources[0]);

      printf
        ("The source will be played at GAIN 0.1 with MIN gain set to 0.0.  This should be low volume. (Press Return):\n");
      CRToContinue ();
      alSourcef (testSources[0], AL_GAIN, (float) 0.1);
      alSourcef (testSources[0], AL_MIN_GAIN, 0.0);
      alSourcePlay (testSources[0]);

      printf
        ("The source will be played at GAIN 1.0 with MAX gain set to 0.0.  This should be zero volume. (Press Return):\n");
      CRToContinue ();
      alSourcef (testSources[0], AL_GAIN, (float) 1.0);
      alSourcef (testSources[0], AL_MAX_GAIN, 0.0);
      alSourcePlay (testSources[0]);

      CRForNextTest ();
      alSourcef (testSources[0], AL_GAIN, 1.0);
      alSourcef (testSources[0], AL_MAX_GAIN, 1.0);
      alSourcef (testSources[0], AL_MIN_GAIN, 0.0);

      /* dispose of sources */
      alSourcei (testSources[0], AL_BUFFER, 0);
      alDeleteSources (1, testSources);
    }
}

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Reference Distance Test
This test exercises a source's reference distance.
*/
ALvoid
SA_ReferenceDistance (ALvoid)
{
  ALuint testSources[2];

  printf ("Reference Distance Test:");
  if (ContinueOrSkip ())
    {
      /* load up sources */
      alGenSources (1, testSources);
      alSourcei (testSources[0], AL_BUFFER, g_Buffers[1]);

      printf
        ("The source will be placed at (0, 0, -10), and the reference distance set at 1.0. This should be low volume. (Press Return):\n");
      CRToContinue ();
      alSource3f (testSources[0], AL_POSITION, 0.0, 0.0, -10.0);
      alSourcef (testSources[0], AL_REFERENCE_DISTANCE, 1.0);
      alSourcei (testSources[0], AL_LOOPING, AL_FALSE);
      alSourcePlay (testSources[0]);

      printf
        ("The source will be played with the reference distance set to 3.0.  This should be medium volume. (Press Return):\n");
      CRToContinue ();
      alSourcef (testSources[0], AL_REFERENCE_DISTANCE, 3.0);
      alSourcePlay (testSources[0]);

      printf
        ("The source will be played with the reference distance set to 10.0.  This should be high volume. (Press Return):\n");
      CRToContinue ();
      alSourcef (testSources[0], AL_REFERENCE_DISTANCE, 10.0);
      alSourcePlay (testSources[0]);

      CRForNextTest ();
      alSource3f (testSources[0], AL_POSITION, 0.0, 0.0, 0.0);
      alSourcef (testSources[0], AL_REFERENCE_DISTANCE, 1.0);

      /* dispose of sources */
      alSourcei (testSources[0], AL_BUFFER, 0);
      alDeleteSources (1, testSources);
    }
}

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Rolloff Factor Test
This test exercises a source's rolloff factor.
*/
ALvoid
SA_RolloffFactor (ALvoid)
{
  ALuint testSources[2];

  printf ("Rolloff Factor Test:");
  if (ContinueOrSkip ())
    {
      /* load up sources */
      alGenSources (1, testSources);
      alSourcei (testSources[0], AL_BUFFER, g_Buffers[1]);

      printf
        ("The source will be played with the rolloff factor set to 0.0.  This should be high volume. (Press Return):\n");
      CRToContinue ();
      alSource3f (testSources[0], AL_POSITION, 0.0, 0.0, -10.0);
      alSourcei (testSources[0], AL_LOOPING, AL_FALSE);
      alSourcef (testSources[0], AL_ROLLOFF_FACTOR, 0.0);
      alSourcePlay (testSources[0]);

      printf
        ("The source will be placed at (0, 0, -10), and the rolloff factor set at 1.0. This should be medium volume. (Press Return):\n");
      CRToContinue ();
      alSourcef (testSources[0], AL_ROLLOFF_FACTOR, 1.0);
      alSourcePlay (testSources[0]);

      printf
        ("The source will be played with the rolloff factor set to 3.0.  This should be low volume. (Press Return):\n");
      CRToContinue ();
      alSourcef (testSources[0], AL_ROLLOFF_FACTOR, 3.0);
      alSourcePlay (testSources[0]);

      printf
        ("The source will be played with the rolloff factor set to 10.0.  This should be very low volume. (Press Return):\n");
      CRToContinue ();
      alSourcef (testSources[0], AL_ROLLOFF_FACTOR, 10.0);
      alSourcePlay (testSources[0]);

      CRForNextTest ();

      /* dispose of sources */
      alSourcei (testSources[0], AL_BUFFER, 0);
      alDeleteSources (1, testSources);
    }
}

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Distance Model Test
This test exercises the three distance models.
*/
ALvoid
SA_DistanceModel (ALvoid)
{
  ALuint testSources[2];

  printf ("Distance Model Test:");
  if (ContinueOrSkip ())
    {
      /* load up sources */
      alGenSources (1, testSources);
      alSourcei (testSources[0], AL_BUFFER, g_Buffers[1]);

      printf
        ("The source will be placed at (0, 0, -10). This should be low volume. (Press Return):\n");
      CRToContinue ();
      alSource3f (testSources[0], AL_POSITION, 0.0, 0.0, -10.0);
      alDistanceModel (AL_INVERSE_DISTANCE);
      alSourcei (testSources[0], AL_LOOPING, AL_FALSE);
      alSourcePlay (testSources[0]);

      printf
        ("The source will be placed at (0, 0, -1). This should be high volume. (Press Return):\n");
      CRToContinue ();
      alSource3f (testSources[0], AL_POSITION, 0.0, 0.0, -1.0);
      alSourcePlay (testSources[0]);

      printf
        ("The source will be placed at (0, 0, -10) and the distance model will be set to AL_NONE. This should be high volume. (Press Return):\n");
      CRToContinue ();
      alSource3f (testSources[0], AL_POSITION, 0.0, 0.0, -10.0);
      alDistanceModel (AL_NONE);
      alSourcePlay (testSources[0]);

      printf
        ("The source will be placed at (0, 0, -100) and the distance model will remain AL_NONE. This should be high volume. (Press Return):\n");
      CRToContinue ();
      alSource3f (testSources[0], AL_POSITION, 0.0, 0.0, -100.0);
      alSourcePlay (testSources[0]);

      printf
        ("The source will be placed at (0, 0, -100) and the distance model will be AL_INVERSE_DISTANCE_CLAMPED. AL_MAX_DISTANCE will be set to 100.0.  This should be low volume. (Press Return):\n");
      CRToContinue ();
      alSource3f (testSources[0], AL_POSITION, 0.0, 0.0, -100.0);
      alDistanceModel (AL_INVERSE_DISTANCE_CLAMPED);
      alSourcef (testSources[0], AL_MAX_DISTANCE, 100.0);
      alSourcePlay (testSources[0]);

      printf
        ("The source will be placed at (0, 0, -100) and the distance model will be AL_INVERSE_DISTANCE_CLAMPED. AL_MAX_DISTANCE will be set to 20.0.  This should be louder. (Press Return):\n");
      CRToContinue ();
      alSource3f (testSources[0], AL_POSITION, 0.0, 0.0, -100.0);
      alSourcef (testSources[0], AL_MAX_DISTANCE, 20.0);
      alSourcePlay (testSources[0]);

      printf
        ("The source will be placed at (0, 0, -100) and the distance model will be AL_INVERSE_DISTANCE_CLAMPED. AL_MAX_DISTANCE will be set to 5.0.  This should be high volume. (Press Return):\n");
      CRToContinue ();
      alSource3f (testSources[0], AL_POSITION, 0.0, 0.0, -100.0);
      alSourcef (testSources[0], AL_MAX_DISTANCE, 5.0);
      alSourcePlay (testSources[0]);

      CRForNextTest ();
      alSource3f (testSources[0], AL_POSITION, 0.0, 0.0, 0.0);
      alDistanceModel (AL_INVERSE_DISTANCE);

      /* dispose of sources */
      alSourcei (testSources[0], AL_BUFFER, 0);
      alDeleteSources (1, testSources);
    }
}

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Doppler Test
This tests doppler shift capability.
*/
ALvoid
SA_Doppler (ALvoid)
{
  ALuint testSources[2];
  int i;
  ALfloat listenerOri[] = { 0.0, 0.0, -1.0, 0.0, 1.0, 0.0 };
  ALfloat sourceDir[] = { 1.0, 0.0, 0.0 };

  printf ("Doppler Test:");
  if (ContinueOrSkip ())
    {
      /* load up sources */
      alGenSources (1, testSources);
      alSourcei (testSources[0], AL_BUFFER, g_Buffers[1]);

      printf
        ("Trying Left-to-Right sweep with doppler shift, and the speed of sound at its default setting (Press Return):\n");
      CRToContinue ();
      alListenerfv (AL_ORIENTATION, listenerOri);
      alSourcei (testSources[0], AL_LOOPING, AL_TRUE);
      alSource3f (testSources[0], AL_POSITION, -100.0, 0.0, 0.0);
      alSource3f (testSources[0], AL_VELOCITY, 10.0, 0.0, 0.0);
      alSourcefv (testSources[0], AL_DIRECTION, sourceDir);
      alSourcePlay (testSources[0]);
      for (i = -100; i < 100; i++)
        {
          alSource3f (testSources[0], AL_POSITION, (float) i, 0.0, 0.0);
          sleepSeconds (0.1f);
        }
      alSourceStop (testSources[0]);
      printf
        ("Trying Left-to-Right sweep with the speed of sound set to 171.7 (Press Return):\n");
      CRToContinue ();
      alSource3f (testSources[0], AL_POSITION, -100.0, 0.0, 0.0);
      alSource3f (testSources[0], AL_VELOCITY, 10.0, 0.0, 0.0);
      alSpeedOfSound (171.7f);
      if (alGetFloat (AL_SPEED_OF_SOUND) != 171.7f)
        {
          printf (" alGetFloat(AL_SPEED_OF_SOUND) error.\n");
        }
      alSourcePlay (testSources[0]);
      for (i = -100; i < 100; i++)
        {
          alSource3f (testSources[0], AL_POSITION, (float) i, 0.0, 0.0);
          sleepSeconds (0.1f);
        }
      alSourceStop (testSources[0]);
      alDopplerFactor (1.0);
      printf
        ("Trying Left-to-Right sweep with the speed of sound set to 686.6 (Press Return):\n");
      CRToContinue ();
      alSource3f (testSources[0], AL_POSITION, -100.0, 0.0, 0.0);
      alSource3f (testSources[0], AL_VELOCITY, 10.0, 0.0, 0.0);
      alSpeedOfSound (686.6f);
      if (alGetFloat (AL_SPEED_OF_SOUND) != 686.6f)
        {
          printf (" alGetFloat(AL_SPEED_OF_SOUND) error.\n");
        }
      alSourcePlay (testSources[0]);
      for (i = -100; i < 100; i++)
        {
          alSource3f (testSources[0], AL_POSITION, (float) i, 0.0, 0.0);
          sleepSeconds (0.1f);
        }
      alSourceStop (testSources[0]);
      alDopplerFactor (2.0);
      printf
        ("Trying Left-to-Right sweep with the speed of sound set to 686.6 and doppler factor set to 2.0 (Press Return):\n");
      CRToContinue ();
      alSource3f (testSources[0], AL_POSITION, -100.0, 0.0, 0.0);
      alSource3f (testSources[0], AL_VELOCITY, 10.0, 0.0, 0.0);
      alSpeedOfSound (686.6f);
      alSourcePlay (testSources[0]);
      for (i = -100; i < 100; i++)
        {
          alSource3f (testSources[0], AL_POSITION, (float) i, 0.0, 0.0);
          sleepSeconds (0.1f);
        }
      alSpeedOfSound (343.3f);
      alDopplerFactor (1.0f);
      alSource3f (testSources[0], AL_POSITION, 0.0, 0.0, 0.0);
      alSourceStop (testSources[0]);
      CRForNextTest ();

      /* dispose of sources */
      alSourcei (testSources[0], AL_BUFFER, 0);
      alDeleteSources (1, testSources);
    }
}

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Frequency Test
This test alters the frequency of a playing source.
*/
ALvoid
SA_Frequency (ALvoid)
{
  ALuint testSources[2];
  float root12 = (float) (pow ((float) 2, (float) (1 / 12.0)));
  float increments[15] = { -12, -10, -8, -7, -5, -3, -1, 0,
    2, 4, 5, 7, 9, 11, 12
  };
  int i;

  printf ("Frequency Test:");
  if (ContinueOrSkip ())
    {
      /* load up sources */
      alGenSources (1, testSources);
      alSourcei (testSources[0], AL_BUFFER, g_Buffers[1]);

      printf
        ("A source will be played fifteen times -- going from one-half to double it's native frequency (Press Return):\n");
      CRToContinue ();
      alSourcei (testSources[0], AL_LOOPING, AL_FALSE);
      for (i = 0; i < 15; i++)
        {
          alSourcef (testSources[0], AL_PITCH,
                     (float) (pow (root12, increments[i])));
          alSourcePlay (testSources[0]);
          sleepSeconds (2.0f);
        }
      alSourceStop (testSources[0]);
      alSourcef (testSources[0], AL_PITCH, 1.0);
      CRForNextTest ();

      /* dispose of sources */
      alSourcei (testSources[0], AL_BUFFER, 0);
      alDeleteSources (1, testSources);
    }
}

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Stereo Test
This test plays a stereo buffer.
*/
ALvoid
SA_Stereo (ALvoid)
{
  ALuint testSources[2];
  ALuint error;

  printf ("Stereo Test:");
  if (ContinueOrSkip ())
    {
      /* clear error state */
      alGetError ();

      /* load up sources */
      alGenSources (1, testSources);
      alSourcei (testSources[0], AL_BUFFER, g_Buffers[1]);

      printf
        ("A stereo buffer will play twice in succession (Press Return):\n");
      CRToContinue ();
      alSourcei (testSources[0], AL_BUFFER, 0);
      alSourcei (testSources[0], AL_LOOPING, AL_FALSE);
      if ((error = alGetError ()) != AL_NO_ERROR)
        DisplayALError ((ALbyte *) "Init error : ", error);
      alSourceQueueBuffers (testSources[0], 1, &g_Buffers[6]);
      if ((error = alGetError ()) != AL_NO_ERROR)
        DisplayALError ((ALbyte *) "alSourceQueueBuffers 1 (stereo) : ",
                        error);
      alSourceQueueBuffers (testSources[0], 1, &g_Buffers[6]);
      if ((error = alGetError ()) != AL_NO_ERROR)
        DisplayALError ((ALbyte *) "alSourceQueueBuffers 1 (stereo) : ",
                        error);
      alSourcePlay (testSources[0]);
      CRForNextTest ();

      /* dispose of sources */
      alSourcei (testSources[0], AL_BUFFER, 0);
      alDeleteSources (1, testSources);
    }
}

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Streaming Test
This test streams audio from a file.
*/
ALvoid
SA_Streaming (ALvoid)
{
  printf ("Streaming Test:");
  if (ContinueOrSkip ())
    {
      printf
        ("A stereo audio file will now be streamed from a file (Press Return):\n");
      CRToContinue ();
      I_StreamingTest ();
      CRForNextTest ();
    }
}

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Queuing Underrun Performance
This test checks the performance of OpenAL during a buffer underrun.
*/
ALvoid
SA_QueuingUnderrunPerformance (ALvoid)
{
  ALuint testSources[1];
  ALuint bufferName;
  ALuint error;
  ALint tempInt;

  printf ("Queuing Underrun Performance Test:");
  if (ContinueOrSkip ())
    {
      printf
        ("A stereo buffer will play once, there will be a brief pause, and then the buffer will play again (Press Return):\n");
      CRToContinue ();
      alGetError ();
      alGenSources (1, testSources);
      alSourcei (testSources[0], AL_BUFFER, 0);
      alSourcei (testSources[0], AL_LOOPING, AL_FALSE);
      if ((error = alGetError ()) != AL_NO_ERROR)
        DisplayALError ((ALbyte *) "Init error : ", error);
      alSourceQueueBuffers (testSources[0], 1, &g_Buffers[6]);
      if ((error = alGetError ()) != AL_NO_ERROR)
        DisplayALError ((ALbyte *) "alSourceQueueBuffers 1 (stereo) : ",
                        error);
      alSourcePlay (testSources[0]);
      sleepSeconds (4.0f);
      alGetSourcei (testSources[0], AL_SOURCE_STATE, &tempInt);
      if (tempInt != AL_STOPPED)
        printf ("Wrong underrun state -- should be AL_STOPPED. ");
      alGetSourcei (testSources[0], AL_BUFFERS_PROCESSED, &tempInt);
      if (tempInt != 1)
        {
          printf
            ("Wrong underrun state -- should have one buffer processed. ");
        }
      else
        {
          alSourceUnqueueBuffers (testSources[0], tempInt, &bufferName);
        }
      alSourceQueueBuffers (testSources[0], 1, &g_Buffers[6]);
      if ((error = alGetError ()) != AL_NO_ERROR)
        DisplayALError ((ALbyte *) "alSourceQueueBuffers 1 (stereo) : ",
                        error);
      alSourcePlay (testSources[0]);
      sleepSeconds (3.0f);
      printf
        ("The stereo buffer will now play twice with no pause (Press Return):\n");
      CRToContinue ();
      alSourceQueueBuffers (testSources[0], 1, &g_Buffers[6]);
      alSourcePlay (testSources[0]);
      sleepSeconds (4.0f);
      CRForNextTest ();

      /* dispose of sources */
      alSourcei (testSources[0], AL_BUFFER, 0);
      alDeleteSources (1, testSources);
    }
}

/* Buffer Test */
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Buffer Test
This test allows the user to dynamically attach and unattach different buffers
to a single source.
*/
ALvoid
I_BufferTest (ALvoid)
{
  ALuint source[1];
  ALint error;
  char ch;

  ALfloat source0Pos[] = { 1.0, 0.0, -1.0 };    /* Front and right of the listener */
  ALfloat source0Vel[] = { 0.0, 0.0, 0.0 };

  alGenSources (1, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    {
      DisplayALError ((ALbyte *) "alGenSources 2 : ", error);
      return;
    }

  alSourcef (source[0], AL_PITCH, 1.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcef 0 AL_PITCH : \n", error);

  alSourcef (source[0], AL_GAIN, 1.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcef 0 AL_GAIN : \n", error);

  alSourcefv (source[0], AL_POSITION, source0Pos);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcefv 0 AL_POSITION : \n", error);

  alSourcefv (source[0], AL_VELOCITY, source0Vel);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcefv 0 AL_VELOCITY : \n", error);

  alSourcei (source[0], AL_LOOPING, AL_FALSE);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcei 0 AL_LOOPING AL_TRUE: \n", error);

  printf ("Buffer Test\n");
  printf ("Press '1' to play buffer 0 on source 0\n");
  printf ("Press '2' to play buffer 1 on source 0\n");
  printf ("Press '3' to stop source 0\n");
  printf ("Press 'q' to quit\n");

  do
    {
      ch = getUpperCh ();

      switch (ch)
        {
        case '1':
          /* Stop source */
          alSourceStop (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourceStop 0 : ", error);
          /* Attach buffer 0 to source */
          alSourcei (source[0], AL_BUFFER, g_Buffers[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcei AL_BUFFER 0 : ", error);
          /* Play */
          alSourcePlay (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcePlay 0 : ", error);
          break;
        case '2':
          /* Stop source */
          alSourceStop (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourceStop 0 : ", error);
          /* Attach buffer 0 to source */
          alSourcei (source[0], AL_BUFFER, g_Buffers[1]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcei AL_BUFFER 1 : ", error);
          /* Play */
          alSourcePlay (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcePlay 0 : ", error);
          break;
        case '3':
          /* Stop source */
          alSourceStop (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourceStop 0 : ", error);
          break;
        }
    }
  while (ch != 'Q');

  /* Release resources */
  alSourceStopv (1, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourceStopv 1 : ", error);

  alDeleteSources (1, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alDeleteSources 1 : ", error);
}

/* Position Test */
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Position Test
This test creates 2 Sources - one to the front right of the listener, and one to
the rear left of the listener
*/
ALvoid
I_PositionTest (ALvoid)
{
  ALint error;

  ALuint source[2];
  ALbyte ch;

  ALfloat source0Pos[] = { -1.0, 0.0, 1.0 };    /* Behind and to the left of the listener */
  ALfloat source0Vel[] = { 0.0, 0.0, 0.0 };

  ALfloat source1Pos[] = { 1.0, 0.0, -1.0 };    /* Front and right of the listener */
  ALfloat source1Vel[] = { 0.0, 0.0, 0.0 };

  alGenSources (2, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    {
      DisplayALError ((ALbyte *) "alGenSources 2 : ", error);
      return;
    }

  alSourcef (source[0], AL_PITCH, 1.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcef 0 AL_PITCH : \n", error);

  alSourcef (source[0], AL_GAIN, 1.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcef 0 AL_GAIN : \n", error);

  alSourcefv (source[0], AL_POSITION, source0Pos);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcefv 0 AL_POSITION : \n", error);

  alSourcefv (source[0], AL_VELOCITY, source0Vel);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcefv 0 AL_VELOCITY : \n", error);

  alSourcei (source[0], AL_BUFFER, g_Buffers[1]);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcei 0 AL_BUFFER buffer 0 : \n", error);

  alSourcei (source[0], AL_LOOPING, AL_TRUE);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcei 0 AL_LOOPING AL_TRUE: \n", error);

  alSourcef (source[1], AL_PITCH, 1.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcef 1 AL_PITCH : \n", error);

  alSourcef (source[1], AL_GAIN, 1.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcef 1 AL_GAIN : \n", error);

  alSourcefv (source[1], AL_POSITION, source1Pos);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcefv 1 AL_POSITION : \n", error);

  alSourcefv (source[1], AL_VELOCITY, source1Vel);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcefv 1 AL_VELOCITY : \n", error);

  alSourcei (source[1], AL_BUFFER, g_Buffers[1]);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcei 1 AL_BUFFER buffer 1 : \n", error);

  alSourcei (source[1], AL_LOOPING, AL_FALSE);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcei 1 AL_LOOPING AL_FALSE: \n", error);

  printf ("Position Test\n");
  printf ("Press '1' to play source 0 (looping) rear left of listener\n");
  printf
    ("Press '2' to play source 1 once (single shot) front right of listener\n");
  printf ("Press '3' to stop source 0\n");
  printf ("Press '4' to stop source 1\n");
  printf ("Press 'q' to quit\n");

  do
    {
      ch = getUpperCh ();

      switch (ch)
        {
        case '1':
          alSourcePlay (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcePlay source 0 : ", error);
          break;
        case '2':
          alSourcePlay (source[1]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcePlay source 1 : ", error);
          break;
        case '3':
          alSourceStop (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourceStop source 0 : ", error);
          break;
        case '4':
          alSourceStop (source[1]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourceStop source 1 : ", error);
          break;
        }
    }
  while (ch != 'Q');

  /* Release resources */
  alSourceStopv (2, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourceStopv 2 : ", error);

  alDeleteSources (2, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alDeleteSources 2 : ", error);
}

/* Looping Test */
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Looping Test
This test checks the ability to switch Looping ON and OFF for a particular source, either before
or during Playback.  (If looping is switched off during playback, the buffer should finish playing
until the end of the sample.)
*/
ALvoid
I_LoopingTest (ALvoid)
{
  ALint error;
  ALuint source[2];
  ALbyte ch;
  ALboolean bLooping0 = AL_FALSE;
  ALboolean bLooping1 = AL_FALSE;

  ALfloat source0Pos[] = { -1.0, 0.0, -1.0 };   /* Front left of the listener */
  ALfloat source0Vel[] = { 0.0, 0.0, 0.0 };

  ALfloat source1Pos[] = { 1.0, 0.0, -1.0 };    /* Front right of the listener */
  ALfloat source1Vel[] = { 0.0, 0.0, 0.0 };

  /* Clear Error Code */
  alGetError ();

  alGenSources (2, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    {
      DisplayALError ((ALbyte *) "alGenSources 1 : ", error);
      return;
    }

  alSourcef (source[0], AL_PITCH, 1.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcef 0 AL_PITCH : \n", error);

  alSourcef (source[0], AL_GAIN, 1.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcef 0 AL_GAIN : \n", error);

  alSourcefv (source[0], AL_POSITION, source0Pos);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcefv 0 AL_POSITION : \n", error);

  alSourcefv (source[0], AL_VELOCITY, source0Vel);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcefv 0 AL_VELOCITY : \n", error);

  alSourcei (source[0], AL_BUFFER, g_Buffers[0]);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcei 0 AL_BUFFER buffer 0 : \n", error);

  alSourcei (source[0], AL_LOOPING, AL_FALSE);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcei 0 AL_LOOPING AL_FALSE : \n", error);

  alSourcef (source[1], AL_PITCH, 1.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcef 1 AL_PITCH : \n", error);

  alSourcef (source[1], AL_GAIN, 1.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcef 1 AL_GAIN : \n", error);

  alSourcefv (source[1], AL_POSITION, source1Pos);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcefv 1 AL_POSITION : \n", error);

  alSourcefv (source[1], AL_VELOCITY, source1Vel);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcefv 1 AL_VELOCITY : \n", error);

  alSourcei (source[1], AL_BUFFER, g_Buffers[1]);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcei 1 AL_BUFFER buffer 1 : \n", error);

  alSourcei (source[1], AL_LOOPING, AL_FALSE);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcei 1 AL_LOOPING AL_FALSE: \n", error);

  printf ("Looping Test\n");
  printf ("Press '1' to play source 0 once (single shot)\n");
  printf ("Press '2' to toggle looping on source 0\n");
  printf ("Press '3' to play source 1 once (single shot)\n");
  printf ("Press '4' to toggle looping on source 1\n");
  printf ("Press 'q' to quit\n");
  printf ("\nSource 0 : Not looping Source 1 : Not looping\n");
  do
    {
      ch = getUpperCh ();

      switch (ch)
        {
        case '1':
          alSourcePlay (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcePlay source 0 : ", error);
          break;
        case '2':
          if (bLooping0 == AL_FALSE)
            {
              bLooping0 = AL_TRUE;
              if (bLooping1)
                printf ("Source 0 : Looping     Source 1 : Looping    \n");
              else
                printf ("Source 0 : Looping     Source 1 : Not looping\n");
            }
          else
            {
              bLooping0 = AL_FALSE;
              if (bLooping1)
                printf ("Source 0 : Not looping Source 1 : Looping    \n");
              else
                printf ("Source 0 : Not looping Source 1 : Not looping\n");
            }
          alSourcei (source[0], AL_LOOPING, bLooping0);
          break;
        case '3':
          alSourcePlay (source[1]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcePlay source 1 : ", error);
          break;
        case '4':
          if (bLooping1 == AL_FALSE)
            {
              bLooping1 = AL_TRUE;
              if (bLooping0)
                printf ("Source 0 : Looping     Source 1 : Looping    \n");
              else
                printf ("Source 0 : Not looping Source 1 : Looping    \n");
            }
          else
            {
              bLooping1 = AL_FALSE;
              if (bLooping0)
                printf ("Source 0 : Looping     Source 1 : Not looping\n");
              else
                printf ("Source 0 : Not looping Source 1 : Not looping\n");
            }
          alSourcei (source[1], AL_LOOPING, bLooping1);
          break;
        }
    }
  while (ch != 'Q');

  printf ("\n");

  /* Release resources */
  alSourceStop (source[0]);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourceStop source 1 : ", error);

  alDeleteSources (2, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alDeleteSources 1 : ", error);
}

#if TEST_EAX
/* EAX Test */
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE EAX Test
This test Uses 2 Sources to test out EAX 2.0 Reverb, Occlusion and Obstruction.  Also tests the use
of the DEFERRED flag in EAX.
*/
ALvoid
I_EAXTest (ALvoid)
{
  ALint error;
  ALuint source[2];
  ALbyte ch;
  ALuint Env;
  ALint Room;
  ALint Occlusion;
  ALint Obstruction;
  EAXBUFFERPROPERTIES eaxBufferProp0;

  ALfloat source0Pos[] = { -1.0, 0.0, 1.0 };    /* Behind and to the left of the listener */
  ALfloat source0Vel[] = { 0.0, 0.0, 0.0 };

  ALfloat source1Pos[] = { 1.0, 0.0, -1.0 };    /* Front and right of the listener */
  ALfloat source1Vel[] = { 0.0, 0.0, 0.0 };

  /* Clear Error Code */
  alGetError ();

  alGenSources (2, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    {
      DisplayALError ((ALbyte *) "alGenSources 2 : ", error);
      return;
    }

  alSourcef (source[0], AL_PITCH, 1.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcef 0 AL_PITCH : \n", error);

  alSourcef (source[0], AL_GAIN, 1.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcef 0 AL_GAIN : \n", error);

  alSourcefv (source[0], AL_POSITION, source0Pos);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcefv 0 AL_POSITION : \n", error);

  alSourcefv (source[0], AL_VELOCITY, source0Vel);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcefv 0 AL_VELOCITY : \n", error);

  alSourcei (source[0], AL_BUFFER, g_Buffers[0]);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcei 0 AL_BUFFER buffer 0 : \n", error);

  alSourcei (source[0], AL_LOOPING, AL_TRUE);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcei 0 AL_LOOPING AL_TRUE: \n", error);

  alSourcef (source[1], AL_PITCH, 1.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcef 1 AL_PITCH : \n", error);

  alSourcef (source[1], AL_GAIN, 1.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcef 1 AL_GAIN : \n", error);

  alSourcefv (source[1], AL_POSITION, source1Pos);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcefv 1 AL_POSITION : \n", error);

  alSourcefv (source[1], AL_VELOCITY, source1Vel);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcefv 1 AL_VELOCITY : \n", error);

  alSourcei (source[1], AL_BUFFER, g_Buffers[1]);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcei 1 AL_BUFFER buffer 1 : \n", error);

  alSourcei (source[1], AL_LOOPING, AL_FALSE);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcei 1 AL_LOOPING AL_FALSE: \n", error);

  printf ("EAX Test\n\n");
  printf ("Press '1' to play source 0 (looping)\n");
  printf ("Press '2' to play source 1 once (single shot)\n");
  printf ("Press '3' to stop source 0\n");
  printf ("Press '4' to stop source 1\n");
  printf ("Press '5' to add Hangar reverb (DEFERRED)\n");
  printf ("Press '6' to remove reverb (DEFERRED)\n");
  printf ("Press '7' to occlude source 0 (DEFERRED)\n");
  printf ("Press '8' to remove occlusion from source 0 (DEFERRED)\n");
  printf ("Press '9' to obstruct source 1 (IMMEDIATE)\n");
  printf ("Press '0' to remove obstruction from source 1 (IMMEDIATE)\n");
  printf ("Press 'c' to COMMIT EAX settings\n");
  printf ("Press 'q' to quit\n\n");

  do
    {
      ch = getUpperCh ();

      switch (ch)
        {
        case '1':
          alSourcePlay (source[0]);
          break;
        case '2':
          alSourcePlay (source[1]);
          break;
        case '3':
          alSourceStop (source[0]);
          break;
        case '4':
          alSourceStop (source[1]);
          break;

        case '5':
          Env = EAX_ENVIRONMENT_HANGAR;
          eaxSet (&DSPROPSETID_EAX_ListenerProperties,
                  DSPROPERTY_EAXLISTENER_ENVIRONMENT |
                  DSPROPERTY_EAXLISTENER_DEFERRED, 0, &Env,
                  sizeof (ALuint));
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *)
                            "eaxSet EAXLISTENER_ENVIRONMENT | EAXLISTENER_DEFERRED : \n",
                            error);
          break;

        case '6':
          Room = -10000;
          eaxSet (&DSPROPSETID_EAX_ListenerProperties,
                  DSPROPERTY_EAXLISTENER_ROOM |
                  DSPROPERTY_EAXLISTENER_DEFERRED, 0, &Room,
                  sizeof (ALint));
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *)
                            "eaxSet EAXLISTENER_ROOM | EAXLISTENER_DEFERRED : \n",
                            error);
          break;

        case '7':
          eaxGet (&DSPROPSETID_EAX_BufferProperties,
                  DSPROPERTY_EAXBUFFER_ALLPARAMETERS, source[0],
                  &eaxBufferProp0, sizeof (EAXBUFFERPROPERTIES));
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "eaxGet EAXBUFFER_ALLPARAMETERS : \n",
                            error);
          eaxBufferProp0.lOcclusion = -5000;
          eaxSet (&DSPROPSETID_EAX_BufferProperties,
                  DSPROPERTY_EAXBUFFER_ALLPARAMETERS |
                  DSPROPERTY_EAXBUFFER_DEFERRED, source[0], &eaxBufferProp0,
                  sizeof (EAXBUFFERPROPERTIES));
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *)
                            "eaxSet EAXBUFFER_ALLPARAMETERS | EAXBUFFER_DEFERRED : \n",
                            error);
          break;

        case '8':
          Occlusion = 0;
          eaxSet (&DSPROPSETID_EAX_BufferProperties,
                  DSPROPERTY_EAXBUFFER_OCCLUSION |
                  DSPROPERTY_EAXBUFFER_DEFERRED, source[0], &Occlusion,
                  sizeof (ALint));
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *)
                            "eaxSet EAXBUFFER_OCCLUSION | EAXBUFFER_DEFERRED : \n",
                            error);
          break;

        case '9':
          Obstruction = -5000;
          eaxSet (&DSPROPSETID_EAX_BufferProperties,
                  DSPROPERTY_EAXBUFFER_OBSTRUCTION, source[1], &Obstruction,
                  sizeof (ALint));
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "eaxSet EAXBUFFER_OBSTRUCTION : \n",
                            error);
          break;

        case '0':
          Obstruction = 0;
          eaxSet (&DSPROPSETID_EAX_BufferProperties,
                  DSPROPERTY_EAXBUFFER_OBSTRUCTION, source[1], &Obstruction,
                  sizeof (ALint));
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "eaxSet EAXBUFFER_OBSTRUCTION : \n",
                            error);
          break;

        case 'C':
          /* Commit settings on source 0 */
          eaxSet (&DSPROPSETID_EAX_BufferProperties,
                  DSPROPERTY_EAXBUFFER_COMMITDEFERREDSETTINGS, source[0],
                  NULL, 0);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *)
                            "eaxSet EAXBUFFER_COMMITDEFERREDSETTINGS : \n",
                            error);

          /* Commit Listener settings */
          eaxSet (&DSPROPSETID_EAX_ListenerProperties,
                  DSPROPERTY_EAXLISTENER_COMMITDEFERREDSETTINGS, 0, NULL,
                  0);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *)
                            "eaxSet EAXLISTENER_COMMITDEFERREDSETTINGSENVIRONMENT : \n",
                            error);
          break;
        }
    }
  while (ch != 'Q');

  /* reset EAX level */
  Room = -10000;
  eaxSet (&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOM,
          0, &Room, sizeof (ALint));
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "eaxSet EAXLISTENER_ROOM : \n", error);

  /* Release resources */
  alSourceStopv (2, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourceStopv 2 : ", error);

  alDeleteSources (2, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alDeleteSources 2 : ", error);
}
#endif

/* Queue Test */
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Queue Test
This test checks the ability to queue and unqueue a sequence of buffers on a Source. (Buffers
can only be     unqueued if they have been PROCESSED by a Source.)
*/
ALvoid
I_QueueTest (ALvoid)
{
  ALint error;
  ALuint source[1];
  ALbyte ch;
  ALuint buffers[5];
  ALboolean bLooping;
  ALint BuffersInQueue, BuffersProcessed;
  ALfloat source0Pos[] = { 0.0, 0.0, -1.0 };    /* Immediately in front of listener */
  ALfloat source0Vel[] = { 0.0, 0.0, 0.0 };
  ALuint Buffer;
  ALint i;

  /* Clear Error Code */
  alGetError ();

  alGenSources (1, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    {
      DisplayALError ("alGenSources 1 : ", error);
      return;
    }

  alSourcef (source[0], AL_PITCH, 1.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourcef 0 AL_PITCH : ", error);

  alSourcef (source[0], AL_GAIN, 1.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourcef 0 AL_GAIN : ", error);

  alSourcefv (source[0], AL_POSITION, source0Pos);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourcefv 0 AL_POSITION : ", error);

  alSourcefv (source[0], AL_VELOCITY, source0Vel);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourcefv 0 AL_VELOCITY : ", error);

  alSourcei (source[0], AL_LOOPING, AL_FALSE);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourcei 0 AL_LOOPING AL_FALSE: ", error);

  bLooping = AL_FALSE;

  printf ("Queue Test\n\n");
  printf ("Press '1' to start playing source 0\n");
  printf ("Press '2' to stop source 0\n");
  printf ("Press '3' to toggle looping on source 0\n");
  printf ("Press '4' to queue 4 buffers on source 0\n");
  printf ("Press '5' to queue 1st buffer on source 0\n");
  printf ("Press '6' to queue 2nd buffer on source 0\n");
  printf ("Press '7' to queue 3rd buffer on source 0\n");
  printf ("Press '8' to queue 4th buffer on source 0\n");
  printf ("Press '9' to queue 5th buffer (Buffer 0) on source 0\n");
  printf ("Press '0' to display stats\n");

  printf ("Press 'a' to unqueue first Buffer\n");
  printf ("Press 'b' to unqueue first 2 Buffers\n");
  printf ("Press 'c' to unqueue first 3 Buffers\n");
  printf ("Press 'd' to unqueue first 4 Buffers\n");
  printf ("Press 'e' to unqueue first 5 Buffers\n");
  printf ("Press 'f' to unqueue all buffers\n");

  printf ("Press 'q' to quit\n");

  printf ("Source 0 not looping\n");

  buffers[0] = g_Buffers[2];
  buffers[1] = g_Buffers[3];
  buffers[2] = g_Buffers[4];
  buffers[3] = g_Buffers[5];
  buffers[4] = 0;

  do
    {
      ch = getUpperCh ();
      switch (ch)
        {
        case '1':
          alSourcePlay (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourcePlay source 0 : ", error);
          break;
        case '2':
          alSourceStop (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourceStop source 0 : ", error);
          break;
        case '3':
          if (bLooping == AL_TRUE)
            {
              bLooping = AL_FALSE;
              printf ("Source 0 not looping\n");
            }
          else
            {
              bLooping = AL_TRUE;
              printf ("Source 0 looping    \n");
            }
          alSourcei (source[0], AL_LOOPING, bLooping);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourcei AL_LOOPING : ", error);
          break;
        case '4':
          alSourceQueueBuffers (source[0], 4, buffers);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourceQueueBuffers 4 : ", error);
          break;
        case '5':
          alSourceQueueBuffers (source[0], 1, &buffers[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourceQueueBuffers 1 : ", error);
          break;
        case '6':
          alSourceQueueBuffers (source[0], 1, &buffers[1]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourceQueueBuffers 1 : ", error);
          break;
        case '7':
          alSourceQueueBuffers (source[0], 1, &buffers[2]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourceQueueBuffers 1 : ", error);
          break;
        case '8':
          alSourceQueueBuffers (source[0], 1, &buffers[3]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourceQueueBuffers 1 : ", error);
          break;
        case '9':
          /* Queue buffer 0 */
          alSourceQueueBuffers (source[0], 1, &buffers[4]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourceQueueBuffers 1 (buffer 0) : ", error);
          break;
        case 'A':
          {
            /* Unqueue first Buffer */
            ALuint buffersremoved[1];
            alSourceUnqueueBuffers (source[0], 1, buffersremoved);

            if ((error = alGetError ()) != AL_NO_ERROR)
              {
                DisplayALError ("alSourceUnqueueBuffers 1 : ", error);
              }
            else
              {
                if (buffersremoved[0] == buffers[0])
                  buffersremoved[0] = 1;
                else if (buffersremoved[0] == buffers[1])
                  buffersremoved[0] = 2;
                else if (buffersremoved[0] == buffers[2])
                  buffersremoved[0] = 3;
                else if (buffersremoved[0] == buffers[3])
                  buffersremoved[0] = 4;
                else
                  buffersremoved[0] = 0;

                printf ("\nRemoved Buffer %d from queue\n",
                        buffersremoved[0]);
              }
          }
          break;
        case 'B':
          {
            /* Unqueue first 2 Buffers */
            ALuint buffersremoved[2];
            alSourceUnqueueBuffers (source[0], 2, buffersremoved);

            if ((error = alGetError ()) != AL_NO_ERROR)
              {
                DisplayALError ("alSourceUnqueueBuffers 2 : ", error);
              }
            else
              {
                for (i = 0; i < 2; i++)
                  {
                    if (buffersremoved[i] == buffers[0])
                      buffersremoved[i] = 1;
                    else if (buffersremoved[i] == buffers[1])
                      buffersremoved[i] = 2;
                    else if (buffersremoved[i] == buffers[2])
                      buffersremoved[i] = 3;
                    else if (buffersremoved[i] == buffers[3])
                      buffersremoved[i] = 4;
                    else
                      buffersremoved[i] = 0;
                  }

                printf ("\nRemoved Buffers %d and %d from queue\n",
                        buffersremoved[0], buffersremoved[1]);
              }
          }
          break;
        case 'C':
          {
            /* Unqueue first 3 Buffers */
            ALuint buffersremoved[3];
            alSourceUnqueueBuffers (source[0], 3, buffersremoved);
            if ((error = alGetError ()) != AL_NO_ERROR)
              {
                DisplayALError ("alSourceUnqueueBuffers 3 : ", error);
              }
            else
              {
                for (i = 0; i < 3; i++)
                  {
                    if (buffersremoved[i] == buffers[0])
                      buffersremoved[i] = 1;
                    else if (buffersremoved[i] == buffers[1])
                      buffersremoved[i] = 2;
                    else if (buffersremoved[i] == buffers[2])
                      buffersremoved[i] = 3;
                    else if (buffersremoved[i] == buffers[3])
                      buffersremoved[i] = 4;
                    else
                      buffersremoved[i] = 0;
                  }

                printf ("\nRemoved Buffers %d, %d and %d from queue\n",
                        buffersremoved[0], buffersremoved[1],
                        buffersremoved[2]);
              }
          }
          break;
        case 'D':
          {
            /* Unqueue first 4 Buffers */
            ALuint buffersremoved[4];
            alSourceUnqueueBuffers (source[0], 4, buffersremoved);

            if ((error = alGetError ()) != AL_NO_ERROR)
              {
                DisplayALError ("alSourceUnqueueBuffers 1 : ", error);
              }
            else
              {
                for (i = 0; i < 4; i++)
                  {
                    if (buffersremoved[i] == buffers[0])
                      buffersremoved[i] = 1;
                    else if (buffersremoved[i] == buffers[1])
                      buffersremoved[i] = 2;
                    else if (buffersremoved[i] == buffers[2])
                      buffersremoved[i] = 3;
                    else if (buffersremoved[i] == buffers[3])
                      buffersremoved[i] = 4;
                    else
                      buffersremoved[i] = 0;
                  }

                printf ("\nRemoved Buffers %d, %d, %d and %d from queue\n",
                        buffersremoved[0], buffersremoved[1],
                        buffersremoved[2], buffersremoved[3]);
              }
          }
          break;
        case 'E':
          {
            /* Unqueue first 5 Buffers */
            ALuint buffersremoved[5];
            alSourceUnqueueBuffers (source[0], 5, buffersremoved);

            if ((error = alGetError ()) != AL_NO_ERROR)
              {
                DisplayALError ("alSourceUnqueueBuffers 1 : ", error);
              }
            else
              {
                for (i = 0; i < 5; i++)
                  {
                    if (buffersremoved[i] == buffers[0])
                      buffersremoved[i] = 1;
                    else if (buffersremoved[i] == buffers[1])
                      buffersremoved[i] = 2;
                    else if (buffersremoved[i] == buffers[2])
                      buffersremoved[i] = 3;
                    else if (buffersremoved[i] == buffers[3])
                      buffersremoved[i] = 4;
                    else
                      buffersremoved[i] = 0;
                  }

                printf
                  ("\nRemoved Buffers %d, %d, %d, %d and %d from queue\n",
                   buffersremoved[0], buffersremoved[1], buffersremoved[2],
                   buffersremoved[3], buffersremoved[4]);
              }
          }
          break;
        case 'F':
          alSourcei (source[0], AL_BUFFER, 0);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSource AL_BUFFER NULL : ", error);
          break;
        case '0':
          /* Retrieve number of buffers in queue */
          alGetSourcei (source[0], AL_BUFFERS_QUEUED, &BuffersInQueue);
          /* Retrieve number of processed buffers */
          alGetSourcei (source[0], AL_BUFFERS_PROCESSED, &BuffersProcessed);
          /* Retrieve current buffer */
          alGetSourcei (source[0], AL_BUFFER, (ALint *) &Buffer);
          if (Buffer == buffers[0])
            Buffer = 1;
          else if (Buffer == buffers[1])
            Buffer = 2;
          else if (Buffer == buffers[2])
            Buffer = 3;
          else if (Buffer == buffers[3])
            Buffer = 4;
          else
            Buffer = 0;

          printf ("Current Buffer is %d, %d Buffers in queue, %d Processed\n",
                  Buffer, BuffersInQueue, BuffersProcessed);

          break;
        }
    }
  while (ch != 'Q');

  /* Release resources */
  alSourceStop (source[0]);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourceStop : ", error);

  alDeleteSources (1, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alDeleteSources 1 : ", error);
}

/* Frequency Test */
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Frequency Test
This test exercises AL_PITCH functionality
*/
ALvoid
I_FreqTest (ALvoid)
{
  ALint error;
  ALuint source[1];
  ALbyte ch;
  ALfloat source0Pos[] = { 1.0, 0.0, -1.0 };    /* Front and right of the listener */
  ALfloat source0Vel[] = { 0.0, 0.0, 0.0 };

  alGenSources (1, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    {
      DisplayALError ((ALbyte *) "alGenSources 1 : ", error);
      return;
    }

  alSourcef (source[0], AL_PITCH, 1.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcef 0 AL_PITCH : \n", error);

  alSourcef (source[0], AL_GAIN, 1.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcef 0 AL_GAIN : \n", error);

  alSourcefv (source[0], AL_POSITION, source0Pos);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcefv 0 AL_POSITION : \n", error);

  alSourcefv (source[0], AL_VELOCITY, source0Vel);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcefv 0 AL_VELOCITY : \n", error);

  alSourcei (source[0], AL_BUFFER, g_Buffers[1]);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcei 0 AL_BUFFER buffer 1 : \n", error);

  alSourcei (source[0], AL_LOOPING, AL_TRUE);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcei 0 AL_LOOPING AL_TRUE: \n", error);

  printf ("Frequency Test\n");
  printf ("Press '1' to play source 0 (looping)\n");
  printf ("Press '2' to Double Frequency\n");
  printf ("Press '3' to Halve Frequency\n");
  printf ("Press 'q' to quit\n");

  do
    {
      ch = getUpperCh ();

      switch (ch)
        {
        case '1':
          alSourcef (source[0], AL_PITCH, 1.0f);
          alSourcePlay (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcePlay source 0 : ", error);
          break;
        case '2':
          alSourcef (source[0], AL_PITCH, 2.0f);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcef source 0 AL_PITCH 2.0 : ",
                            error);
          break;
        case '3':
          alSourcef (source[0], AL_PITCH, 0.5f);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcef source 0 AL PITCH 0.5: ",
                            error);
          break;
        }
    }
  while (ch != 'Q');

  /* Release resources */
  alSourceStopv (1, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourceStopv 2 : ", error);

  alDeleteSources (1, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alDeleteSources 2 : ", error);
}

/* Stereo Test */
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Stereo Test
This test plays a stereo buffer.
*/
ALvoid
I_StereoTest (ALvoid)
{
  ALint error;
  ALuint source[1];
  ALuint buffers[2];
  ALuint Buffer;
  ALint BuffersInQueue, BuffersProcessed;
  ALbyte ch;
  ALboolean bLoop = AL_TRUE;
  ALfloat source0Pos[] = { 1.0, 0.0, -1.0 };    /* Front and right of the listener */
  ALfloat source0Vel[] = { 0.0, 0.0, 0.0 };

  alGenSources (1, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    {
      DisplayALError ((ALbyte *) "alGenSources 1 : ", error);
      return;
    }

  buffers[0] = g_Buffers[6];
  buffers[1] = g_Buffers[6];

  printf ("Stereo Test\n");
  printf ("Press '1' to play a stereo buffer on source 0 (looping)\n");
  printf ("Press '2' to play a mono buffer on source 0 (looping)\n");
  printf ("Press '3' to stop source 0\n");
  printf
    ("Press '4' to queue 2 stereo buffers on source 0 and start playing\n");
  printf ("Press '5' to unqueue the 2 stereo buffers on source 0\n");
  printf ("Press '6' to toggle looping on / off\n");
  printf ("Press '0' to display stats\n");
  printf ("Press 'q' to quit\n");
  printf ("Looping is on\n");

  do
    {
      ch = getUpperCh ();

      switch (ch)
        {
        case '1':
          /* Stop source */
          alSourceStop (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourceStop source 0 : ", error);

          /* Attach new buffer */
          alSourcei (source[0], AL_BUFFER, g_Buffers[6]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *)
                            "alSourcei 0 AL_BUFFER buffer 6 (stereo) : \n",
                            error);

          /* Set volume */
          alSourcef (source[0], AL_GAIN, 0.5f);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcef 0 AL_GAIN : \n", error);

          /* Set looping */
          alSourcei (source[0], AL_LOOPING, bLoop);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcei 0 AL_LOOPING AL_TRUE: \n",
                            error);

          /* Play source */
          alSourcePlay (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcePlay source 0 : ", error);

          break;
        case '2':
          /* Stop source */
          alSourceStop (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourceStop source 0 : ", error);

          /* Attach new buffer */
          alSourcei (source[0], AL_BUFFER, g_Buffers[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *)
                            "alSourcei 0 AL_BUFFER buffer 0 (mono) : \n",
                            error);

          /* Set 3D position */
          alSourcefv (source[0], AL_POSITION, source0Pos);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcefv 0 AL_POSITION : \n",
                            error);

          /* Set 3D velocity */
          alSourcefv (source[0], AL_VELOCITY, source0Vel);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcefv 0 AL_VELOCITY : \n",
                            error);

          /* Set volume to full */
          alSourcef (source[0], AL_GAIN, 1.0f);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcef 0 AL_GAIN : \n", error);

          /* Set Looping */
          alSourcei (source[0], AL_LOOPING, bLoop);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcei 0 AL_LOOPING : \n", error);

          /* Play source */
          alSourcePlay (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcePlay source 0 : ", error);
          break;
        case '3':
          alSourceStop (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourceStop source 0 : ", error);
          break;
        case '4':
          alSourceStop (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourceStop Source 0 : ", error);

          /* Attach NULL buffer to source to clear everything */
          alSourcei (source[0], AL_BUFFER, 0);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcei AL_BUFFER (NULL) : ",
                            error);

          alSourceQueueBuffers (source[0], 2, buffers);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourceQueueBuffers 2 (stereo) : ",
                            error);

          /* Set Looping */
          alSourcei (source[0], AL_LOOPING, bLoop);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcei 0 AL_LOOPING : \n", error);

          alSourcePlay (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcePlay Source 0 : ", error);
          break;
        case '5':
          alSourceUnqueueBuffers (source[0], 2, buffers);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourceUnqueueBuffers 2 (stereo) : ",
                            error);
          break;
        case '6':
          if (bLoop)
            {
              printf ("Looping is off\n");
              bLoop = AL_FALSE;
            }
          else
            {
              printf ("Looping is on  \n");
              bLoop = AL_TRUE;
            }
          alSourcei (source[0], AL_LOOPING, bLoop);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcei 0 AL_LOOPING : \n", error);
          break;
        case '0':
          /* Retrieve number of buffers in queue */
          alGetSourcei (source[0], AL_BUFFERS_QUEUED, &BuffersInQueue);
          /* Retrieve number of processed buffers */
          alGetSourcei (source[0], AL_BUFFERS_PROCESSED, &BuffersProcessed);
          /* Retrieve current buffer */
          alGetSourcei (source[0], AL_BUFFER, (ALint *) &Buffer);
          if (Buffer == buffers[0])
            Buffer = 6;
          else if (Buffer == buffers[1])
            Buffer = 6;
          else
            Buffer = 0;

          printf ("Current Buffer is %d, %d Buffers in queue, %d Processed\n",
                  Buffer, BuffersInQueue, BuffersProcessed);

          break;
        }
    }
  while (ch != 'Q');

  /* Release resources */
  alSourceStop (source[0]);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourceStop : ", error);

  alDeleteSources (1, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alDeleteSources 2 : ", error);
}

/* Gain Test */
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Gain Test
This test plays two sources, allowing the control of source and listener gain.
*/
ALvoid
I_GainTest (ALvoid)
{
  ALint error;
  ALuint source[2];
  ALbyte ch;

  ALfloat source0Pos[] = { 1.0, 0.0, -1.0 };    /* Front and right of the listener */
  ALfloat source0Vel[] = { 0.0, 0.0, 0.0 };

  ALfloat source1Pos[] = { -1.0, 0.0, -1.0 };
  ALfloat source1Vel[] = { 0.0, 0.0, 0.0 };

  alGenSources (2, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    {
      DisplayALError ((ALbyte *) "alGenSources 2 : ", error);
      return;
    }

  alSourcef (source[0], AL_PITCH, 1.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcef 0 AL_PITCH : \n", error);

  alSourcef (source[0], AL_GAIN, 1.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcef 0 AL_GAIN : \n", error);

  alSourcefv (source[0], AL_POSITION, source0Pos);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcefv 0 AL_POSITION : \n", error);

  alSourcefv (source[0], AL_VELOCITY, source0Vel);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcefv 0 AL_VELOCITY : \n", error);

  alSourcei (source[0], AL_BUFFER, g_Buffers[0]);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcei 0 AL_BUFFER buffer 0 : \n", error);

  alSourcei (source[0], AL_LOOPING, AL_TRUE);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcei 0 AL_LOOPING AL_TRUE: \n", error);

  alSourcef (source[1], AL_PITCH, 1.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcef 1 AL_PITCH : \n", error);

  alSourcef (source[1], AL_GAIN, 1.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcef 1 AL_GAIN : \n", error);

  alSourcefv (source[1], AL_POSITION, source1Pos);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcefv 1 AL_POSITION : \n", error);

  alSourcefv (source[1], AL_VELOCITY, source1Vel);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcefv 1 AL_VELOCITY : \n", error);

  alSourcei (source[1], AL_BUFFER, g_Buffers[1]);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcei 1 AL_BUFFER buffer 1 : \n", error);

  alSourcei (source[1], AL_LOOPING, AL_TRUE);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourcei 1 AL_LOOPING AL_TRUE: \n", error);

  printf ("Gain Test\n");
  printf ("Press '1' to play source 0 (looping)\n");
  printf ("Press '2' to play source 1 (looping)\n");
  printf ("Press '3' to stop source 0\n");
  printf ("Press '4' to stop source 1\n");
  printf ("Press '5' to set source 0 gain to 1.0\n");
  printf ("Press '6' to set source 0 gain to 0.5\n");
  printf ("Press '7' to set source 0 gain to 0.25\n");
  printf ("Press '8' to set source 0 gain to 0\n");
  printf ("Press 'a' to set Listener Gain to 1.0\n");
  printf ("Press 'b' to set Listener Gain to 0.5\n");
  printf ("Press 'c' to set Listener Gain to 0.25\n");
  printf ("Press 'd' to set Listener Gain to 0.0\n");
  printf ("Press 'q' to quit\n");

  do
    {
      ch = getUpperCh ();

      switch (ch)
        {
        case '1':
          alSourcePlay (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcePlay source 0 : ", error);
          break;
        case '2':
          alSourcePlay (source[1]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcePlay source 1 : ", error);
          break;
        case '3':
          alSourceStop (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourceStop source 0 : \n", error);
          break;
        case '4':
          alSourceStop (source[1]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourceStop source 1 : \n", error);
          break;
        case '5':
          alSourcef (source[0], AL_GAIN, 1.0f);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcef 0 AL_GAIN 1.0 : \n", error);
          break;
        case '6':
          alSourcef (source[0], AL_GAIN, 0.5f);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcef 0 AL_GAIN 0.5 : \n", error);
          break;
        case '7':
          alSourcef (source[0], AL_GAIN, 0.25f);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcef 0 AL_GAIN 0.25 : \n",
                            error);
          break;
        case '8':
          alSourcef (source[0], AL_GAIN, 0.0f);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alSourcef 0 AL_GAIN 0.0 : \n", error);
          break;
        case 'A':
          alListenerf (AL_GAIN, 1.0f);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alListenerf AL_GAIN 1.0 : \n", error);
          break;
        case 'B':
          alListenerf (AL_GAIN, 0.5f);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alListenerf AL_GAIN 0.5 : \n", error);
          break;
        case 'C':
          alListenerf (AL_GAIN, 0.25f);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alListenerf AL_GAIN 0.25 : \n",
                            error);
          break;
        case 'D':
          alListenerf (AL_GAIN, 0.0f);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ((ALbyte *) "alListenerf AL_GAIN 0.0 : \n", error);
          break;
        }
    }
  while (ch != 'Q');

  /* Reset & Release resources */
  alListenerf (AL_GAIN, 1.0f);
  alSourceStopv (2, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alSourceStop : ", error);

  alDeleteSources (2, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ((ALbyte *) "alDeleteSources 2 : ", error);
}

#ifdef __MACOS__
void
ALTestSwapWords (unsigned int *puint)
{
  unsigned int tempint;
  char *pChar1, *pChar2;

  tempint = *puint;
  pChar2 = (char *) &tempint;
  pChar1 = (char *) puint;

  pChar1[0] = pChar2[3];
  pChar1[1] = pChar2[2];
  pChar1[2] = pChar2[1];
  pChar1[3] = pChar2[0];
}

void
ALTestSwapBytes (unsigned short *pshort)
{
  unsigned short tempshort;
  char *pChar1, *pChar2;

  tempshort = *pshort;
  pChar2 = (char *) &tempshort;
  pChar1 = (char *) pshort;

  pChar1[0] = pChar2[1];
  pChar1[1] = pChar2[0];
}
#endif
#ifdef MAC_OS_X
void
ALTestSwapWords (unsigned int *puint)
{
  unsigned int tempint;
  char *pChar1, *pChar2;

  tempint = *puint;
  pChar2 = (char *) &tempint;
  pChar1 = (char *) puint;

  pChar1[0] = pChar2[3];
  pChar1[1] = pChar2[2];
  pChar1[2] = pChar2[1];
  pChar1[3] = pChar2[0];
}

void
ALTestSwapBytes (unsigned short *pshort)
{
  unsigned short tempshort;
  char *pChar1, *pChar2;

  tempshort = *pshort;
  pChar2 = (char *) &tempshort;
  pChar1 = (char *) pshort;

  pChar1[0] = pChar2[1];
  pChar1[1] = pChar2[0];
}
#endif

#define BSIZE 20000
#define NUMBUFFERS      4

/* Streaming Test */
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Streaming Test
This test streams a long stereo wave file from harddisc - using AL Queuing
*/
ALvoid
I_StreamingTest (ALvoid)
{
  FILE *fp;
  WAVE_Struct wave;
  ALbyte data[BSIZE];
  ALuint SourceID;
  ALuint BufferID[NUMBUFFERS];
  ALuint TempBufferID;
  ALuint ulBuffersAvailable;
  ALuint ulUnqueueCount, ulQueueCount, ulDataSize, ulDataToRead;
  ALint error, lProcessed, lPlaying, lLoop;

  printf ("Streaming Test\n");

  fp = fopen (FILENAME_STEREO, "rb");
  if (fp == NULL)
    {
      printf ("Failed to open %s\n", FILENAME_STEREO);
      return;
    }

  /* Read in WAVE Header */
  if (fread (&wave, 1, sizeof (WAVE_Struct), fp) != sizeof (WAVE_Struct))
    {
      printf ("Invalid wave file\n");
      fclose (fp);
      return;
    }

#ifdef SWAPBYTES
  ALTestSwapWords (&wave.dataSize);
  ALTestSwapBytes (&wave.Channels);
  ALTestSwapWords (&wave.SamplesPerSec);
#endif

  /* Generate a number of buffers to be used to queue data on to Source */
  alGenBuffers (NUMBUFFERS, BufferID);
  if ((error = alGetError ()) != AL_NO_ERROR)
    {
      DisplayALError ("alGenBuffers : ", error);
      fclose (fp);
      return;
    }

  /* Generate a Source */
  alGenSources (1, &SourceID);
  if ((error = alGetError ()) != AL_NO_ERROR)
    {
      DisplayALError ("alGenSources 1 : ", error);
      alDeleteBuffers (NUMBUFFERS, BufferID);
      fclose (fp);
      return;
    }

  ulUnqueueCount = 0;
  ulQueueCount = 0;
  ulBuffersAvailable = NUMBUFFERS;
  ulDataSize = wave.dataSize;

  while (!KBHIT ())
    {
      /* Check how many Buffers have been processed */
      alGetSourcei (SourceID, AL_BUFFERS_PROCESSED, &lProcessed);
      while (lProcessed)
        {
          /* Unqueue the buffer */
          alSourceUnqueueBuffers (SourceID, 1, &TempBufferID);

          /* Update unqueue count */
          if (++ulUnqueueCount == NUMBUFFERS)
            ulUnqueueCount = 0;

          /* Increment buffers available */
          ulBuffersAvailable++;

          /* Decrement lProcessed count */
          lProcessed--;
        }

      /* If there is more data to read, and available buffers ... read in more data and fill the buffers ! */
      if ((ulDataSize) && (ulBuffersAvailable))
        {
          ulDataToRead = (ulDataSize > BSIZE) ? BSIZE : ulDataSize;

          fread (data, ulDataToRead, 1, fp);
          ulDataSize -= ulDataToRead;

          /* Copy to Buffer */
#ifdef SWAPBYTES
          for (int i = 0; i < BSIZE; i = i + 2)
            {
              ALTestSwapBytes ((unsigned short *) (data + i));
            }
#endif
          alBufferData (BufferID[ulQueueCount], AL_FORMAT_STEREO16, data,
                        ulDataToRead, wave.SamplesPerSec);

          /* Queue the buffer */
          alSourceQueueBuffers (SourceID, 1, &BufferID[ulQueueCount]);

          if (++ulQueueCount == NUMBUFFERS)
            ulQueueCount = 0;

          /* Decrement buffers available */
          ulBuffersAvailable--;
        }

      /* If all the Buffers are available then we must have finished */
      if (ulBuffersAvailable == NUMBUFFERS)
        break;

      /* If the Source has stopped (been starved of data) it will need to be restarted */
      alGetSourcei (SourceID, AL_SOURCE_STATE, &lPlaying);
      if (lPlaying != AL_PLAYING)
        {
          /* If the Source has stopped prematurely, all the processed buffers must be unqueued */
          /* before re-playing (otherwise they will be heard twice). */
          /* Any buffers queued after the Source stopped will not be processed, so they won't */
          /* be unqueued by this step */
          alGetSourcei (SourceID, AL_BUFFERS_PROCESSED, &lProcessed);
          while (lProcessed)
            {
              alSourceUnqueueBuffers (SourceID, 1, &TempBufferID);
              if (++ulUnqueueCount == NUMBUFFERS)
                ulUnqueueCount = 0;
              ulBuffersAvailable++;
              lProcessed--;
            }

          alSourcePlay (SourceID);
        }
    }

  /* Clean-up */
  alSourceStop (SourceID);

  alDeleteSources (1, &SourceID);

  for (lLoop = 0; lLoop < NUMBUFFERS; lLoop++)
    alDeleteBuffers (1, &BufferID[lLoop]);
}

/* Multiple Sources Test */
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Multiple Sources Test
This test generates a large number of sources, positions, and plays them.
*/
ALvoid
I_MultipleSourcesTest ()
{
  ALuint numSources = 0;
  ALuint Sources[64] = { 0 };
  ALint error;
  ALuint i;
  char ch;
  ALfloat radius;
  double anglestep;
  ALfloat pos[3];

  /* Generate as many sources as possible (up to 64) */
  for (i = 0; i < 64; i++)
    {
      alGenSources (1, &Sources[i]);
      if ((error = alGetError ()) != AL_NO_ERROR)
        break;
      else
        numSources++;
    }

  printf ("Multiple Sources Test\n\n");
  printf ("Generated %d Sources\n", numSources);

  /* Set sources to located in a circle around the listener */

  anglestep = (2 * 3.1416) / (ALfloat) numSources;
  radius = 2.0f;

  for (i = 0; i < numSources; i++)
    {
      /* Attach buffer */
      alSourcei (Sources[i], AL_BUFFER, g_Buffers[0]);

      /* Set position */
      pos[0] = (float) (cos (anglestep * i) * radius);
      pos[1] = 0.0f;
      pos[2] = (float) (sin (anglestep * i) * radius);
      alSourcefv (Sources[i], AL_POSITION, pos);
      printf ("Source %d at %.3f, %.3f, %.3f\n", i, pos[0], pos[1], pos[2]);

      /* Enable looping */
      alSourcei (Sources[i], AL_LOOPING, AL_TRUE);
    }

  printf ("Press '1' to start playing Sources seperately\n");
  printf ("Press '2' to stop playing Sources seperately\n");
  printf ("Press 'q' to quit\n");

  do
    {
      ch = getUpperCh ();
      switch (ch)
        {
        case '1':
          for (i = 0; i < numSources; i++)
            {
              alSourcePlay (Sources[i]);
              if ((error = alGetError ()) != AL_NO_ERROR)
                DisplayALError ("alSourcePlay : ", error);

              /* Delay a little */
              sleepSeconds (0.1f);
            }
          break;
        case '2':
          alSourceStopv (numSources, Sources);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourceStopv : ", error);
          break;
        }
    }
  while (ch != 'Q');

  /* Delete the Sources */
  alDeleteSources (numSources, Sources);
}

/* Source Relative Test */
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Source Relative Test
*/
ALvoid
I_SourceRelativeTest ()
{
  ALuint source[1];
  ALint error;
  ALbyte ch;
  ALboolean bRelative = AL_FALSE;

  alGenSources (1, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    {
      DisplayALError ("alGenSources 1 : ", error);
      return;
    }

  alSourcei (source[0], AL_BUFFER, g_Buffers[1]);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourcei 0 AL_BUFFER buffer 0 : ", error);

  alSourcei (source[0], AL_LOOPING, AL_TRUE);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourcei 0 AL_LOOPING AL_TRUE: ", error);

  alSourcei (source[0], AL_SOURCE_RELATIVE, AL_FALSE);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourcei 0 AL_SOURCE_RELATIVE AL_FALSE : ", error);

  printf ("Source Relative Test\n");
  printf ("Press '1' to play source 0 at 1, 0, 0\n");
  printf ("Press '2' to move listener  to 2, 0, 0\n");
  printf ("Press '3' to toggle SOURCE RELATIVE Mode\n");
  printf ("Press 'q' to quit\n");

  do
    {
      ch = getUpperCh ();
      switch (ch)
        {
        case '1':
          alSource3f (source[0], AL_POSITION, 1.0f, 0.0f, 0.0f);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourcefv 0 AL_POSITION : ", error);

          alSourcePlay (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourcePlay source 0 : ", error);
          break;
        case '2':
          alListener3f (AL_POSITION, 2.0f, 0.0f, 0.0f);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alListenerfv  AL_POSITION : ", error);
          break;
        case '3':
          if (bRelative)
            {
              printf ("Source Relative == AL_FALSE\r");
              alSourcei (source[0], AL_SOURCE_RELATIVE, AL_FALSE);
              bRelative = AL_FALSE;
            }
          else
            {
              alSourcei (source[0], AL_SOURCE_RELATIVE, AL_TRUE);
              printf ("Source Relative == AL_TRUE \r");
              bRelative = AL_TRUE;
            }
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourcei source 0 AL_SOURCE_RELATIVE : ",
                            error);
          break;
        }
    }
  while (ch != 'Q');

  /* Release resources */
  alSourceStopv (1, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourceStopv 1 : ", error);

  alDeleteSources (1, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alDeleteSources 1 : ", error);

  /* Restore Listener Position */
  alListener3f (AL_POSITION, 0.0f, 0.0f, 0.0f);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alListenerfv  AL_POSITION : ", error);
}

/* Source Rolloff Test */
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Source Rolloff Test
*/
ALvoid
I_SourceRolloffTest ()
{
  ALint error;

  ALuint source[2];
  ALbyte ch;

  ALfloat source0Pos[] = { -4.0, 0.0, 4.0 };    /* Behind and to the left of the listener */
  ALfloat source1Pos[] = { 4.0, 0.0, -4.0 };    /* Front and to the right of the listener */

  alGenSources (2, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    {
      DisplayALError ("alGenSources 2 : ", error);
      return;
    }

  alSourcefv (source[0], AL_POSITION, source0Pos);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourcefv 0 AL_POSITION : ", error);

  alSourcei (source[0], AL_BUFFER, g_Buffers[1]);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourcei 0 AL_BUFFER buffer 0 : ", error);

  alSourcei (source[0], AL_LOOPING, AL_TRUE);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourcei 0 AL_LOOPING AL_TRUE: ", error);

  alSourcefv (source[1], AL_POSITION, source1Pos);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourcefv 1 AL_POSITION : ", error);

  alSourcei (source[1], AL_BUFFER, g_Buffers[1]);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourcei 1 AL_BUFFER buffer 1 : ", error);

  alSourcei (source[1], AL_LOOPING, AL_TRUE);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourcei 1 AL_LOOPING AL_FALSE: ", error);

  printf ("Source Roll-off Test\n");
  printf ("Press '1' to play source 0 rear left of listener\n");
  printf ("Press '2' to stop source 0\n");
  printf ("Press '3' to play source 1 front right of listener\n");
  printf ("Press '4' to stop source 1\n");
  printf ("Press '5' to set Source 0 Roff-off Factor to 0.5\n");
  printf ("Press '6' to set Source 0 Roll-off Factor to 1.0\n");
  printf ("Press '7' to set Source 0 Roll-off Factor to 2.0\n");
  printf ("Press '8' to set Source 1 Roff-off Factor to 0.5\n");
  printf ("Press '9' to set Source 1 Roll-off Factor to 1.0\n");
  printf ("Press 'A' to set Source 1 Roll-off Factor to 2.0\n");

  printf ("Press 'q' to quit\n");

  do
    {
      ch = getUpperCh ();
      switch (ch)
        {
        case '1':
          alSourcePlay (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourcePlay source 0 : ", error);
          break;
        case '2':
          alSourceStop (source[0]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourceStop source 0 : ", error);
          break;
        case '3':
          alSourcePlay (source[1]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourcePlay source 1 : ", error);
          break;
        case '4':
          alSourceStop (source[1]);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourceStop source 1 : ", error);
          break;
        case '5':
          alSourcef (source[0], AL_ROLLOFF_FACTOR, 0.5f);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourcef ROLLOFF_FACTOR 0.5 : ", error);
          break;
        case '6':
          alSourcef (source[0], AL_ROLLOFF_FACTOR, 1.0f);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourcef ROLLOFF_FACTOR 1.0 : ", error);
          break;
        case '7':
          alSourcef (source[0], AL_ROLLOFF_FACTOR, 2.0f);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourcef ROLLOFF_FACTOR 2.0 : ", error);
          break;
        case '8':
          alSourcef (source[1], AL_ROLLOFF_FACTOR, 0.5f);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourcef ROLLOFF_FACTOR 0.5 : ", error);
          break;
        case '9':
          alSourcef (source[1], AL_ROLLOFF_FACTOR, 1.0f);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourcef ROLLOFF_FACTOR 1.0 : ", error);
          break;
        case 'A':
          alSourcef (source[1], AL_ROLLOFF_FACTOR, 2.0f);
          if ((error = alGetError ()) != AL_NO_ERROR)
            DisplayALError ("alSourcef ROLLOFF_FACTOR 2.0 : ", error);
          break;
        }
    }
  while (ch != 'Q');

  /* Release resources */
  alSourceStopv (2, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourceStopv 2 : ", error);

  alDeleteSources (2, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alDeleteSources 2 : ", error);
}

/* ADPCM Test */
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE ADPCM Test
*/
ALvoid
I_ADPCMTest ()
{
  ALuint uiSourceID, uiBufferID;
  ALenum eMonoADPCM = 0, eStereoADPCM = 0;
  ALsizei size, freq;
  ALenum format;
  ALvoid *data;
  ALboolean loop;
  char ch;

  /* Check for support for ADPCM */
  eMonoADPCM = alGetEnumValue ("AL_FORMAT_MONO_IMA4");
  eStereoADPCM = alGetEnumValue ("AL_FORMAT_STEREO_IMA4");

  if (1 || (eMonoADPCM && eStereoADPCM))
    {
      /* Load ADPCM data */
      getWAVData (FILENAME_FOOTADPCM, &format, &data, &size, &freq, &loop);
      if (alGetError () != AL_NO_ERROR)
        {
          printf ("Failed to load %s\n", FILENAME_FOOTADPCM);
          return;
        }

      alGetError ();

      /* Generate an AL Buffer and an AL Source */
      alGenBuffers (1, &uiBufferID);
      /* Copy data to AL buffer */
      alBufferData (uiBufferID, format, data, size, freq);
      /* Release wave data */
      unloadWAVData (data);

      if (alGetError () != AL_NO_ERROR)
        {
          printf ("Failed to generate and copy data to buffer\n");
          return;
        }

      /* Generate a Source */
      alGenSources (1, &uiSourceID);
      /* Attach buffer to Source */
      alSourcei (uiSourceID, AL_BUFFER, uiBufferID);
      /* Enable Looping */
      alSourcei (uiSourceID, AL_LOOPING, AL_TRUE);
      /* Play Source */
      alSourcePlay (uiSourceID);

      printf ("Press a key to stop test\n");
      ch = getUpperCh ();

      /* Stop the Source */
      alSourceStop (uiSourceID);
      /* Attach NULL Buffer */
      alSourcei (uiSourceID, AL_BUFFER, 0);
      /* Delete Source */
      alDeleteSources (1, &uiSourceID);
      /* Delete Buffer */
      alDeleteBuffers (1, &uiBufferID);
    }
  else
    {
      printf ("No ADPCM support found !\n");
    }
}

/* Velocity Test */
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Velocity Test
*/
ALvoid
I_VelocityTest ()
{
  ALint error;

  ALuint source;
  ALbyte ch;

  ALfloat sourcePos[] = { 1.0, 0.0, 0.0 };
  ALfloat sourceVel[] = { 0.0, 0.0, 0.0 };

  ALfloat listenerPos[] = { 0.0, 0.0, 0.0 };
  ALfloat listenerVel[] = { 0.0, 0.0, 0.0 };

  ALCcontext *context = alcGetCurrentContext ();
  ALCdevice *device = alcGetContextsDevice (context);
  ALint major, minor;
  alcGetIntegerv (device, ALC_MAJOR_VERSION, 1, &major);
  alcGetIntegerv (device, ALC_MINOR_VERSION, 1, &minor);

  alGenSources (1, &source);

  alSourcefv (source, AL_POSITION, sourcePos);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourcefv 0 AL_POSITION : \n", error);

  alSourcefv (source, AL_VELOCITY, sourceVel);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourcefv 0 AL_VELOCITY : \n", error);

  alSourcei (source, AL_BUFFER, g_Buffers[1]);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourcei 0 AL_BUFFER buffer 0 : \n", error);

  alSourcei (source, AL_LOOPING, AL_TRUE);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourcei 0 AL_LOOPING AL_TRUE: \n", error);

  alListenerfv (AL_POSITION, listenerPos);
  alListenerfv (AL_VELOCITY, listenerVel);

  alSourcePlay (source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourcePlay source 0 : ", error);

  printf ("Velocity Test\n");
  printf ("Press '1' to increase source X velocity by 50 units / second\n");
  printf ("Press '2' to decrease source X velocity by 50 units / second\n");
  printf ("Press '3' to increase listener X velocity by 50 units / second\n");
  printf ("Press '4' to decrease listener X velocity by 50 units / second\n");
  printf ("Press '5' to set Speed Of Sound to 343.3\n");
  printf ("Press '6' to set Speed Of Sound to 686.6\n");
  printf ("Press '7' to set Speed Of Sound to 171.6\n");
  printf ("Press '8' to set Doppler Factor to 1\n");
  printf ("Press '9' to set Doppler Factor to 2\n");
  printf ("Press '0' to set Doppler Factor to 4\n");
  printf ("Press 'q' to quit\n");

  do
    {
      printf ("Source %.1f Listener %.1f\r", sourceVel[0], listenerVel[0]);

      ch = getUpperCh ();
      switch (ch)
        {
        case '1':
          sourceVel[0] += 50.0f;
          break;
        case '2':
          sourceVel[0] -= 50.0f;
          break;
        case '3':
          listenerVel[0] += 50.0f;
          break;
        case '4':
          listenerVel[0] -= 50.0f;
          break;
        case '5':
          if ((major > 1) || ((major == 1) && (minor >= 1)))
            alSpeedOfSound (343.3f);
          break;
        case '6':
          if ((major > 1) || ((major == 1) && (minor >= 1)))
            alSpeedOfSound (686.6f);
          break;
        case '7':
          if ((major > 1) || ((major == 1) && (minor >= 1)))
            alSpeedOfSound (171.6f);
          break;
        case '8':
          alDopplerFactor (1.0);
          break;
        case '9':
          alDopplerFactor (2.0);
          break;
        case '0':
          alDopplerFactor (4.0);
          break;
        default:
          break;
        }

      alSourcefv (source, AL_VELOCITY, sourceVel);
      alListenerfv (AL_VELOCITY, listenerVel);
    }
  while (ch != 'Q');

  /* Release resources */
  alSourceStopv (1, &source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourceStopv 2 : ", error);

  alDeleteSources (1, &source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alDeleteSources 2 : ", error);
}

/* Get Source Offset Test */
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Get Source Offset Test
*/
ALvoid
I_GetSourceOffsetTest ()
{
  ALuint Source;
  char ch, state;
  ALint lOffset;
  ALfloat flOffset;

  alGenSources (1, &Source);
  alSourcei (Source, AL_BUFFER, g_Buffers[6]);

  printf ("Get Source Offset Test\n");
  printf ("Press 1 to Play %s\n", FILENAME_STEREO);
  printf ("Press 2 to Use AL_BYTE_OFFSET (int) to track progress\n");
  printf ("Press 3 to Use AL_SAMPLE_OFFSET (int) to track progress\n");
  printf ("Press 4 to Use AL_SEC_OFFSET (int) to track progress\n");
  printf ("Press 5 to Use AL_BYTE_OFFSET (float) to track progress\n");
  printf ("Press 6 to Use AL_SAMPLE_OFFSET (float) to track progress\n");
  printf ("Press 7 to Use AL_SEC_OFFSET (float) to track progress\n");
  printf ("Press Q to quit\n");

  ch = '1';
  state = '2';

  while (1)
    {
      if (KBHIT ())
        {
          ch = getUpperCh ();

          if (ch == '1')
            {
              alSourcePlay (Source);
            }
          else if (ch == 'Q')
            {
              break;
            }

          if ((ch >= '2') && (ch <= '7'))
            {
              state = ch;
            }
        }
      else
        {
          switch (state)
            {
            case '2':
              alGetSourcei (Source, AL_BYTE_OFFSET, &lOffset);
              printf ("ByteOffset is %d                   \r", lOffset);
              break;
            case '3':
              alGetSourcei (Source, AL_SAMPLE_OFFSET, &lOffset);
              printf ("SampleOffset is %d                 \r", lOffset);
              break;
            case '4':
              alGetSourcei (Source, AL_SEC_OFFSET, &lOffset);
              printf ("SecondOffset is %d                 \r", lOffset);
              break;
            case '5':
              alGetSourcef (Source, AL_BYTE_OFFSET, &flOffset);
              printf ("ByteOffset is %.3f                 \r", flOffset);
              break;
            case '6':
              alGetSourcef (Source, AL_SAMPLE_OFFSET, &flOffset);
              printf ("SampleOffset is %.3f               \r", flOffset);
              break;
            case '7':
              alGetSourcef (Source, AL_SEC_OFFSET, &flOffset);
              printf ("SecondOffset is %.3f               \r", flOffset);
              break;
            }
        }
    }

  alSourceStop (Source);
  alDeleteSources (1, &Source);
}

/* Set Source Offset Test */
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Set Source Offset Test
*/
ALvoid
I_SetSourceOffsetTest ()
{
  ALuint Source;
  char ch;
  ALint lOffset = 0;
  ALfloat flOffset = 0.0f;
  ALint error;
  ALint iType = 0;

  alGenSources (1, &Source);
  alSourcei (Source, AL_BUFFER, g_Buffers[0]);

  printf ("Set Source Offset Test\n");
  printf ("Press 1 to Stop and Play 1 Footstep\n");
  printf ("Press 2 to Stop and Play 2 Footsteps\n");
  printf ("Press 3 to Stop and Play 3 Footsteps\n");
  printf ("Press 4 to Stop and Play 4 Footsteps\n");
  printf ("Press 5 to Stop and Play with bad offset\n");
  printf ("Press 6 to Set Offset to play 1 Footstep\n");
  printf ("Press 7 to Set Offset to play 2 Footsteps\n");
  printf ("Press 8 to Set Offset to play 3 Footsteps\n");
  printf ("Press 9 to Set Offset to play 4 Footsteps\n");
  printf ("Press 0 to Set Offset to bad offset\n");
  printf ("Press A to Play complete footstep sample\n");
  printf ("Press B to Play (Looping) complete footstep sample\n");
  printf ("Press C to Pause Source\n");
  printf ("Press D to Stop Source\n");
  printf ("Press E to Rewind Source\n");
  printf ("Press F to toggle Bytes, Samples, Seconds\n");
  printf ("Press Q to quit\n");

  while (1)
    {
      if (KBHIT ())
        {
          ch = getUpperCh ();

          switch (ch)
            {
            case '1':
              alSourceStop (Source);
              if (iType == 0)
                alSourcei (Source, AL_BYTE_OFFSET, 216000);
              else if (iType == 1)
                alSourcei (Source, AL_SAMPLE_OFFSET, 108000);
              else if (iType == 2)
                alSourcef (Source, AL_SEC_OFFSET, 2.440f);
              alSourcePlay (Source);
              break;

            case '2':
              alSourceStop (Source);
              if (iType == 0)
                alSourcei (Source, AL_BYTE_OFFSET, 144000);
              else if (iType == 1)
                alSourcei (Source, AL_SAMPLE_OFFSET, 72000);
              else if (iType == 2)
                alSourcef (Source, AL_SEC_OFFSET, 1.640f);
              alSourcePlay (Source);
              break;

            case '3':
              alSourceStop (Source);
              if (iType == 0)
                alSourcei (Source, AL_BYTE_OFFSET, 75000);
              else if (iType == 1)
                alSourcei (Source, AL_SAMPLE_OFFSET, 37500);
              else if (iType == 2)
                alSourcef (Source, AL_SEC_OFFSET, 0.840f);
              alSourcePlay (Source);
              break;

            case '4':
              alSourceStop (Source);
              if (iType == 0)
                alSourcei (Source, AL_BYTE_OFFSET, 1000);
              else if (iType == 1)
                alSourcei (Source, AL_SAMPLE_OFFSET, 500);
              else if (iType == 2)
                alSourcef (Source, AL_SEC_OFFSET, 0.015f);
              alSourcePlay (Source);
              break;

            case '5':
              alSourceStop (Source);
              if (iType == 0)
                alSourcei (Source, AL_BYTE_OFFSET, 400000);
              else if (iType == 1)
                alSourcei (Source, AL_SAMPLE_OFFSET, 200000);
              else if (iType == 2)
                alSourcef (Source, AL_SEC_OFFSET, 5.000f);
              alSourcePlay (Source);
              break;

            case '6':
              if (iType == 0)
                alSourcei (Source, AL_BYTE_OFFSET, 216000);
              else if (iType == 1)
                alSourcei (Source, AL_SAMPLE_OFFSET, 108000);
              else if (iType == 2)
                alSourcef (Source, AL_SEC_OFFSET, 2.440f);
              break;

            case '7':
              if (iType == 0)
                alSourcei (Source, AL_BYTE_OFFSET, 144000);
              else if (iType == 1)
                alSourcei (Source, AL_SAMPLE_OFFSET, 72000);
              else if (iType == 2)
                alSourcef (Source, AL_SEC_OFFSET, 1.640f);
              break;

            case '8':
              if (iType == 0)
                alSourcei (Source, AL_BYTE_OFFSET, 75000);
              else if (iType == 1)
                alSourcei (Source, AL_SAMPLE_OFFSET, 37500);
              else if (iType == 2)
                alSourcef (Source, AL_SEC_OFFSET, 0.840f);
              break;

            case '9':
              if (iType == 0)
                alSourcei (Source, AL_BYTE_OFFSET, 1000);
              else if (iType == 1)
                alSourcei (Source, AL_SAMPLE_OFFSET, 500);
              else if (iType == 2)
                alSourcef (Source, AL_SEC_OFFSET, 0.015f);
              break;

            case '0':
              if (iType == 0)
                alSourcei (Source, AL_BYTE_OFFSET, 400000);
              else if (iType == 1)
                alSourcei (Source, AL_SAMPLE_OFFSET, 200000);
              else if (iType == 2)
                alSourcef (Source, AL_SEC_OFFSET, 5.000f);
              break;

            case 'A':
              alSourcei (Source, AL_LOOPING, AL_FALSE);
              alSourcePlay (Source);
              break;

            case 'B':
              alSourcei (Source, AL_LOOPING, AL_TRUE);
              alSourcePlay (Source);
              break;

            case 'C':
              alSourcePause (Source);
              break;

            case 'D':
              alSourceStop (Source);
              break;

            case 'E':
              alSourceRewind (Source);
              break;

            case 'F':
              iType = (iType + 1) % 3;
              break;
            }

          if (ch == 'Q')
            break;
        }
      else
        {
          switch (iType)
            {
            case 0:
              alGetSourcei (Source, AL_BYTE_OFFSET, &lOffset);
              if ((error = alGetError ()) != AL_NO_ERROR)
                DisplayALError ("alGetSourcei AL_BYTE_OFFSET : ", error);
              else
                printf ("ByteOffset is %d                  \r", lOffset);
              break;

            case 1:
              alGetSourcei (Source, AL_SAMPLE_OFFSET, &lOffset);
              if ((error = alGetError ()) != AL_NO_ERROR)
                DisplayALError ("alGetSourcei AL_SAMPLE_OFFSET : ", error);
              else
                printf ("SampleOffset is %d                \r", lOffset);
              break;

            case 2:
              alGetSourcef (Source, AL_SEC_OFFSET, &flOffset);
              if ((error = alGetError ()) != AL_NO_ERROR)
                DisplayALError ("alGetSourcef AL_SEC_OFFSET : ", error);
              else
                printf ("Seconds Offset is %.3f            \r", flOffset);
              break;
            }
        }
    }

  alSourceStop (Source);
  alDeleteSources (1, &Source);
}

/* Distance Model Test */
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Distance Model Test
*/
ALvoid
I_DistanceModelTest ()
{
  ALuint source[1];
  ALint error;
  ALint lLoop, endLoop;

  alGenSources (1, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    {
      DisplayALError ("alGenSources 2 : ", error);
      return;
    }

  if (g_bNewDistModels)
    {
      endLoop = 6;
    }
  else
    {
      endLoop = 2;
    }

  alSourcei (source[0], AL_BUFFER, g_Buffers[0]);
  alSourcei (source[0], AL_LOOPING, AL_TRUE);
  alSourcePlay (source[0]);

  printf ("Distance Model Test\n");

  for (lLoop = 0; lLoop <= endLoop; lLoop++)
    {
      switch (lLoop)
        {
        case 0:
          printf ("Distance Model is AL_NONE\n");
          alDistanceModel (AL_NONE);
          break;
        case 1:
          printf ("Distance Model is AL_INVERSE_DISTANCE\n");
          alDistanceModel (AL_INVERSE_DISTANCE);
          break;
        case 2:
          printf ("Distance Model is AL_INVERSE_DISTANCE_CLAMPED\n");
          alDistanceModel (AL_INVERSE_DISTANCE_CLAMPED);
          break;
        case 3:
          printf ("Distance Model is AL_LINEAR_DISTANCE\n");
          alDistanceModel (AL_LINEAR_DISTANCE);
          break;
        case 4:
          printf ("Distance Model is AL_LINEAR_DISTANCE_CLAMPED\n");
          alDistanceModel (AL_LINEAR_DISTANCE_CLAMPED);
          break;
        case 5:
          printf ("Distance Model is AL_EXPONENT_DISTANCE\n");
          alDistanceModel (AL_EXPONENT_DISTANCE);
          break;
        case 6:
          printf ("Distance Model is AL_EXPONENT_DISTANCE_CLAMPED\n");
          alDistanceModel (AL_EXPONENT_DISTANCE_CLAMPED);
          break;
        }

      printf ("Distance is 16\n");
      alSource3f (source[0], AL_POSITION, 0.0f, 0.0f, -16.0f);
      printf ("Ref Distance is 8\n");
      alSourcef (source[0], AL_REFERENCE_DISTANCE, 8.0f);
      printf ("Max Distance is 32\n");
      alSourcef (source[0], AL_MAX_DISTANCE, 32.0f);

      sleepSeconds (2.0f);

      printf ("Distance is 64\n");
      alSource3f (source[0], AL_POSITION, 0.0f, 0.0f, -64.0f);
      printf ("Ref Distance is 8\n");
      alSourcef (source[0], AL_REFERENCE_DISTANCE, 8.0f);
      printf ("Max Distance is 32\n");
      alSourcef (source[0], AL_MAX_DISTANCE, 32.0f);

      sleepSeconds (2.0f);
    }

  printf ("Restoring INVERSE_DISTANCE_CLAMPED model\n");
  alDistanceModel (AL_INVERSE_DISTANCE_CLAMPED);
  sleepSeconds (0.05f);

  alSourceStopv (1, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alSourceStopv : ", error);

  alDeleteSources (1, source);
  if ((error = alGetError ()) != AL_NO_ERROR)
    DisplayALError ("alDeleteSources 1 : ", error);
}

/* Capture Test */
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Capture Test
*/
ALvoid
I_CaptureTest ()
{
  ALCdevice *pCaptureDevice;
  const ALCchar *szDefaultCaptureDevice;
  ALint lSamplesAvailable;
  ALchar ch;
  FILE *pFile;
  ALchar Buffer[4000];
  WAVEHEADER waveHeader;
  ALint lDataSize = 0;
  ALint lSize;

  /* NOTE : This code does NOT setup the Wave Device's Audio Mixer to select a recording input */
  /* or recording level. */

  /* Get list of available Capture Devices */
  const ALchar *pDeviceList =
    alcGetString (NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
  if (pDeviceList)
    {
      printf ("Available Capture Devices are:-\n");

      while (*pDeviceList)
        {
          printf ("%s\n", pDeviceList);
          pDeviceList += strlen (pDeviceList) + 1;
        }
    }

  szDefaultCaptureDevice =
    alcGetString (NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
  printf ("\nDefault Capture Device is '%s'\n\n", szDefaultCaptureDevice);

  pCaptureDevice =
    alcCaptureOpenDevice (szDefaultCaptureDevice, 22050, AL_FORMAT_STEREO16,
                          22050);
  if (pCaptureDevice)
    {
      printf ("Opened '%s' Capture Device\n\n",
              alcGetString (pCaptureDevice, ALC_CAPTURE_DEVICE_SPECIFIER));

      printf ("Press 1 to Start Recording\n");
      printf ("Press 2 to Stop Recording\n");
      printf ("Press Q to quit\n");

      pFile = fopen (FILENAME_RECORDING, "wb");

      sprintf (waveHeader.szRIFF, "RIFF");
      waveHeader.lRIFFSize = 0;
      sprintf (waveHeader.szWave, "WAVE");
      sprintf (waveHeader.szFmt, "fmt ");
      waveHeader.lFmtSize = sizeof (WAVEFORMATEX);
      waveHeader.wfex.nChannels = 2;
      waveHeader.wfex.wBitsPerSample = 16;
      waveHeader.wfex.wFormatTag = 1;   /* WAVE_FORMAT_PCM */
      waveHeader.wfex.nSamplesPerSec = 22050;
      waveHeader.wfex.nBlockAlign =
        waveHeader.wfex.nChannels * waveHeader.wfex.wBitsPerSample / 8;
      waveHeader.wfex.nAvgBytesPerSec =
        waveHeader.wfex.nSamplesPerSec * waveHeader.wfex.nBlockAlign;
      waveHeader.wfex.cbSize = 0;
      sprintf (waveHeader.szData, "data");
      waveHeader.lDataSize = 0;

      fwrite (&waveHeader, sizeof (WAVEHEADER), 1, pFile);

      while (1)
        {
          if (KBHIT ())
            {
              ch = getUpperCh ();

              switch (ch)
                {
                case '1':
                  alcCaptureStart (pCaptureDevice);
                  break;

                case '2':
                  alcCaptureStop (pCaptureDevice);
                  break;
                }

              if (ch == 'Q')
                {
                  alcCaptureStop (pCaptureDevice);
                  break;
                }
            }
          else
            {
              alcGetIntegerv (pCaptureDevice, ALC_CAPTURE_SAMPLES, 1,
                              &lSamplesAvailable);
              printf ("Samples available is %d\r", lSamplesAvailable);

              /* When we have enough data to fill our 4000 byte buffer, grab the samples */
              if (lSamplesAvailable > (4000 / waveHeader.wfex.nBlockAlign))
                {
                  /* Consume Samples */
                  alcCaptureSamples (pCaptureDevice, Buffer,
                                     4000 / waveHeader.wfex.nBlockAlign);

                  /* Write the audio data to a file */
                  fwrite (Buffer, 4000, 1, pFile);

                  /* Record total amount of data recorded */
                  lDataSize += 4000;
                }
            }
        }

      /* Check if any Samples haven't been captured yet */
      alcGetIntegerv (pCaptureDevice, ALC_CAPTURE_SAMPLES, 1,
                      &lSamplesAvailable);
      while (lSamplesAvailable)
        {
          if (lSamplesAvailable > (4000 / waveHeader.wfex.nBlockAlign))
            {
              alcCaptureSamples (pCaptureDevice, Buffer,
                                 4000 / waveHeader.wfex.nBlockAlign);
              fwrite (Buffer, 4000, 1, pFile);
              lSamplesAvailable -= (4000 / waveHeader.wfex.nBlockAlign);
              lDataSize += 4000;
            }
          else
            {
              alcCaptureSamples (pCaptureDevice, Buffer, lSamplesAvailable);
              fwrite (Buffer, lSamplesAvailable * waveHeader.wfex.nBlockAlign,
                      1, pFile);
              lSamplesAvailable = 0;
              lDataSize += lSamplesAvailable * waveHeader.wfex.nBlockAlign;
            }
        }

      /* Fill in Size information in Wave Header */
      fseek (pFile, 4, SEEK_SET);
      lSize = lDataSize + sizeof (WAVEHEADER) - 8;
      fwrite (&lSize, 4, 1, pFile);
      fseek (pFile, 42, SEEK_SET);
      fwrite (&lDataSize, 4, 1, pFile);

      fclose (pFile);

      alcCaptureCloseDevice (pCaptureDevice);
    }
}

/* Capture And Play Test */
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Capture And Play Test
*/
ALvoid
I_CaptureAndPlayTest ()
{
  ALCdevice *pCaptureDevice;
  const ALCchar *szDefaultCaptureDevice;
  ALint lSamplesAvailable;
  ALchar ch;
  ALchar Buffer[QUEUEBUFFERSIZE];
  ALuint SourceID, TempBufferID;
  ALuint BufferID[QUEUEBUFFERCOUNT];
  ALuint ulBuffersAvailable = QUEUEBUFFERCOUNT;
  ALuint ulUnqueueCount, ulQueueCount;
  ALint lLoop, lFormat, lFrequency, lBlockAlignment, lProcessed, lPlaying;
  ALboolean bPlaying = AL_FALSE;
  ALboolean bPlay = AL_FALSE;
  const ALchar *pDeviceList;
#if TEST_EAX
  ALint lEnv, lDirect, lRoom;
#endif

  /* NOTE : This code does NOT setup the Wave Device's Audio Mixer to select a recording input */
  /* or recording level. */

  /* Generate a Source and QUEUEBUFFERCOUNT Buffers for Queuing */
  alGetError ();
  alGenSources (1, &SourceID);

  for (lLoop = 0; lLoop < QUEUEBUFFERCOUNT; lLoop++)
    alGenBuffers (1, &BufferID[lLoop]);

  if (alGetError () != AL_NO_ERROR)
    {
      printf ("Failed to generate Source and / or Buffers\n");
      return;
    }

  ulUnqueueCount = 0;
  ulQueueCount = 0;

  /* Get list of available Capture Devices */
  pDeviceList = alcGetString (NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
  if (pDeviceList)
    {
      printf ("Available Capture Devices are:-\n");

      while (*pDeviceList)
        {
          printf ("%s\n", pDeviceList);
          pDeviceList += strlen (pDeviceList) + 1;
        }
    }

  szDefaultCaptureDevice =
    alcGetString (NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
  printf ("\nDefault Capture Device is '%s'\n\n", szDefaultCaptureDevice);

  /* The next call can fail if the WaveDevice does not support the requested format, so the application */
  /* should be prepared to try different formats in case of failure */

  lFormat = AL_FORMAT_MONO16;
  lFrequency = 22050;
  lBlockAlignment = 2;

  pCaptureDevice =
    alcCaptureOpenDevice (szDefaultCaptureDevice, lFrequency, lFormat,
                          lFrequency);
  if (pCaptureDevice)
    {
      printf ("Opened '%s' Capture Device\n\n",
              alcGetString (pCaptureDevice, ALC_CAPTURE_DEVICE_SPECIFIER));

      printf ("Press 1 to Start Recording and Playing\n");
      printf ("Press 2 to Stop Recording\n");
#if TEST_EAX
      printf ("Press 3 to Apply EAX Reverb\n");
      printf ("Press 4 to Remove EAX Reverb\n");
#endif
      printf ("Press Q to quit\n");

      while (1)
        {
          if (KBHIT ())
            {
              ch = getUpperCh ();

              switch (ch)
                {
                case '1':
                  alcCaptureStart (pCaptureDevice);
                  bPlay = AL_TRUE;
                  break;

                case '2':
                  alcCaptureStop (pCaptureDevice);
                  break;
#if TEST_EAX
                case '3':
                  /* Mute Direct path */
                  lDirect = -10000;
                  eaxSet (&DSPROPSETID_EAX_BufferProperties,
                          DSPROPERTY_EAXBUFFER_DIRECT, 0, &lDirect,
                          sizeof (ALint));

                  /* Apply a Reverb Preset */
                  lEnv = EAX_ENVIRONMENT_HANGAR;
                  eaxSet (&DSPROPSETID_EAX_ListenerProperties,
                          DSPROPERTY_EAXLISTENER_ENVIRONMENT, 0, &lEnv,
                          sizeof (ALint));

                  lRoom = 0;
                  eaxSet (&DSPROPSETID_EAX_ListenerProperties,
                          DSPROPERTY_EAXLISTENER_ROOM, 0, &lRoom,
                          sizeof (ALint));
                  break;

                case '4':
                  /* Un-mute Direct path */
                  lDirect = 0;
                  eaxSet (&DSPROPSETID_EAX_BufferProperties,
                          DSPROPERTY_EAXBUFFER_DIRECT, 0, &lDirect,
                          sizeof (ALint));

                  /* Mute Reverb Preset */
                  lRoom = -10000;
                  eaxSet (&DSPROPSETID_EAX_ListenerProperties,
                          DSPROPERTY_EAXLISTENER_ROOM, 0, &lRoom,
                          sizeof (ALint));
                  break;
#endif
                }

              if (ch == 'Q')
                {
                  alcCaptureStop (pCaptureDevice);
                  break;
                }
            }
          else
            {
              alcGetIntegerv (pCaptureDevice, ALC_CAPTURE_SAMPLES, 1,
                              &lSamplesAvailable);
              printf ("Samples available is %d\r", lSamplesAvailable);

              /* If the Source is (or should be) playing, get number of buffers processed */
              /* and check play status */
              if (bPlaying)
                {
                  alGetSourcei (SourceID, AL_BUFFERS_PROCESSED, &lProcessed);
                  while (lProcessed)
                    {
                      /* Unqueue the buffer */
                      alSourceUnqueueBuffers (SourceID, 1, &TempBufferID);

                      /* Update unqueue count */
                      if (++ulUnqueueCount == QUEUEBUFFERCOUNT)
                        ulUnqueueCount = 0;

                      /* Increment buffers available */
                      ulBuffersAvailable++;

                      lProcessed--;
                    }

                  /* If the Source has stopped (been starved of data) it will need to be */
                  /* restarted */
                  alGetSourcei (SourceID, AL_SOURCE_STATE, &lPlaying);
                  if (lPlaying == AL_STOPPED)
                    bPlay = AL_TRUE;
                }

              /* When we have enough data to fill our QUEUEBUFFERSIZE byte buffer, grab the samples */
              if ((lSamplesAvailable > (QUEUEBUFFERSIZE / lBlockAlignment))
                  && (ulBuffersAvailable))
                {
                  /* Consume Samples */
                  alcCaptureSamples (pCaptureDevice, Buffer,
                                     QUEUEBUFFERSIZE / lBlockAlignment);

                  alBufferData (BufferID[ulQueueCount], lFormat, Buffer,
                                QUEUEBUFFERSIZE, lFrequency);

                  /* Queue the buffer, and mark buffer as queued */
                  alSourceQueueBuffers (SourceID, 1, &BufferID[ulQueueCount]);

                  if (++ulQueueCount == QUEUEBUFFERCOUNT)
                    ulQueueCount = 0;

                  /* Decrement buffers available */
                  ulBuffersAvailable--;

                  /* If we need to start the Source do it now IF AND ONLY IF we have queued at least 2 buffers */
                  if ((bPlay)
                      && (ulBuffersAvailable <= (QUEUEBUFFERCOUNT - 2)))
                    {
                      alSourcePlay (SourceID);
                      bPlaying = AL_TRUE;
                      bPlay = AL_FALSE;
                    }
                }
            }
        }

      alcCaptureCloseDevice (pCaptureDevice);
    }
  else
    {
      printf
        ("WaveDevice is unavailable, or does not supported the request format\n");
    }

  alSourceStop (SourceID);
  alDeleteSources (1, &SourceID);
  for (lLoop = 0; lLoop < QUEUEBUFFERCOUNT; lLoop++)
    alDeleteBuffers (1, &BufferID[lLoop]);
}

/* Vorbis Test */
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Vorbis Test
This test exercises Ogg Vorbis playback functionality.
*/
ALvoid
I_VorbisTest ()
{
  ALint error;
  ALbyte *data;
  ALuint source[3];
  ALuint tempBuffers[4];
  ALbyte ch;
  ALboolean bLoop = AL_FALSE;

  alGetError ();                /* reset error state */

  if (alIsExtensionPresent ((ALchar *) "AL_EXT_vorbis") == AL_TRUE)
    {
      alGenSources (3, source);
      if ((error = alGetError ()) != AL_NO_ERROR)
        {
          DisplayALError ((ALbyte *) "alGenSources 3 : ", error);
          return;
        }

      /* create buffers for queueing */
      alGenBuffers (4, tempBuffers);

      printf ("Vorbis Test\n");
      printf ("Press '1' to play an Ogg Vorbis buffer on source 0\n");
      printf ("Press '2' to toggle looping on / off for source 0\n");
      printf ("Press '3' to play an Ogg Vorbis buffer on source 1\n");
      printf ("Press '4' to stream an Ogg Vorbis buffer on source 2\n");
      printf ("Press 'q' to quit\n");
      printf ("Looping is off\n");

      do
        {
          ch = getUpperCh ();

          switch (ch)
            {
            case '1':
              /* Stop source */
              alSourceStop (source[0]);
              if ((error = alGetError ()) != AL_NO_ERROR)
                DisplayALError ((ALbyte *) "alSourceStop source 0 : ", error);

              /* Attach new buffer */
              alSourcei (source[0], AL_BUFFER, g_Buffers[7]);
              if ((error = alGetError ()) != AL_NO_ERROR)
                DisplayALError ((ALbyte *)
                                "alSourcei 0 AL_BUFFER buffer 7 : \n", error);

              /* Set volume */
              alSourcef (source[0], AL_GAIN, 0.5f);
              if ((error = alGetError ()) != AL_NO_ERROR)
                DisplayALError ((ALbyte *) "alSourcef 0 AL_GAIN : \n", error);

              /* Set looping */
              alSourcei (source[0], AL_LOOPING, bLoop);
              if ((error = alGetError ()) != AL_NO_ERROR)
                DisplayALError ((ALbyte *)
                                "alSourcei 0 AL_LOOPING AL_TRUE: \n", error);

              /* Play source */
              alSourcePlay (source[0]);
              if ((error = alGetError ()) != AL_NO_ERROR)
                DisplayALError ((ALbyte *) "alSourcePlay source 0 : ", error);

              break;
            case '2':
              if (bLoop)
                {
                  printf ("Looping is off\n");
                  bLoop = AL_FALSE;
                }
              else
                {
                  printf ("Looping is on  \n");
                  bLoop = AL_TRUE;
                }
              alSourcei (source[0], AL_LOOPING, bLoop);
              if ((error = alGetError ()) != AL_NO_ERROR)
                DisplayALError ((ALbyte *) "alSourcei 0 AL_LOOPING : \n",
                                error);
              break;
            case '3':
              /* Stop source */
              alSourceStop (source[1]);
              if ((error = alGetError ()) != AL_NO_ERROR)
                DisplayALError ((ALbyte *) "alSourceStop source 1 : ", error);

              /* Attach new buffer */
              alSourcei (source[1], AL_BUFFER, g_Buffers[7]);
              if ((error = alGetError ()) != AL_NO_ERROR)
                DisplayALError ((ALbyte *)
                                "alSourcei 1 AL_BUFFER buffer 7 : \n", error);

              /* Set volume */
              alSourcef (source[1], AL_GAIN, 0.5f);
              if ((error = alGetError ()) != AL_NO_ERROR)
                DisplayALError ((ALbyte *) "alSourcef 1 AL_GAIN : \n", error);

              /* Play source */
              alSourcePlay (source[1]);
              if ((error = alGetError ()) != AL_NO_ERROR)
                DisplayALError ((ALbyte *) "alSourcePlay source 1 : ", error);

              break;
            case '4':
              {
                FILE *fp;
                int actual;
                int loop;
                /* Stop source */
                alSourceStop (source[2]);
                if ((error = alGetError ()) != AL_NO_ERROR)
                  DisplayALError ((ALbyte *) "alSourceStop source 2 : ",
                                  error);
                alSourcei (source[2], AL_BUFFER, 0);

                /* Queue buffers */
                fp = fopen (FILENAME_BOOM, "rb");
                if (fp == NULL)
                  {
                    printf ("Failed to open %s\n", FILENAME_BOOM);
                    break;
                  }

                alGetError ();  /* clear error state */

                data = (ALbyte *) malloc (((g_ovSize / 4) + 1));
                if (data != 0)
                  {
                    ALenum formatVorbis =
                      alGetEnumValue ("AL_FORMAT_VORBIS_EXT");
                    for (loop = 0; loop < 4; loop++)
                      {
                        actual = fread (data, 1, ((g_ovSize / 4) + 1), fp);
                        alBufferData (tempBuffers[loop], formatVorbis, data,
                                      actual, 0);
                      }
                    alSourceQueueBuffers (source[2], 4, tempBuffers);

                    fclose (fp);

                    /* Set volume */
                    alSourcef (source[2], AL_GAIN, 0.5f);
                    if ((error = alGetError ()) != AL_NO_ERROR)
                      DisplayALError ((ALbyte *) "alSourcef 2 AL_GAIN : \n",
                                      error);

                    /* Play source */
                    alSourcePlay (source[2]);
                    if ((error = alGetError ()) != AL_NO_ERROR)
                      DisplayALError ((ALbyte *) "alSourcePlay source 2 : ",
                                      error);

                    free ((void *) data);
                  }
                else
                  {
                    printf ("Failed memory allocation.\n");
                  }
              }
              break;
            }
        }
      while (ch != 'Q');

      /* Release resources */
      alSourceStop (source[0]);
      alSourceStop (source[1]);
      alSourceStop (source[2]);
      alDeleteBuffers (4, tempBuffers);
      if ((error = alGetError ()) != AL_NO_ERROR)
        DisplayALError ((ALbyte *) "alSourceStop : ", error);

      alDeleteSources (3, source);
      if ((error = alGetError ()) != AL_NO_ERROR)
        DisplayALError ((ALbyte *) "alDeleteSources 3 : ", error);
    }
  else
    {                           /* Ogg Vorbis extension not present */
      printf ("\nOgg Vorbis extension not available.\n");
    }
}
