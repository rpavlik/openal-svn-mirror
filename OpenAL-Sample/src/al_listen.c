
/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_listen.c
 *
 * Functions related to management and use of listeners.
 */
#include "al_siteconfig.h"

#include <AL/al.h>
#include <AL/alext.h>
#include <stdlib.h>

#include "al_error.h"
#include "al_listen.h"
#include "al_main.h"
#include "alc/alc_speaker.h"

#define MAX_LISTENER_NUM_VALUES 6

static ALint
numValuesForAttribute( ALenum param )
{
	switch (param) {
	case AL_POSITION:
	case AL_VELOCITY:
		return 3;
	case AL_GAIN:
	case AL_GAIN_LINEAR_LOKI:
		return 1;
	case AL_ORIENTATION:
		return 6;
		break;
	default:
		return 0;
	}
}

static void
setListenerAttributef( ALenum param, const ALfloat *values, ALint numValues)
{
	/* TODO: As usual, we should better have a getter for a *locked* context */
	AL_context *cc = _alcDCGetContext();
	if( cc == NULL ) {
		_alDCSetError(AL_INVALID_OPERATION);
		return;
	}
	_alcDCLockContext();

	if (numValues != numValuesForAttribute(param)) {
		_alDCSetError(AL_INVALID_ENUM);
		_alcDCUnlockContext();
		return;
	}
	if( values == NULL ) {
		_alDCSetError(AL_INVALID_VALUE);
		_alcDCUnlockContext();
		return;
	}

	switch( param ) {
	case AL_POSITION:
		if ((cc->listener.position[0] == values[0]) &&
		    (cc->listener.position[1] == values[1]) &&
		    (cc->listener.position[2] == values[2])) {
			break;
		}
		cc->listener.position[0] = values[0];
		cc->listener.position[1] = values[1];
		cc->listener.position[2] = values[2];
		_alcDCSpeakerMove();
		break;

	case AL_VELOCITY:
		cc->listener.velocity[0] = values[0];
		cc->listener.velocity[1] = values[1];
		cc->listener.velocity[2] = values[2];
		break;

	case AL_GAIN:
	case AL_GAIN_LINEAR_LOKI:
		if (values[0] < 0.0f) {
			_alDCSetError(AL_INVALID_VALUE);
			break;
		}
		cc->listener.gain = values[0];
		break;

	case AL_ORIENTATION:
		if ((cc->listener.orientation[0] == values[0]) &&
		    (cc->listener.orientation[1] == values[1]) &&
		    (cc->listener.orientation[2] == values[2]) &&
		    (cc->listener.orientation[3] == values[3]) &&
		    (cc->listener.orientation[4] == values[4]) &&
		    (cc->listener.orientation[5] == values[5])) {
			break;
		}
		cc->listener.orientation[0] = values[0]; /* at */
		cc->listener.orientation[1] = values[1];
		cc->listener.orientation[2] = values[2];
		cc->listener.orientation[3] = values[3]; /* up */
		cc->listener.orientation[4] = values[4];
		cc->listener.orientation[5] = values[5];
		_alcDCSpeakerMove();
		break;

	default:
		_alDCSetError( AL_INVALID_ENUM );
		break;
	}

	_alcDCUnlockContext();
}

static void
setListenerAttributei( ALenum param, const ALint *intValues, ALint numValues)
{
	ALfloat floatValues[MAX_LISTENER_NUM_VALUES];
	int i;
	for (i = 0; i < numValues; i++) {
		floatValues[i] = (ALfloat)intValues[i];
	}
	setListenerAttributef(param, floatValues, numValues);
}

void
alListenerf( ALenum param, ALfloat value )
{
	setListenerAttributef(param, &value, 1);
}

void
alListener3f( ALenum param, ALfloat value1, ALfloat value2, ALfloat value3 )
{
	ALfloat values[3];
	values[0] = value1;
	values[1] = value2;
	values[2] = value3;
	setListenerAttributef(param, values, 3);
}

void
alListenerfv( ALenum param, const ALfloat *values )
{
	setListenerAttributef(param, values, numValuesForAttribute(param));
}
 
void
alListeneri( ALenum param, ALint value )
{
	setListenerAttributei(param, &value, 1);
}

void
alListener3i( ALenum param, ALint value1, ALint value2, ALint value3 )
{
	ALint values[3];
	values[0] = value1;
	values[1] = value2;
	values[2] = value3;
	setListenerAttributei(param, values, 3);
}

