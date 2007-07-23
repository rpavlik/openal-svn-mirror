/*
 * MMSYSTEM backend for Windows
 */

#include "al_siteconfig.h"
#include "backends/alc_backend.h"
#include <stdlib.h>

#ifndef USE_BACKEND_NATIVE_WINDOWS

void
alcBackendOpenNative_ (UNUSED (ALC_OpenMode mode),
                       UNUSED (ALC_BackendOps **ops),
                       struct ALC_BackendPrivateData **privateData)
{
  *privateData = NULL;
}

#else

#include <AL/al.h>
#include <windows.h>

#include "al_main.h"
#include "al_debug.h"
#include "alc/alc_context.h"
#include "al_mutexlib.h"

#define DEF_BITSPERSAMPLE 16
#define DEF_SPEED         44100
#define DEF_FORMAT        WAVE_FORMAT_PCM
#define DEF_CHANNELS      2
#define DEF_CBSIZE        0

#define MAX_AUDIOBUFS     8
#define MAX_PCMDATA       (DEF_SPEED * DEF_CHANNELS)

static MutexID mutex;

static struct
{
  WAVEHDR whdrs[MAX_AUDIOBUFS];
  int index;
  int freecount;
} audiobufs;

static struct
{
  HWAVEOUT hwo;
  WAVEFORMATEX pwfx;
} WinAudioHandle;

static char pcmdata[MAX_PCMDATA];
static int scalesize = 0;
static CRITICAL_SECTION waveCriticalSection;

static void CALLBACK
WinFillAudio (HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1,
              DWORD dwParam2)
{
  int *freeBlockCounter = (int *) dwInstance;

  /* Only service "buffer done playing" messages */
  if (uMsg != WOM_DONE)
    {
      return;
    }
  /* _alUnlockMutex(mutex); not sure if we need this */

  EnterCriticalSection (&waveCriticalSection);
  (*freeBlockCounter)++;
  LeaveCriticalSection (&waveCriticalSection);
}

/*
 * FIXME: unprepare whdrs
 */
static void
closeMMSYSTEM (struct ALC_BackendPrivateData *privateData)
{
  MMRESULT err;
  WAVEHDR *whdr;
  HWAVEOUT hwo;
  int i;
  char errmsg[256];

  _alDestroyMutex (mutex);
  mutex = NULL;
  DeleteCriticalSection (&waveCriticalSection);

  hwo = WinAudioHandle.hwo;
  whdr = &audiobufs.whdrs[audiobufs.index];
  while (audiobufs.freecount < MAX_AUDIOBUFS)
    {
      _alMicroSleep (10);
    }

  for (i = 0; i < MAX_AUDIOBUFS; i++)
    {
      /* not first time */
      if (audiobufs.whdrs[i].dwFlags & WHDR_PREPARED)
        waveOutUnprepareHeader (hwo, &audiobufs.whdrs[i], sizeof (WAVEHDR));
    }

  err = waveOutReset (hwo);
  if (err != MMSYSERR_NOERROR)
    {
      waveOutGetErrorText (err, errmsg, sizeof (errmsg));
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "waveOutReset failed: %s",
                errmsg);
    }

  err = waveOutClose (hwo);
  if (err != MMSYSERR_NOERROR)
    {
      waveOutGetErrorText (err, errmsg, sizeof (errmsg));
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "waveOutClose failed: %s",
                errmsg);
    }
}

static void
pauseMMSYSTEM (UNUSED (struct ALC_BackendPrivateData *privateData))
{
  return;
}

static void
resumeMMSYSTEM (UNUSED (struct ALC_BackendPrivateData *privateData))
{
  return;
}

static ALboolean
setAttributesMMSYSTEM (UNUSED (struct ALC_BackendPrivateData *privateData),
                       UNUSED (unsigned int *bufsiz), UNUSED (ALenum *fmt),
                       UNUSED (unsigned int *speed))
{
  MMRESULT err;
  LPWAVEFORMATEX pwfx = &WinAudioHandle.pwfx;
  HWAVEOUT hwo;
  ALuint channels = _alGetChannelsFromFormat (*fmt);

  memset (pwfx, 0, sizeof *pwfx);

  pwfx->wFormatTag = DEF_FORMAT;
  pwfx->nChannels = channels;
  pwfx->wBitsPerSample = _alGetBitsFromFormat (*fmt);
  pwfx->nBlockAlign = pwfx->nChannels * (pwfx->wBitsPerSample / 8);
  pwfx->nSamplesPerSec = *speed;
  pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
  pwfx->cbSize = DEF_CBSIZE;

  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
            "chans %d bps %d bal %d sps %d abs %d cbs %d\n", pwfx->nChannels,
            pwfx->wBitsPerSample, pwfx->nBlockAlign,
            (int) pwfx->nSamplesPerSec, (int) pwfx->nAvgBytesPerSec,
            (int) pwfx->cbSize);

  err = waveOutOpen (&WinAudioHandle.hwo, WAVE_MAPPER, pwfx,
                     (DWORD_PTR) WinFillAudio,
                     (DWORD_PTR) & audiobufs.freecount, CALLBACK_FUNCTION);
  if (err != MMSYSERR_NOERROR)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "waveOutOpen failed: %d",
                err);
      return AL_FALSE;
    }
  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "setAttributesMMSYSTEM OK");

  hwo = WinAudioHandle.hwo;
  scalesize = *bufsiz;
  return AL_TRUE;
}

