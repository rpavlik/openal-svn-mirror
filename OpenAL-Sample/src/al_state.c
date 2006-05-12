/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_state.c
 *
 * Per-context state management.
 *
 */
#include "al_siteconfig.h"

#include <AL/al.h>

#include "al_distance.h"
#include "al_error.h"
#include "al_ext.h"

/*
 * A few general remarks about the code below:
 *
 *    * When there is no current context, we set an AL_INVALID_OPERATION. Even
 *      without a context for recording this error, this is nice for
 *      debugging.
 *
 *    * As a general contract with the user, we never modify any data via
 *      pointers if an error condition occurs.
 *
 *    * Getting and locking the context should be done in a single atomic
 *      operation, but our internal API is not yet capable of this.
 *
 *    * We should somehow hook in the extension mechanism here.
 *
 */

void
alEnable( ALenum param )
{
	AL_context *cc = _alcDCGetContext();
	if( cc == NULL ) {
		_alDCSetError( AL_INVALID_OPERATION );
		return;
	}
	_alcDCLockContext();

	switch (param) {
	default:
		_alDCSetError( AL_INVALID_ENUM );
		break;
	}

	_alcDCUnlockContext();
}

void
alDisable( ALenum param )
{
	AL_context *cc = _alcDCGetContext();
	if( cc == NULL ) {
		_alDCSetError( AL_INVALID_OPERATION );
		return;
	}
	_alcDCLockContext();

	switch (param) {
	default:
		_alDCSetError( AL_INVALID_ENUM );
		break;
	}

	_alcDCUnlockContext();
}

ALboolean
alIsEnabled( ALenum param )
{
	AL_context *cc = _alcDCGetContext();
	if( cc == NULL ) {
		_alDCSetError( AL_INVALID_OPERATION );
		return AL_FALSE;
	}
	_alcDCLockContext();

	switch (param) {
	default:
		_alDCSetError( AL_INVALID_ENUM );
		break;
	}

	_alcDCUnlockContext();
	return AL_FALSE;
}

const ALchar*
alGetString( ALenum param )
{
	const ALchar *value;
	static ALchar extensions[1024]; /* TODO: Ugly and not thread-safe! */

	AL_context *cc = _alcDCGetContext();
	if( cc == NULL ) {
		_alDCSetError( AL_INVALID_OPERATION );
		return NULL;
	}
	_alcDCLockContext();

	switch (param) {
	case AL_VERSION:
		value = "1.1";
		break;
	case AL_RENDERER:
		value = "Software";
		break;
	case AL_VENDOR:
		value = "OpenAL Community";
		break;
	case AL_EXTENSIONS:
		_alGetExtensionStrings( extensions, sizeof( extensions ) );
		value = extensions;
		break;
	case AL_NO_ERROR:
		value = "No Error";
		break;
	case AL_INVALID_NAME:
		value = "Invalid Name";
		break;
	case AL_INVALID_ENUM:
		value = "Invalid Enum";
		break;
	case AL_INVALID_VALUE:
		value = "Invalid Value";
		break;
	case AL_INVALID_OPERATION:
		value = "Invalid Operation";
		break;
	case AL_OUT_OF_MEMORY:
		value = "Out of Memory";
		break;
	default:
		value = NULL;
		_alDCSetError( AL_INVALID_ENUM );
		break;
	}

	_alcDCUnlockContext();
	return value;
}

#define MAX_DATA_ENTRIES_FILLED 1

/* everything is better than code duplication, even macros... */

