/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_main.h
 *
 * Miscellanous prototypes, macros and definitions
 */
#ifndef _AL_MAIN_H_
#define _AL_MAIN_H_

#include "al_siteconfig.h"

#include <AL/al.h>

#include "alc/alc_context.h"

/*
 * If compiling with gcc, then we can use attributes.
 *
 * UNUNSED(x) flags a parameter or variable as (potentially) being unused, so
 * that gcc doesn't complain about it with -Wunused.
 */
#ifdef __GNUC__

    /* darwin os uses a cc based on gcc and have __GNUC__ defined.  */
    #if !defined(DARWIN_TARGET) || __APPLE_CC__ >= 1041
    #define UNUSED(x) x __attribute((unused))
    #else
    #define UNUSED(x) x
    #endif /* DARWIN_TARGET */
#else
#define UNUSED(x) x
#endif /* GNU_C_ */

/*
 * _alLockPrintf is used for debugging purposes.  If DEBUG_LOCK is defined,
 * calls to _alLockPrintf generate a print to stderr.  If not, these calls are
 * optimized away.
 */
#ifndef DEBUG_LOCK
#define _alLockPrintf(x, f, l)
#else
int _alLockPrintf( const char *str, const char *fn, int line );
#endif /* DEBUG_LOCK */

/*
 * evaluates to the depth of the passed audioconvert format, in bits.
 */
#define _al_ACformatbits(fmt) (fmt & 0x00FF)

/*
 * These macros are make calls to the associated regular-library str calls,
 * but cast their arguments to const char * before doing so.  This is to aid
 * the manipulate of const ALubyte * strings.
 */
#define ustrcmp(s1, s2)     strcmp((const char *) s1, (const char *) s2)
#define ustrncmp(s1, s2, n) strncmp((const char *) s1, \
				    (const char *) s2, \
				    n)
#define ustrncpy(s1, s2, n) strncpy((char *) s1, \
				    (const char *) s2, \
				    n)

/*
 * Like memcpy, but copies to dst + offset instead of to dst.
 */
#define offset_memcpy(d,o,s,l) memcpy(((char *)d) + o, s, l)

/*
 * Like memcpy, but copies from src + offset instead of from src.
 */
#define memcpy_offset(d,s,o,l) memcpy(d, (char *) s + o, l)

/*
 * If DEBUG_STUB is defined, _alStub prints out a warning message.  If not, no
 * action is taken.
 *
 */
#ifdef DEBUG_STUB
void _alStub( const char *str );
#else
#define _alStub( s )
#endif /* DEBUG_STUB */

/*
 * Does misc. initialization for the library.
 */
ALboolean _alInit( void );

/*
 * Deallocates the data structures created by _alInit.
 */
void _alExit( void );

/*
 * Returns the openal format that has the number of channels channels and the
 * bit depth bits.
 */
ALenum _al_AL2FMT( ALuint channels, ALuint bits );

/*
 * Returns the openal format equivilant to the audioconvert format acformat,
 * with the number of channels specified by channels.
 */
ALenum _al_AC2ALFMT( ALuint acformat, ALuint channels );

/*
 * Returns the equivilant (sort of) audioconvert format specified by alfmt.
 * audioconvert formats do not have channel information, so this should be
 * combined with _alGetChannelsFromFormat.
 */
ALushort _al_AL2ACFMT( ALenum alfmt );

/*
 * evaluates to the number of channels in an openal format.
 */
ALubyte  _alGetChannelsFromFormat(ALenum alformat);

/*
 * Returns the number of bits per sample for the given format.
 */
ALbyte _alGetBitsFromFormat( ALenum format );

/*
 * Returns the openal format that is identical to format, but with sufficient
 * channel width to accomedate new_channel_num channels.
 */
ALenum _al_formatscale( ALenum format, ALuint new_channel_num );

/*
 * Returns the number of byte necessary to contain samples worth of data, if
 * the data undergoes a conversion from ffreq to tfreq in the sampling-rate
 * and from ffmt to tfmt in terms of format.
 */
ALuint _al_PCMRatioify( ALuint ffreq, ALuint tfreq,
			ALenum ffmt, ALenum tfmt,
			ALuint samples );

/*
 *  Returns AL_TRUE if format is an openal format specifying raw pcm data,
 *  AL_FALSE otherwise.
 */
ALboolean _al_RAWFORMAT( ALenum format );

/*
 * Multiplies each ALshort in bpt (len bytes long) by sa, clamped above
 * by 32767 and below by -32768.  Only appropriate for 0.0 <= sa <= 1.0.
 */
void _alFloatMul( ALshort *bpt, ALfloat sa, ALuint len );

/*
 * Starts the mixer thread.
 */
void _alStartMixerThread( void );

/*
 * Waits for the mixer thread to die.  This call does not kill the mixer
 * thread, it just hangs around poking the body.
 */
void _alWaitForMixerThreadToDie( void );

/*
 * Copies srcs[0..nc-1][0..(len/2)-1] to
 * dsts[0..nc-1][offset/2..((offset + len)/2)-1].
 */
void _alBuffersAppend( void **dsts, void **srcs, int len, int offset, int nc );

/*
 * slurp file named by fname to into *buffer, mallocing memory.
 */
int _alSlurp( const char *fname, void **buffer );

/*
 * sleep for n microseconds
 */
void _alMicroSleep( unsigned int n );

/*
 * Convert degree argument to radians
 */
ALfloat _alDegreeToRadian( ALfloat degree );

/*
 * Functions for verifying values fall between min and max,
 * inclusive.
 */

/*
 * Returns AL_TRUE if val is between min and max, inclusive.
 */
ALboolean _alCheckRangef( ALfloat val, ALfloat min, ALfloat max );

/*
 * Returns AL_TRUE if val is either AL_TRUE or AL_FALSE.
 */
ALboolean _alCheckRangeb( ALboolean val );

/*
 * Returns true if fv1 == { 0.0f, 0.0f, 0.0f }
 */
ALboolean _alIsZeroVector( const ALfloat *fv1 );

/*
 * the buffers that sources are split into in SplitSources and
 * Collapsed from in CollapseSources.  Filters work on these
 * intermediate buffers, each of which contains one mono channel of
 * the source data.
 *
 * f_buffers contain PCM data
 */

extern _alDecodeScratch f_buffers;

/*
 * Returns smallest power of two large that meets or exceeds num.
 */
ALuint _alSmallestPowerOfTwo( ALuint num );

/*
 * Returns AL_TRUE if v is a finite, non NaN value, AL_FALSE otherwise.
 */
ALboolean _alIsFinite( ALfloat v );

#endif