static void
writeMMSYSTEM (struct ALC_BackendPrivateData *privateData, const void *data,
               int bytesToWrite)
{
  WAVEHDR *whdr;
  MMRESULT err;
  HWAVEOUT hwo;
  char *bufptr;
  char errmsg[256];

  hwo = WinAudioHandle.hwo;

  _alLockMutex (mutex);

  whdr = &audiobufs.whdrs[audiobufs.index];

  if (whdr->dwFlags & WHDR_PREPARED)
    waveOutUnprepareHeader (hwo, whdr, sizeof (WAVEHDR));

  bufptr = pcmdata + audiobufs.index * scalesize;
  memcpy (bufptr, data, bytesToWrite);
  whdr->lpData = (LPSTR) bufptr;
  whdr->dwBufferLength = bytesToWrite;
  whdr->dwFlags = 0;

  err = waveOutPrepareHeader (hwo, whdr, sizeof (WAVEHDR));
  if (err != MMSYSERR_NOERROR)
    {
      waveOutGetErrorText (err, errmsg, sizeof (errmsg));
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "waveOutPrepareHeader failed: %s", errmsg);
    }

  err = waveOutWrite (hwo, whdr, sizeof (WAVEHDR));
  if (err != MMSYSERR_NOERROR)
    {
      waveOutGetErrorText (err, errmsg, sizeof (errmsg));
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "waveOutWrite failed: %s",
                errmsg);
    }
  EnterCriticalSection (&waveCriticalSection);
  audiobufs.freecount--;
  LeaveCriticalSection (&waveCriticalSection);
  while (!audiobufs.freecount)
    _alMicroSleep (10);

  audiobufs.index = (audiobufs.index + 1) % MAX_AUDIOBUFS;

  _alUnlockMutex (mutex);
}

static ALsizei
readMMSYSTEM (UNUSED (struct ALC_BackendPrivateData *privateData),
              UNUSED (void *data), UNUSED (int bytesToRead))
{
  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "should never happen");
  return 0;
}
static ALfloat
getAudioChannelMMSYSTEM (UNUSED (struct ALC_BackendPrivateData *privateData),
                         UNUSED (ALuint channel))
{
  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
            "MMSYSTEM backend does not support getting the volume");
  return -1.0f;
}

static int
setAudioChannelMMSYSTEM (UNUSED (struct ALC_BackendPrivateData *privateData),
                         UNUSED (ALuint channel), UNUSED (ALfloat volume))
{
  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
            "MMSYSTEM backend does not support setting the volume");
  return -1;
}

static const ALCchar *getNameMMSYSTEM(struct ALC_BackendPrivateData *privateData)
{
    return "Windows MMSystem Native";
}

static ALC_BackendOps nativeOps = {
  closeMMSYSTEM,
  pauseMMSYSTEM,
  resumeMMSYSTEM,
  setAttributesMMSYSTEM,
  writeMMSYSTEM,
  readMMSYSTEM,
  getAudioChannelMMSYSTEM,
  setAudioChannelMMSYSTEM,
  getNameMMSYSTEM
};

static void *
grab_write_native (void)
{
  MMRESULT err;
  LPWAVEFORMATEX pwfx = &WinAudioHandle.pwfx;
  int i;

  audiobufs.index = 0;
  audiobufs.freecount = MAX_AUDIOBUFS;
  mutex = _alCreateMutex ();

  for (i = 0; i < MAX_AUDIOBUFS; i++)
    {
      audiobufs.whdrs[i].lpData = NULL;
      audiobufs.whdrs[i].dwBufferLength = 0;
      audiobufs.whdrs[i].dwFlags = 0;   /* WHDR_DONE */
    }

  memset (pwfx, 0, sizeof *pwfx);

  pwfx->wFormatTag = DEF_FORMAT;
  pwfx->nChannels = DEF_CHANNELS;
  pwfx->wBitsPerSample = DEF_BITSPERSAMPLE;
  pwfx->nBlockAlign = DEF_CHANNELS * (pwfx->wBitsPerSample / 8);
  pwfx->nSamplesPerSec = DEF_SPEED;
  pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
  pwfx->cbSize = DEF_CBSIZE;

  InitializeCriticalSection (&waveCriticalSection);

  err = waveOutOpen (&WinAudioHandle.hwo, WAVE_MAPPER, pwfx,
                     (DWORD_PTR) WinFillAudio,
                     (DWORD_PTR) & audiobufs.freecount, CALLBACK_FUNCTION);
  if (err != MMSYSERR_NOERROR)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "waveOutOpen failed: %d",
                err);
      return NULL;
    }

  _alDebug (ALD_MAXIMUS, __FILE__, __LINE__, "MMSYSTEM OK");
  return &WinAudioHandle.hwo;
}

void
alcBackendOpenNative_ (ALC_OpenMode mode, ALC_BackendOps **ops,
                       struct ALC_BackendPrivateData **privateData)
{
  if (mode == ALC_OPEN_INPUT_)
    {
      _alDebug (ALD_MAXIMUS, __FILE__, __LINE__,
                "MMSYSTEM backend does not support input");
      *privateData = NULL;
      return;
    }

  *privateData = grab_write_native ();
  if (*privateData != NULL)
    {
      *ops = &nativeOps;
    }
}

#endif /* USE_BACKEND_NATIVE_WINDOWS */
