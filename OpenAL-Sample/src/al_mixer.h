/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_mixer.h
 *
 * Prototypes, macros and definitions related to the control and
 * execution of the mixing "thread".
 *
 * The mixing "thread" is responsible for managing playing sources,
 * applying the requisite filters, mixing in audio data from said sources,
 * etc.
 *
 */
#ifndef AL_AL_MIXER_H_
#define AL_AL_MIXER_H_

#include "al_siteconfig.h"
#include <AL/al.h>

/*
 * Number of sources for which optimized mixing functions exist.
 */
#ifdef USE_LIGHT_GEN_MIXING
#define GENMIXSOURCES 8
#else /* not USE_LIGHT_GEN_MIXING */
#define GENMIXSOURCES 64
#endif /* not USE_LIGHT_GEN_MIXING */

#ifdef __MMX__
#define MMXMIXSOURCES 32
#else /* not __MMX__ */
#define MMXMIXSOURCES 0
#endif /* not __MMX__ */

/* set MAXMIXSOURCES to MAX */
#if GENMIXSOURCES<MMXMIXSOURCES
#define MAXMIXSOURCES MMXMIXSOURCES
#else /* not GENMIXSOURCES<MMXMIXSOURCES */
#define MAXMIXSOURCES GENMIXSOURCES
#endif /* not GENMIXSOURCES<MMXMIXSOURCES */


/*
 * our main mixing function.
 */
extern int (*mixer_iterate)( void *dummy );

/*
 * The mixing function checks this variable for equality with AL_TRUE.  When
 * this is the case, it exits.  The default is AL_FALSE.
 */
extern volatile ALboolean time_for_mixer_to_die;

/*
 * Create and initialize data structures needed by the mixing function.
 */
ALboolean _alInitMixer( void );

/*
 * Inform the mixer that settings may have changed.  Data structures can
 * be/are updated to reflect new settings in the current context.
 *
 * Synchronous, if AL_FALSE, causes a new thread to be launched.
 */
void _alSetMixer( ALboolean synchronous );

/*
 * Deallocate data allocated in _alInitMixer.
 */
void _alDestroyMixer( void );

/*
 * "play" the source named by sid.  If sid does not refer to a valid source,
 * AL_INVALID_NAME is set.
 */
void _alAddSourceToMixer( ALuint sid );

/*
 * "stop" the source named by sid.  If sid does not refer to a valid source,
 * AL_INVALID_NAME is set.
 */
ALboolean _alRemoveSourceFromMixer( ALuint sid );

/*
 * "start" the capture named by cpid.  If cpid does not refer to a valid
 * capture, AL_INVALID_NAME is set.
 */
void _alAddCaptureToMixer( ALuint cpid );

/*
 * "stop" the capture named by cpid.  If cpid does not refer to a valid
 * capture, AL_INVALID_NAME is set.
 */
void _alRemoveCaptureFromMixer( ALuint cpid );

/*
 * Lock the mixer mutex, handing fn and ln to _alLockPrintf
 */
void FL_alLockMixBuf( const char *fn, int ln );

/*
 * Unlock the mixer mutex, handing fn and ln to _alLockPrintf
 */
void FL_alUnlockMixBuf( const char *fn, int ln );

/*
 * functions to pause async mixer.  Oy Vey
 */

/*
 * Lock the MixerPause mutex, which is use to "pause" the mixer.
 */
void _alLockMixerPause( void );

/*
 * Unlock the MixerPause mutex, which is use to "resume" the mixer.
 */
void _alUnlockMixerPause( void );

/* macro madness */
#define _alLockMixBuf()   FL_alLockMixBuf(__FILE__, __LINE__)
#define _alUnlockMixBuf() FL_alUnlockMixBuf(__FILE__, __LINE__)

#endif /* not AL_AL_MIXER_H_ */
