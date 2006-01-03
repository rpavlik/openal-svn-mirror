/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * null.c
 *
 * null output.  Context writes, we sleep.
 *
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

#include "arch/interface/interface_sound.h"

static void *bogus_handle = (void *) 0x4ABAD1;
static ALuint sleep_usec(ALuint speed, ALuint chunk);
static ALint nullspeed;

void *grab_write_null(void) {
	return bogus_handle;
}

void *grab_read_null(void) {
	return NULL;
}

ALboolean set_write_null(UNUSED(void *handle),
		  UNUSED(ALuint *bufsiz),
		  UNUSED(ALenum *fmt),
		  ALuint *speed) {

	nullspeed = *speed;

        return AL_TRUE;
}

ALboolean set_read_null(UNUSED(void *handle),
		  UNUSED(ALuint *bufsiz),
		  UNUSED(ALenum *fmt),
		  UNUSED(ALuint *speed)) {

        return AL_TRUE;
}

void null_blitbuffer(UNUSED(void *handle),
		     UNUSED(void *dataptr),
		     int bytes_to_write)  {
	_alMicroSleep(sleep_usec(nullspeed, bytes_to_write));

        return;
}

/*
 *  close file, free data
 */
void release_null(UNUSED(void *handle)) {
	return;
}

static ALuint sleep_usec(ALuint speed, ALuint chunk) {
	ALuint retval;

	retval = 1000000.0 * chunk / speed;

	return retval;
}

void
pause_null( UNUSED(void *handle) )
{
}

void
resume_null( UNUSED(void *handle) )
{
}

ALsizei
capture_null( UNUSED(void *handle), UNUSED(void *capture_buffer), UNUSED(int bufsiz) )
{
	return 0;
}

ALfloat
get_nullchannel( UNUSED(void *handle), UNUSED(ALuint channel) )
{
	return 0.0;
}

int
set_nullchannel( UNUSED(void *handle), UNUSED(ALuint channel), UNUSED(ALfloat volume) )
{
	return 0;
}
