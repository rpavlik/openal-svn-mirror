
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "al_debug.h"
#include "al_error.h"
#include "al_types.h"
#include "al_listen.h"
#include "al_main.h"
#include "al_error.h"
#include "al_config.h"

#include "alc/alc_context.h"
#include "alc/alc_speaker.h"

/*
 * alListeneri( ALenum param, ALint value )
 *
 * Sets the listener attribute associated with param to int value.  If param
 * is not a valid listener attribute, set AL_INVALID_ENUM.  If value is not a
 * valid value for the attribute, set AL_INVALID_VALUE.
 */
void alListeneri( ALenum param, ALint value )
{
	ALfloat fv = value;

	alListenerf(param, fv);

	return;
}

/*
 * alListenerf( ALenum param, ALfloat value )
 *
 * Sets the listener attribute associated with param to float value.  If param
 * is not a valid listener attribute, set AL_INVALID_ENUM.  If value is not a
 * valid value for the attribute, set AL_INVALID_VALUE.
 */
void alListenerf( ALenum param, ALfloat value ) {
	AL_context *dc;
	ALboolean inrange = AL_TRUE;

	_alcDCLockContext();
	dc = _alcDCGetContext();
	if(dc == NULL) {
		_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
			  "alListenerf: no current context\n");

		_alcDCUnlockContext();

		return;
	}

	/* check range */
	switch( param ) {
		case AL_GAIN:
		case AL_GAIN_LINEAR_LOKI:
			inrange = _alCheckRangef(value, 0.0f, 1.0f);
			break;
		default:
			/*
			 * Unknown param, error below.
			 */
			break;
	}

	if(inrange == AL_FALSE) {
		_alDebug(ALD_CONTEXT, __FILE__, __LINE__,
			 "alListenerf(0x%x): value %f out of range",
			 param, value);

		_alDCSetError(AL_INVALID_VALUE);
		_alcDCUnlockContext();

		return;
	}

	switch( param ) {
		case AL_GAIN:
		case AL_GAIN_LINEAR_LOKI:
			dc->listener.Gain = value;
			break;
		default:
			_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
				  "alListenerf: invalid param 0x%x.",
				  param );

			_alDCSetError( AL_INVALID_ENUM );

			break;
	}

	_alcDCUnlockContext();

	return;
}

/*
 * alListener3f( ALenum pname, ALfloat f1, ALfloat f2, ALfloat f3 )
 *
 * Sets the listener attribute associated with param to float vector composed
 * of { f1, f2, f3 }.  If param is not a valid listener attribute, set
 * AL_INVALID_ENUM. If any member of f1, f2, f3 is not a valid value for the
 * attribute, set AL_INVALID_VALUE.
 */
void alListener3f( ALenum pname, ALfloat f1, ALfloat f2, ALfloat f3 ) {
	ALfloat fv[3];

	fv[0] = f1;
	fv[1] = f2;
	fv[2] = f3;

	alListenerfv( pname, fv );

	return;
}

/*
 * alListenerfv( ALenum pname, ALfloat *pv )
 *
 * Sets the listener attribute associated with param to float vector pv. If
 * param is not a valid listener attribute, set AL_INVALID_ENUM. If any member
 * of pv is not a valid value for the attribute, set AL_INVALID_VALUE.
 */
void alListenerfv( ALenum pname, const ALfloat *pv ) {
	AL_context *dc;

	_alcDCLockContext();

	if( pv == NULL ) {
		/* silently ignore */
		_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
				"alListenerfv: invalid values NULL\n" );

		_alcDCUnlockContext();

		return;
	}

	dc = _alcDCGetContext();
	if(dc == NULL) {
		/* okay, this is weird */
		_alcDCUnlockContext();

		return;
	}

	switch( pname ) {
		case AL_POSITION:
			dc->listener.Position[0] = pv[0];
			dc->listener.Position[1] = pv[1];
			dc->listener.Position[2] = pv[2];

			_alcDCSpeakerMove();
			break;
		case AL_VELOCITY:
			dc->listener.Velocity[0] = pv[0];
			dc->listener.Velocity[1] = pv[1];
			dc->listener.Velocity[2] = pv[2];
			break;
		case AL_ORIENTATION:
			dc->listener.Orientation[0] = pv[0]; /* at */
			dc->listener.Orientation[1] = pv[1];
			dc->listener.Orientation[2] = pv[2];

			dc->listener.Orientation[3] = pv[3]; /* up */
			dc->listener.Orientation[4] = pv[4];
			dc->listener.Orientation[5] = pv[5];

			_alcDCSpeakerMove();
			break;
		default:
			_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
				  "alListenerfv: param 0x%x in not valid.",
				  pname);

			_alDCSetError( AL_INVALID_ENUM );

			break;
	}

	_alcDCUnlockContext();

	return;
}

