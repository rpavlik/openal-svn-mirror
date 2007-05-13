/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * alc_error.c
 *
 * openal error reporting.
 *
 */
#include "al_siteconfig.h"

#include <AL/al.h>
#include <AL/alc.h>

#include "al_debug.h"
#include "al_main.h"

#include "alc/alc_error.h"

/*
 * alcErrorIndex is a simple index referring to an error.
 */
static ALCenum lastError = ALC_NO_ERROR;

/**
 * Error support.
 */

/*
 * alcGetError( ALCdevice *dev )
 *
 * Returns the most recent error generated in the AL state machine,
 * but for alc.
 */
ALCenum alcGetError( UNUSED(ALCdevice *dev) )
{
	ALCenum retval = lastError;
	/*
	 * In deference to the new spec, GetError clears the error
	 * after reading it.
	 */
	lastError = ALC_NO_ERROR;

	return retval;
}

/*
 * _alcSetError( ALenum param )
 *
 * Sets the alc error, if unset.
 */
void _alcSetError( ALenum param ) {
	if( lastError == ALC_NO_ERROR ) {
		/* Only set error if no previous error has been recorded. */
		lastError = param;
	}
}