void
alListeneriv( ALenum param, const ALint *values )
{
	setListenerAttributei(param, values, numValuesForAttribute(param));
}

static ALboolean
getListenerAttribute( ALenum param, ALfloat *values, ALint numValues )
{
	ALboolean ok = AL_FALSE;
	/* TODO: As usual, we should better have a getter for a *locked* context */
	AL_context *cc = _alcDCGetContext();
	if( cc == NULL ) {
		_alDCSetError(AL_INVALID_OPERATION);
		return ok;
	}
	_alcDCLockContext();

	if (numValues != numValuesForAttribute(param)) {
		_alDCSetError(AL_INVALID_ENUM);
		_alcDCUnlockContext();
		return ok;
	}
	if( values == NULL ) {
		_alDCSetError(AL_INVALID_VALUE);
		_alcDCUnlockContext();
		return ok;
	}

	switch( param ) {
	case AL_POSITION:
		values[0] = cc->listener.position[0];
		values[1] = cc->listener.position[1];
		values[2] = cc->listener.position[2];
		ok = AL_TRUE;
		break;

	case AL_VELOCITY:
		values[0] = cc->listener.velocity[0];
		values[1] = cc->listener.velocity[1];
		values[2] = cc->listener.velocity[2];
		ok = AL_TRUE;
		break;

	case AL_GAIN:
	case AL_GAIN_LINEAR_LOKI:
		values[0] = cc->listener.gain;
		ok = AL_TRUE;
		break;

	case AL_ORIENTATION:
		values[0] = cc->listener.orientation[0]; /* at */
		values[1] = cc->listener.orientation[1];
		values[2] = cc->listener.orientation[2];
		values[3] = cc->listener.orientation[3]; /* up */
		values[4] = cc->listener.orientation[4];
		values[5] = cc->listener.orientation[5];
		ok = AL_TRUE;
		break;

	default:
		_alDCSetError( AL_INVALID_ENUM );
		break;
	}

	_alcDCUnlockContext();
	return ok;
}

void
alGetListenerf( ALenum param, ALfloat *value )
{
	getListenerAttribute(param, value, 1);
}

void
alGetListener3f( ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3 )
{
	ALfloat floatValues[3];
	if (getListenerAttribute(param, floatValues, 3)) {
		*value1 = floatValues[0];
		*value2 = floatValues[1];
		*value3 = floatValues[2];
	}
}

void
alGetListenerfv( ALenum param, ALfloat *values )
{
	getListenerAttribute(param, values, numValuesForAttribute(param));
}

void
alGetListeneri( ALenum param, ALint *value )
{
	ALfloat floatValues[1];
	if (getListenerAttribute(param, floatValues, 1)) {
		*value = floatValues[0];
	}
}

void
alGetListener3i( ALenum param, ALint *value1, ALint *value2, ALint *value3 )
{
	ALfloat floatValues[3];
	if (getListenerAttribute(param, floatValues, 3)) {
		*value1 = (ALint)floatValues[0];
		*value2 = (ALint)floatValues[1];
		*value3 = (ALint)floatValues[2];
	}
}

void
alGetListeneriv( ALenum param, ALint *values )
{
	ALfloat floatValues[MAX_LISTENER_NUM_VALUES];
	int i;
	int n = numValuesForAttribute(param);
	if (getListenerAttribute(param, floatValues, n)) {
		for (i = 0; i < n; i++) {
			values[i] = (ALint)floatValues[i];
		}
	}
}

/*
 * Initializes already allocated listener.
 */
void
_alInitListener( AL_listener *listener )
{
	listener->position[0] = 0.0f;
	listener->position[1] = 0.0f;
	listener->position[2] = 0.0f;

	listener->velocity[0] = 0.0f;
	listener->velocity[1] = 0.0f;
	listener->velocity[2] = 0.0f;

	listener->gain = 1.0f;

	/* at */
	listener->orientation[0] = 0.0f;
	listener->orientation[1] = 0.0f;
	listener->orientation[2] = -1.0f;

	/* up */
	listener->orientation[3] = 0.0f;
	listener->orientation[4] = 1.0f;
	listener->orientation[5] = 0.0f;
}

/*
 * Doesn't do anything.
 */
void
_alDestroyListener(UNUSED(AL_listener *ls))
{
}
