/*
 * null output. Context writes, we sleep.
*/
#include "al_siteconfig.h"

#include <AL/al.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "al_main.h"
#include "al_debug.h"

#include "backends/alc_backend.h"

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

void *
alcBackendOpenNull_ (ALC_OpenMode mode)
{
  return mode == ALC_OPEN_INPUT_ ? grab_read_null () : grab_write_null ();
}

static ALboolean
set_write_null (UNUSED (void *handle), UNUSED (ALuint *bufsiz),
                UNUSED (ALenum *fmt), ALuint *speed)
{
  nullspeed = *speed;
  return AL_TRUE;
}

static ALboolean
set_read_null (UNUSED (void *handle), UNUSED (ALuint *bufsiz),
               UNUSED (ALenum *fmt), UNUSED (ALuint *speed))
{

  return AL_TRUE;
}

ALboolean
alcBackendSetAttributesNull_ (void *handle, ALuint *bufsiz,
                              ALenum *fmt, ALuint *speed)
{
  return nullMode == ALC_OPEN_INPUT_ ?
    set_read_null (handle, bufsiz, fmt, speed) :
    set_write_null (handle, bufsiz, fmt, speed);
}

void
null_blitbuffer (UNUSED (void *handle),
                 UNUSED (const void *dataptr), int bytes_to_write)
{
  _alMicroSleep (sleep_usec (nullspeed, bytes_to_write));
}

void
release_null (UNUSED (void *handle))
{
}

static ALuint
sleep_usec (ALuint speed, ALuint chunk)
{
  return 1000000.0 * chunk / speed;
}

void
pause_null (UNUSED (void *handle))
{
}

void
resume_null (UNUSED (void *handle))
{
}

ALsizei
capture_null (UNUSED (void *handle), UNUSED (void *capture_buffer),
              UNUSED (int bufsiz))
{
  return 0;
}

ALfloat
get_nullchannel (UNUSED (void *handle), UNUSED (ALuint channel))
{
  return 0.0;
}

int
set_nullchannel (UNUSED (void *handle), UNUSED (ALuint channel),
                 UNUSED (ALfloat volume))
{
  return 0;
}
