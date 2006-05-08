/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_mixfunc.h
 *
 * Mixfuncs --- specialized mixing functions --- are unrolled, hopefully
 * easily optimizable mixing functions that take advantage of knowing the
 * number of mixing streams passed to them.  Used in conjunction with the
 * mixmanager, this provides an interface for the library to plug in well
 * optimized mixing functions.
 */
#ifndef AL_AL_MIXFUNC_H_
#define AL_AL_MIXFUNC_H_

#include "al_siteconfig.h"
#include "al_types.h"
#include "al_mixer.h"

typedef struct _AL_MixFunc {
	void ( *funcs[ MAXMIXSOURCES + 1 ] )( ALshort *dst,
					      alMixEntry *entries );
	void ( *func_n )( ALshort *dst,
	                  alMixEntry *entries,
	                  ALuint numents );
	ALuint max;
} ALMixFunc;

/*
 * _alMixFuncInit initializes the already allocated ALMixFunc object (mf) to
 * contain at least size mixing functions.
 */
ALboolean _alMixFuncInit( ALMixFunc *mf, ALuint size );

/*
 * _alMixFuncDestroy performs any needed finalization on the ALMixFunc object
 * mf.
 */
void _alMixFuncDestroy( ALMixFunc *mf );

#endif /* not AL_AL_MIXFUNC_H_ */
