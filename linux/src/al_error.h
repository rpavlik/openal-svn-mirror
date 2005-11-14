/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_error.h
 *
 * openal error reporting.
 */
#ifndef _AL_ERROR_H_
#define _AL_ERROR_H_

#include <AL/al.h>
#include "alc/alc_context.h"

/*
 * _alSetError( ALuint cid, ALenum param )
 *
 * Sets the context specific error for the context specified by cid to the
 * value param.
 */
void _alSetError( ALuint cid, ALenum param );

/*
 * _alShouldBombOnError_LOKI is a variable that, when set to AL_TRUE, will
 * cause alSetError to abort when setting an error.  Default is AL_FALSE.
 */
extern ALboolean _alShouldBombOnError_LOKI;

/* Macros to handle default context */

#define _alDCSetError(p) _alSetError(_alcCCId, p)

#endif /* _AL_ERROR_H_ */
