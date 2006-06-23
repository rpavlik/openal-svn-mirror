/*
 * null output. Context writes, we sleep.
*/
#include "al_siteconfig.h"
#include "backends/alc_backend.h"
#include <stdlib.h>

#ifndef USE_BACKEND_NULL

void
alcBackendOpenNull_ (UNUSED(ALC_OpenMode mode), UNUSED(ALC_BackendOps **ops),
		     ALC_BackendPrivateData **privateData)
{
	*privateData = NULL;
}

#else

#include <AL/al.h>

#include "al_main.h"
#include "al_debug.h"


static void *bogus_handle = (void *) 0x4ABAD1;
static ALuint sleep_usec (ALuint speed, ALuint chunk);
static ALint nullspeed;
static ALC_OpenMode nullMode;

static void *
grab_write_null (void)
{
  nullMode = ALC_OPEN_OUTPUT_;
  return bogus_handle;
}

static void *
grab_read_null (void)
{
  nullMode = ALC_OPEN_INPUT_;
  return NULL;
}

static ALboolean
set_write_null (UNUSED (void *handle), UNUSED (ALuint *deviceBufferSizeInBytes),
                UNUSED (ALenum *fmt), ALuint *speed)
{
  nullspeed = *speed;
  return AL_TRUE;
}

static ALboolean
set_read_null (UNUSED (void *handle), UNUSED (ALuint *deviceBufferSizeInBytes),
               UNUSED (ALenum *fmt), UNUSED (ALuint *speed))
{

  return AL_TRUE;
}

static ALboolean
alcBackendSetAttributesNull_ (void *handle, ALuint *deviceBufferSizeInBytes,
                              ALenum *fmt, ALuint *speed)
{
  return nullMode == ALC_OPEN_INPUT_ ?
    set_read_null (handle, deviceBufferSizeInBytes, fmt, speed) :
    set_write_null (handle, deviceBufferSizeInBytes, fmt, speed);
}

static void
null_blitbuffer (UNUSED (void *handle),
                 UNUSED (const void *dataptr), int bytesToWrite)
{
  _alMicroSleep (sleep_usec (nullspeed, bytesToWrite));
}

static void
release_null (UNUSED (void *handle))
{
}

static ALuint
sleep_usec (ALuint speed, ALuint chunk)
{
  return 1000000.0 * chunk / speed;
}

static void
pause_null (UNUSED (void *handle))
{
}

static void
resume_null (UNUSED (void *handle))
{
}

static ALsizei
capture_null (UNUSED (void *handle), UNUSED (void *capture_buffer),
              UNUSED (int bytesToRead))
{
  return 0;
}

static ALfloat
get_nullchannel (UNUSED (void *handle), UNUSED (ALuint channel))
{
  return 0.0;
}

static int
set_nullchannel (UNUSED (void *handle), UNUSED (ALuint channel),
                 UNUSED (ALfloat volume))
{
  return 0;
}

static ALC_BackendOps nullOps = {
	release_null,
	pause_null,
	resume_null,
	alcBackendSetAttributesNull_,
	null_blitbuffer,
	capture_null,
	get_nullchannel,
	set_nullchannel
};

void
alcBackendOpenNull_ (ALC_OpenMode mode, ALC_BackendOps **ops, ALC_BackendPrivateData **privateData)
{
  *privateData =  (mode == ALC_OPEN_INPUT_) ? grab_read_null () : grab_write_null ();
  if (*privateData != NULL) {
    *ops = &nullOps;
  }
}

#endif /* USE_BACKEND_NULL */
