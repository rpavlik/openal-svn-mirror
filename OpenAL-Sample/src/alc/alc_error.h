/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * alc_error.h
 *
 * openal alc error reporting.
 */
#ifndef AL_ALC_ALC_ERROR_H_
#define AL_ALC_ALC_ERROR_H_

#include "al_siteconfig.h"
#include <AL/alc.h>

/*
 * Returns AL_TRUE if param specifies a valid alc error, AL_FALSE otherwise.
 */
ALboolean alcIsError( ALCenum param );

/*
 * Sets the context-independant error to param, if param is a valid context
 * error.
 */
void _alcSetError( ALCenum param );

/*
 * Returns a const ALubyte * string giving a readable representation of the
 * error param, or NULL if param is not an alc error.
 */
const ALubyte *_alcGetErrorString( ALCenum param );

#endif /* not AL_ALC_ALC_ERROR_H_ */