/*
 * alGetListeneri( ALenum pname, ALint *value )
 *
 * Populates value with the value of the listener attribute pname.  If pname
 * is not a valid listener attribute, AL_INVALID_ENUM is set.  If value is
 * NULL, this is a legal NOP.
 */
void alGetListeneri( ALenum pname, ALint *value )
{
	ALint safety_first[6];

	alGetListeneriv( pname, safety_first );

	*value = safety_first[0];
}

/*
 * alGetListeneriv( ALenum pname, ALint *value )
 *
 * Populates value with the value of the listener attribute pname.  If pname
 * is not a valid listener attribute, AL_INVALID_ENUM is set.  If value is
 * NULL, this is a legal NOP.
 */
void alGetListeneriv( ALenum pname, ALint *value ) {
	AL_context *cc;
	ALint *temp;

	_alcDCLockContext();

	if(value == NULL) {
		/* silently ignore */
		_alDebug(ALD_CONTEXT, __FILE__, __LINE__,
		      "alGetListeneri: invalid values NULL\n");

		_alcDCUnlockContext();

		return;
	}

	cc = _alcDCGetContext();
	if(cc == NULL) {
		/*
		 * There is no current context, which means that
		 * we cannot set the error.  But if there is no
		 * current context we should not have been able
		 * to get here, since we've already locked the
		 * default context.  So this is weird.
		 *
		 * In any case, set and error, unlock, and pray.
		 */
		_alDCSetError( AL_INVALID_OPERATION );
		_alcDCUnlockContext();

		return;
	}

	temp = _alDCGetListenerParam( pname );
	if(temp == NULL) {
		_alDebug(ALD_CONTEXT, __FILE__, __LINE__,
			"alGetListeneriv: param 0x%x not valid", pname);

		_alDCSetError( AL_INVALID_ENUM );
		_alcDCUnlockContext();

		return;
	}

	*value = *temp;

	_alcDCUnlockContext();

	return;
}

/*
 * alGetListenerfv( ALenum pname, ALfloat *values )
 *
 * Populates values with the values of the listener attribute pname.  If pname
 * is not a valid listener attribute, AL_INVALID_ENUM is set.  If values is
 * NULL, this is a legal NOP.
 */
void alGetListenerf( ALenum param, ALfloat *value )
{
	ALfloat safety_first[6];

	alGetListenerfv( param, safety_first );

	*value = safety_first[0];

	return;
}

/*
 * alGetListenerfv( ALenum pname, ALfloat *values )
 *
 * Populates values with the values of the listener attribute pname.  If pname
 * is not a valid listener attribute, AL_INVALID_ENUM is set.  If values is
 * NULL, this is a legal NOP.
 */
void alGetListenerfv( ALenum param, ALfloat *values ) {
	AL_context *cc;
	ALfloat *fv;
	ALsizei numarguments = 1;

	switch( param ) {
		case AL_GAIN:
		case AL_GAIN_LINEAR_LOKI:
			/* only one float */
			numarguments = 1;
			break;
		case AL_ORIENTATION:
			numarguments = 6;
			break;
		case AL_POSITION:
		case AL_VELOCITY:
			numarguments = 3;
			break;
		default:
			_alcDCLockContext();
			_alDCSetError( AL_INVALID_ENUM );
			_alcDCUnlockContext();

			return;
			break;
	}

	if( values == NULL ) {
		/* silently ignore */

		_alDebug(ALD_CONTEXT, __FILE__, __LINE__,
		      "alGetListenerfv: invalid values NULL\n");

		return;
	}

	_alcDCLockContext();

	cc = _alcDCGetContext();

	if(cc == NULL) {
		/*
		 * There is no current context, which means that
		 * we cannot set the error.  But if there is no
		 * current context we should not have been able
		 * to get here, since we've already locked the
		 * default context.  So this is weird.
		 *
		 * In any case, set and error, unlock, and pray.
		 */

		_alDCSetError( AL_INVALID_OPERATION );
		_alcDCUnlockContext();

		return;
	}

	fv = _alDCGetListenerParam( param );
	if( fv != NULL ) {
		/*
		 * we actually have a value for the param, so
		 * copy it and return.  Otherwise, set default
		 * below or do conversion.
		 */

		memcpy( values, fv, sizeof *values * numarguments );

		_alcDCUnlockContext();

		return;
	}

	/*
	 * set default or do conversion.
	 */
	switch(param) {
		case AL_GAIN:
			fv = _alDCGetListenerParam( AL_GAIN );
			if( fv == NULL ) {
				values[0] = 1.0f;
			} else {
				values[0] = fv[0];
			}
			break;
		case AL_POSITION:
		case AL_VELOCITY:
			values[0] = 0.0f;
			values[1] = 0.0f;
			values[2] = 0.0f;
			break;
		case AL_ORIENTATION:
			/* at */
			values[0] = 0.0f;
			values[1] = 0.0f;
			values[2] = -1.0f;

			/* up */
			values[3] = 0.0f;
			values[4] = 1.0f;
			values[5] = 0.0f;

			break;
		default:
			_alDebug(ALD_CONTEXT, __FILE__, __LINE__,
				"alGetListenerfv: param 0x%x not valid",
				param );

		  _alDCSetError( AL_INVALID_ENUM );

		  break;
	}

	_alcDCUnlockContext();

	return;
}

