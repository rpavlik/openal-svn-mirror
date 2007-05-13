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

/* Sets the context-independant error to param */
void _alcSetError( ALCenum param );

#endif /* not AL_ALC_ALC_ERROR_H_ */
