/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_able.c
 *
 * Handles Enable / Disable stuff.
 */
#include "al_siteconfig.h"

#include <AL/al.h>

#include "al_error.h"

/*
 * alEnable( ALenum param )
 *
 * Enables param if possible for the current context.  If param does not
 * specify a valid enabling token, AL_INVALID_ENUM is set.
 */
void alEnable( ALenum param )
{
	if (alcGetCurrentContext() == NULL) {
		_alDCSetError( AL_INVALID_OPERATION );
	}

	_alcDCLockContext();
	/* ToDo: We should somehow hook in the extension mechanism here */
	switch (param) {
	default:
		_alDCSetError( AL_INVALID_ENUM );
		break;
	}
	_alcDCUnlockContext();
}

/*
 * alDisable( ALenum param )
 *
 * Disables param if possible for the current context.  If param does not
 * specify a valid enabling token, AL_INVALID_ENUM is set.
 */
void alDisable( ALenum param )
{
	if (alcGetCurrentContext() == NULL) {
		_alDCSetError( AL_INVALID_OPERATION );
	}

	_alcDCLockContext();
	/* ToDo: We should somehow hook in the extension mechanism here */
	switch (param) {
	default:
		_alDCSetError( AL_INVALID_ENUM );
		break;
	}
	_alcDCUnlockContext();
}

/*
 * alIsEnabled( ALenum param )
 *
 * returns AL_TRUE if the attribute specified by param is enabled, AL_FALSE
 * otherwise.
 *
 * if param is not a valid enable/disable token, AL_INVALID_ENUM is set.
 */
ALboolean alIsEnabled( ALenum param )
{
	if (alcGetCurrentContext() == NULL) {
		_alDCSetError( AL_INVALID_OPERATION );
	}

	_alcDCLockContext();
	/* ToDo: We should somehow hook in the extension mechanism here */
	switch (param) {
	default:
		_alDCSetError( AL_INVALID_ENUM );
		break;
	}
	_alcDCUnlockContext();
	return AL_FALSE;
}
