/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_error.c
 *
 * openal error reporting.
 *
 */

#include "al_siteconfig.h"

#include <AL/al.h>
#include <signal.h>
#include <stdio.h>

#include "al_debug.h"
#include "al_types.h"
#include "al_error.h"
#include "alc/alc_context.h"

/*
 * _alShouldBombOnError_LOKI controls whether or not _alSetError should
 * abort when setting an error.  This allows applications to get immediate
 * error reporting.
 */
ALboolean _alShouldBombOnError_LOKI = AL_FALSE;

/*
 * index2ErrorNo( int index )
 *
 * Returns an al error from a simple index.
 */
static ALenum index2ErrorNo( int index );

/*
 * ErrorNo2index( ALenum error_number )
 *
 * Returns a simple index from an al error.
 */
static int ErrorNo2index( ALenum error_number );

/**
 * Error support.
 */

/*
 * alGetError( void )
 *
 * Returns the most recent error generated in the AL state machine.
 */
ALenum alGetError( void ) {
	AL_context *cc;
	int index;

	_alcDCLockContext();

	cc = _alcDCGetContext();

	if(cc == NULL)
	{
		_alcDCUnlockContext();
		return 0;
	}

	index = index2ErrorNo(cc->alErrorIndex);

	/*
	 * In deference to the new spec, GetError clears the error
	 * after reading it.
	 */
	cc->alErrorIndex = 0;

	_alcDCUnlockContext();

	return index;
}

/*
 * ErrorNo2index( ALenum error_number )
 *
 * Returns a simple index from an al error.
 */
static int ErrorNo2index( ALenum error_number ) {
	switch( error_number ) {
		case AL_NO_ERROR:
			return 0;
			break;
		case AL_INVALID_NAME:
			return 1;
			break;
		case AL_INVALID_ENUM:
			return 2;
			break;
		case AL_INVALID_VALUE:
			return 3;
			break;
		case AL_INVALID_OPERATION:
			return 4;
			break;
		case AL_OUT_OF_MEMORY:
			return 5;
			break;
		default:
			_alDebug( ALD_ERROR, __FILE__, __LINE__,
				"Unknown error condition: 0x%x", error_number );
			return -1;
			break;
	}

	return -1;
}

/*
 * index2ErrorNo( int index )
 *
 * Returns an al error from a simple index.
 */
static ALenum index2ErrorNo( int index ) {
	switch( index ) {
 		case 0:
			return AL_NO_ERROR;
			break;
		case 1:
			return AL_INVALID_NAME;
			break;
		case 2:
			return AL_INVALID_ENUM;
			break;
		case 3:
			return AL_INVALID_VALUE;
			break;
		case 4:
			return AL_INVALID_OPERATION;
			break;
		case 5:
			return AL_OUT_OF_MEMORY;
			break;
		default:
			_alDebug( ALD_ERROR, __FILE__, __LINE__,
				"Unknown index : %d", index );
			break;
	}

	return -1;
}

/*
 * _alSetError( ALuint cid, ALenum param )
 *
 * Sets the error for the context with name cid to param.
 *
 * assumes locked context
 */
void _alSetError( ALuint cid, ALenum param ) {
	AL_context *cc;

	cc = _alcGetContext( cid );
	if(cc == NULL) {
		/* No default context, no error set. */
		return;
	}

	if( cc->alErrorIndex == 0 ) {
		/*
		 * Only set error if no previous error has been recorded.
		 */

		cc->alErrorIndex = ErrorNo2index(param);
	}

	if(_alShouldBombOnError_LOKI == AL_TRUE) {
		raise(SIGABRT);
	}

	return;
}