#define DEFINE_GETTER(type,conv,namev,name)		\
void							\
namev( ALenum param, type *data )			\
{							\
	AL_context *cc = _alcDCGetContext();		\
	if( cc == NULL ) {				\
		_alDCSetError( AL_INVALID_OPERATION );	\
		return;					\
	}						\
	_alcDCLockContext();				\
							\
	switch( param ) {				\
	case AL_DOPPLER_FACTOR:				\
		data[0] = conv(cc->doppler_factor);	\
		break;					\
	case AL_DOPPLER_VELOCITY:			\
		data[0] = conv(cc->doppler_velocity);	\
		break;					\
	case AL_SPEED_OF_SOUND:				\
		data[0] = conv(cc->speed_of_sound);	\
		break;					\
	case AL_DISTANCE_MODEL:				\
		data[0] = conv(cc->distance_model);	\
		break;					\
	default:					\
		_alDCSetError( AL_INVALID_ENUM );	\
		break;					\
	}						\
							\
	_alcDCUnlockContext();				\
}							\
							\
type							\
name( ALenum param )					\
{							\
	type buf[MAX_DATA_ENTRIES_FILLED];		\
	namev(param, buf);				\
	return buf[0];					\
}

#define CONV_BOOLEAN(x) ((x) ? AL_TRUE : AL_FALSE)
#define CONV_INTEGER(x) ((ALint)(x))
#define CONV_FLOAT(x)   ((ALfloat)(x))
#define CONV_DOUBLE(x)  ((ALdouble)(x))

DEFINE_GETTER(ALboolean,CONV_BOOLEAN,alGetBooleanv,alGetBoolean)
DEFINE_GETTER(ALint,CONV_INTEGER,alGetIntegerv,alGetInteger)
DEFINE_GETTER(ALfloat,CONV_FLOAT,alGetFloatv,alGetFloat)
DEFINE_GETTER(ALdouble,CONV_DOUBLE,alGetDoublev,alGetDouble)

#undef DEFINE_GETTER

void
alDopplerFactor( ALfloat value )
{
	AL_context *cc = _alcDCGetContext();
	if( cc == NULL ) {
		_alDCSetError( AL_INVALID_OPERATION );
		return;
	}
	_alcDCLockContext();

	if( value < 0.0f ) {
		_alDCSetError( AL_INVALID_VALUE );
		_alcDCUnlockContext();
		return;
	}
	cc->doppler_factor = value;

	_alcDCUnlockContext();
}

void
alDopplerVelocity( ALfloat value )
{
	AL_context *cc = _alcDCGetContext();
	if( cc == NULL ) {
		_alDCSetError( AL_INVALID_OPERATION );
		return;
	}
	_alcDCLockContext();

	if( value <= 0.0f ) {
		_alDCSetError( AL_INVALID_VALUE );
		_alcDCUnlockContext();
		return;
	}
	cc->doppler_velocity = value;

	_alcDCUnlockContext();
}

void
alSpeedOfSound( ALfloat value )
{
	AL_context *cc = _alcDCGetContext();
	if( cc == NULL ) {
		_alDCSetError( AL_INVALID_OPERATION );
		return;
	}
	_alcDCLockContext();

	if( value <= 0.0f ) {
		_alDCSetError( AL_INVALID_VALUE );
		_alcDCUnlockContext();
		return;
	}
	cc->speed_of_sound = value;

	_alcDCUnlockContext();
}

void
alDistanceModel( ALenum distanceModel )
{
	AL_context *cc = _alcDCGetContext();
	if( cc == NULL ) {
		_alDCSetError( AL_INVALID_OPERATION );
		return;
	}
	_alcDCLockContext();

	switch( distanceModel ) {
	case AL_NONE:
	case AL_INVERSE_DISTANCE:
	case AL_INVERSE_DISTANCE_CLAMPED:
	case AL_LINEAR_DISTANCE:
	case AL_LINEAR_DISTANCE_CLAMPED:
	case AL_EXPONENT_DISTANCE:
	case AL_EXPONENT_DISTANCE_CLAMPED:
		cc->distance_model = distanceModel;
		_alUpdateDistanceModel(cc);
		break;
	default:
		_alDCSetError( AL_INVALID_ENUM );
		break;
	}

	_alcDCUnlockContext();
}