/*
 * alGetListener4f( ALenum pname, ALfloat *f1, ALfloat *f2, ALfloat *f3 )
 *
 * Populates values with the values of the listener attribute pname.  If pname
 * is not a valid listener attribute, AL_INVALID_ENUM is set.  If values is
 * NULL, this is a legal NOP.
 */
void alGetListener3f( ALenum param,
		      ALfloat *f1, ALfloat *f2, ALfloat *f3 )
{
	ALfloat safety_first[6];

	alGetListenerfv(param, safety_first);

	*f1 = safety_first[0];
	*f2 = safety_first[1];
	*f3 = safety_first[2];

	return;
}

/*
 * _alDestroyListener(UNUSED(AL_listener *ls))
 *
 * Doesn't do anything.
 *
 */
void _alDestroyListener(UNUSED(AL_listener *ls))
{
	/* not needed */

	return;
}

/*
 * _alGetListenerParam( ALuint cid, ALenum param )
 *
 * Returns a pointer to the listener attribute specified by param, for the
 * context named cid, or NULL if param is invalid.
 *
 * assumes locked context
 *
 * FIXME: add case statements for other params
 */
void *_alGetListenerParam( ALuint cid, ALenum param ) {
	AL_context *cc;
	AL_listener *list;

	cc = _alcGetContext( cid );
	if(cc == NULL) {
		/*
		 * cid is an invalid context.	We can't set an error
		 * here because this requires a valid context.
		 */

		_alDebug(ALD_CONTEXT, __FILE__, __LINE__,
			"_alGetListenerParam: called with invalid context %d",
			cid);

		_alDCSetError( AL_INVALID_OPERATION );

		return NULL;
	}

	list = &cc->listener;

	switch(param) {
		case AL_GAIN:
		case AL_GAIN_LINEAR_LOKI:
			return &list->Gain;
			break;
		case AL_VELOCITY:
			return &list->Velocity;
			break;
		case AL_POSITION:
			return &list->Position;
		case AL_ORIENTATION:
			return &list->Orientation;
		default:
			_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
				  "_alGetListenerParam(%d, ...) passed bad param 0x%x",
				  param);

			_alSetError( cid, AL_INVALID_ENUM );

			break;
	}

	return NULL;
}

/*
 * _alInitListener( AL_listener *listener )
 *
 * Initializes already allocated listener.
 */
void _alInitListener( AL_listener *listener ) {
	ALfloat tempfv[6];
	ALboolean err;
	int i;

	err = _alGetGlobalVector("listener-position", ALRC_FLOAT, 3, tempfv);
	if(err == AL_FALSE) {
		/* no preset position */
		for(i = 0; i < 3; i++) {
			listener->Position[i] = 0.0f;
		}
	} else {
		memcpy( listener->Position, tempfv, SIZEOFVECTOR);
	}

	err = _alGetGlobalVector("listener-velocity", ALRC_FLOAT, 3, tempfv);
	if(err == AL_FALSE) {
		/* no preset velocity */
		for(i = 0; i < 3; i++) {
			listener->Velocity[i] = 0.0f;
		}
	} else {
		memcpy( listener->Velocity, tempfv, SIZEOFVECTOR);
	}

	err = _alGetGlobalVector("listener-orientation", ALRC_FLOAT, 6, tempfv);
	if(err == AL_FALSE) {
		/* no preset orientation */

		/* at */
		listener->Orientation[0] = 0.0f;
		listener->Orientation[1] = 0.0f;
		listener->Orientation[2] = -1.0f;

		/* up */
		listener->Orientation[3] = 0.0f;
		listener->Orientation[4] = 1.0f;
		listener->Orientation[5] = 0.0f;
	} else {
		memcpy( listener->Orientation, tempfv, 2 * SIZEOFVECTOR);
	}

	listener->Gain = 1.0f;

	return;
}
