/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_main.c
 *
 * stuff that doesn't fit anywhere else.  Also, global initialization/
 * finitization.
 *
 */
#include "al_siteconfig.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <stdio.h>
#include <sys/stat.h>

#if HAVE_NANOSLEEP && HAVE_TIME_H
#include <time.h>
#include <errno.h>
#elif HAVE_USLEEP && HAVE_UNISTD_H
#include <unistd.h>
#elif HAVE_SLEEP && HAVE_WINDOWS_H
#include <windows.h>
#else
#error No way to sleep on this platform
#endif

#include "al_debug.h"
#include "al_dlopen.h"
#include "al_types.h"
#include "al_main.h"
#include "al_buffer.h"
#include "al_source.h"
#include "al_mixer.h"
#include "al_ext.h"
#include "config/al_config.h"
#include "al_vector.h"
#include "alc/alc_context.h"

#include "al_threadlib.h"

#include "audioconvert/audioconvert.h"

/* standard extensions
 *
 * To avoid having these built in (ie, using the plugin arch), don't
 * include these headers and move the files from EXT_OBJS to EXT_DLL_OBJS.
 */
#include "extensions/al_ext_loki.h"
#include "extensions/al_ext_mp3.h"
#include "extensions/al_ext_vorbis.h"

#ifndef M_PI
#define M_PI		3.14159265358979323846	/* pi */
#endif /* M_PI */

/*
 * mixer thread's ID, if it needs one
 */
extern ThreadID mixthread;

/*
 * pcm buffers that filters act on
 */
_alDecodeScratch f_buffers = {{NULL}, 0}; /* MacOS needs init vals */

/*
 * Our extension functions
 */
static AL_extension exts[] = {
#ifdef BUILTIN_EXT_LOKI
	BUILTIN_EXT_LOKI,
#endif /* BUILDIN_EXT_LOKI */
#ifdef BUILTIN_EXT_MP3
	BUILTIN_EXT_MP3,
#endif /* BUILDIN_EXT_MP3 */
#ifdef BUILTIN_EXT_VORBIS
	BUILTIN_EXT_VORBIS,
#endif /* BUILDIN_EXT_VORBIS */
	{ NULL, NULL }
};

/*
 * _alInit( void )
 *
 * _alInit is called when the "first" context is created.  If all
 * contexts are deleted, and then one is created, it is called again.
 *
 * Returns AL_TRUE unless some weird sort of memory allocation problem occurs,
 * in which case AL_FALSE is returned.
 */
ALboolean _alInit( void ) {
	ALboolean err;
	ALuint i;

	alDLInit_ ();

	for(i = 0; i < _ALC_MAX_CHANNELS; i++) {
		f_buffers.data[i]   = NULL;
	}

	f_buffers.len = 0;

	/* buffer initializations */
	err = _alInitBuffers();
	if(err == AL_FALSE) {
		return AL_FALSE;
	}

	/* extension initilizations */
	err = _alInitExtensions();
	if(err == AL_FALSE) {
		_alDestroyBuffers();

		return AL_FALSE;
	}

#ifdef BUILTIN_EXT_LOKI
	/* FIXME: dynamic-ify this */
	/* register extension groups */
	_alRegisterExtensionGroup( (const ALubyte*) "ALC_LOKI_audio_channel" );
	_alRegisterExtensionGroup( (const ALubyte*) "AL_LOKI_buffer_data_callback" );
	_alRegisterExtensionGroup( (const ALubyte*) "AL_LOKI_IMA_ADPCM_format" );
	_alRegisterExtensionGroup( (const ALubyte*) "AL_LOKI_WAVE_format" );
	_alRegisterExtensionGroup( (const ALubyte*) "AL_LOKI_play_position" );
	_alRegisterExtensionGroup( (const ALubyte*) "AL_LOKI_quadriphonic" );

#ifdef ENABLE_EXTENSION_AL_EXT_MP3
	 _alRegisterExtensionGroup( (const ALubyte*) "AL_EXT_MP3" );
#endif /* ENABLE_EXTENSION_AL_EXT_MP3 */

#ifdef ENABLE_EXTENSION_AL_EXT_VORBIS
	_alRegisterExtensionGroup( (const ALubyte*) "AL_EXT_vorbis" );
#endif /* ENABLE_EXTENSION_AL_EXT_VORBIS */

#endif /* BUILTIN_EXT_LOKI */

	_alRegisterExtensionGroup( (const ALubyte*) "AL_EXT_capture" );
	_alRegisterExtensionGroup( (const ALubyte*) "ALC_EXT_capture" );

	for(i = 0; exts[i].addr != NULL; i++) {
		_alRegisterExtension(exts[i].name, exts[i].addr);
	}

	/* do builtin extensions initialization */
	BUILTIN_EXT_LOKI_INIT;
	BUILTIN_EXT_MP3_INIT;
	BUILTIN_EXT_VORBIS_INIT;

	return AL_TRUE;
}

/*
 * _alExit( void )
 *
 * Finalizes things when the last context is deleted.
 *
 * FIXME: we can probably clean a lot of this up now that we have
 * alc{Open,Close}Device.
 */
void _alExit( void ) {
	int i;

#ifndef NO_THREADING
	/* we could be sync, so we check mixthread for a valid ID */
	if(mixthread != NULL) {
		time_for_mixer_to_die = AL_TRUE;

		_alWaitThread( mixthread );

		while( time_for_mixer_to_die == AL_TRUE ) {
			_alMicroSleep(100000);
		}
	}
#endif /* NO_THREADING */

	for(i = 0; i < _ALC_MAX_CHANNELS; i++) {
		if(f_buffers.data[i] != NULL) {
			free( f_buffers.data[i] );
			f_buffers.data[i] = NULL;
		}
	}

	f_buffers.len = 0;

	_alDestroyConfig();

	_alDestroyExtensions();
	_alDestroyExtensionGroups( );
	_alDestroyMixer();
	_alDestroyFilters();

	_alcDestroyAll();

	_alDestroyBuffers(); /* buffers after mixer and alc destroy */

	/* do builtin extensions destruction */
	BUILTIN_EXT_LOKI_FINI;
	BUILTIN_EXT_MP3_FINI;
	BUILTIN_EXT_VORBIS_FINI;

	alDLExit_ ();
}

/*
 * _alStub( const char *str )
 *
 * If DEBUG_STUB is defined, _alStub prints out a warning message.  If not, no
 * action is taken.
 *
 */
#ifdef DEBUG_STUB
void _alStub( const char *str ) {
	fprintf(stderr, "%s stub function\n", str);

	return;
}
#endif

/*
 * _alLockPrintf( const char *str, const char *fn, int line );
 *
 * _alLockPrintf is used for debugging purposes.  If DEBUG_LOCK is defined,
 * calls to _alLockPrintf generate a print to stderr.  If not, these calls are
 * optimized away.
 */
#ifdef DEBUG_LOCK
int _alLockPrintf( const char *msg, const char *fn, int ln ) {
	static char threadstr[2048];
	char blanks[] = "                             ";
	int maxlen = 18 - (strlen(fn) + log10(ln));

	blanks[maxlen] = '\0';

	snprintf(threadstr, sizeof(threadstr), "%s[%u]", blanks, _alSelfThread());

	return _alDebug(ALD_LOCK, fn, ln, "%s %s", threadstr, msg);
}
#endif

/*
 * _al_AC2ALFMT( ALuint acformat, ALuint channels )
 *
 * Returns the openal format equivilant to the audioconvert format acformat,
 * with the number of channels specified by channels.
 */
ALenum _al_AC2ALFMT( ALuint acformat, ALuint channels ) {
	switch( acformat ) {
		case AUDIO_U8:
			if(channels == 4) {
				return AL_FORMAT_QUAD8_LOKI;
			}
			if(channels == 2) {
				return AL_FORMAT_STEREO8;
			}
			if(channels == 1) {
				return AL_FORMAT_MONO8;
			}
			break;
		case AUDIO_S16LSB:
		case AUDIO_S16MSB:
			if(channels == 4) {
				return AL_FORMAT_QUAD16_LOKI;
			}
			if(channels == 2) {
				return AL_FORMAT_STEREO16;
			}
			if(channels == 1) {
				return AL_FORMAT_MONO16;
			}
			break;
	}

#ifdef DEBUG_CONVERT
	fprintf( stderr, "AC2ALFMT: wtf? format = 0x%x\n", acformat );
#endif

	return -1;
}

/*
 * _al_AL2ACFMT( ALenum alfmt )
 *
 * Returns the equivilant (sort of) audioconvert format specified by alfmt.
 * audioconvert formats do not have channel information, so this should be
 * combined with _alGetChannelsFromFormat.
 */
ALushort _al_AL2ACFMT( ALenum alformat ) {
	switch( alformat ) {
		case AL_FORMAT_QUAD8_LOKI:
		case AL_FORMAT_STEREO8:
		case AL_FORMAT_MONO8:
			return AUDIO_U8;
		case AL_FORMAT_QUAD16_LOKI:
		case AL_FORMAT_STEREO16:
		case AL_FORMAT_MONO16:
			return AUDIO_S16;
		default:
			break;
	}

#ifdef DEBUG_CONVERT
	fprintf(stderr, "AL2ACFMT: wtf? format = 0x%x\n", alformat);
#endif

	return 0;
}

/*
 * _alGetChannelsFromFormat(fmt)
 *
 * evaluates to the number of channels in an openal format.
 */
ALubyte _alGetChannelsFromFormat(ALenum alformat)
{
	switch( alformat ) {
		case AL_FORMAT_MONO8:
		case AL_FORMAT_MONO16:
			return 1;
		case AL_FORMAT_STEREO8:
		case AL_FORMAT_STEREO16:
			return 2;
		case AL_FORMAT_QUAD8_LOKI:
		case AL_FORMAT_QUAD16_LOKI:
			return 4;
		default:
			break;
	}

#ifdef DEBUG_CONVERT
	fprintf(stderr, "ALCHANNELS: wtf? format = 0x%x\n", alformat);
#endif
	return 0;
}

/*
 * _al_formatscale( ALenum format, ALuint new_channel_num )
 *
 * Returns the openal format that is identical to format, but with sufficient
 * channel width to accomedate new_channel_num channels.
 */
ALenum _al_formatscale(ALenum format, ALuint new_channel_num) {
	int fmt_bits = _alGetBitsFromFormat(format);

	switch(new_channel_num) {
		case 1:
		  switch(fmt_bits) {
			  case 8: return AL_FORMAT_MONO8; break;
			  case 16: return AL_FORMAT_MONO16; break;
			  default: return -1;
		  }
		  break;
		case 2:
		  switch(fmt_bits) {
			  case 8: return AL_FORMAT_STEREO8; break;
			  case 16: return AL_FORMAT_STEREO16; break;
			  default: return -1;
		  }
		  break;
		case 4:
		  switch(fmt_bits) {
			  case 8: return AL_FORMAT_QUAD8_LOKI; break;
			  case 16: return AL_FORMAT_QUAD16_LOKI; break;
			  default: return -1;
		  }
		  break;
		default:
#ifdef DEBUG_CONVERT
		  fprintf(stderr,
		  	"No support for %d channel AL format, sorry\n",
			new_channel_num);
#endif /* DEBUG_CONVERT */
		  break;
	}

	return -1;
}

/*
 * _alBuffersAppend( void **dsts, void **srcs, int len, int offset, int nc )
 *
 * Copies srcs[0..nc-1][0..(len/2)-1] to
 * dsts[0..nc-1][offset/2..((offset + len)/2)-1].
 */
void _alBuffersAppend(void **dsts, void **srcs, int len, int offset, int nc) {
	char *dstp;
	char *srcp;
	int i;
	int k;

	for(i = 0; i < nc; i++) {
		dstp = dsts[i];
		srcp = srcs[i];

		dstp += offset;

		for(k = 0; k < len; k++) {
			dstp[k] = srcp[k];
		}
	}

	return;
}

/*
 * _alRotatePointAboutAxis( const ALfloat angle, ALfloat *point,
 *                          const ALfloat *axis )
 *
 * Rotates point angle radians about axis.
 *
 * angle  - in radians
 * point  - x/y/z
 * axis   - x/y/z (unit vector)
 *
 * FIXME: check my math
 * FIXME: needs to check args
 */
#if 0
void _alRotatePointAboutAxis( const ALfloat angle, ALfloat *point,
                              const ALfloat *axis ) {
	ALmatrix *m;
	ALmatrix *pm;
	ALmatrix *rm;

	float s;
	float c;
	float t;

	float x = axis[0];
	float y = axis[1];
	float z = axis[2];
	int i;

	if(angle == 0.0) {
		/* FIXME: use epsilon? */
		return;
	}

	s = sin( angle );
	c = cos( angle );
	t = 1.0 - c;

	m  = _alMatrixAlloc(3, 3);
	pm = _alMatrixAlloc(1, 3);
	rm = _alMatrixAlloc(1, 3);

#if 1
	m->data[0][0] = t * x * x + c;
	m->data[0][1] = t * x * y - s * z;
	m->data[0][2] = t * x * z + s * y;

	m->data[1][0] = t * x * y + s * z;
	m->data[1][1] = t * y * y + c;
	m->data[1][2] = t * y * z - s * x;

	m->data[2][0] = t * x * z - s * y;
	m->data[2][1] = t * y * z + s * x;
	m->data[2][2] = t * z * z + c;
#else
	m->data[0][0] = t * x * x + c;
	m->data[1][0] = t * x * y - s * z;
	m->data[2][0] = t * x * z + s * y;

	m->data[0][1] = t * x * y + s * z;
	m->data[1][1] = t * y * y + c;
	m->data[2][1] = t * y * z - s * x;

	m->data[0][2] = t * x * z - s * y;
	m->data[1][2] = t * y * z + s * x;
	m->data[2][2] = t * z * z + c;
#endif

	for(i = 0; i < 3; i++) {
		pm->data[0][i] = point[i];
		rm->data[0][i] = 0;
	}

	/*
	 * rm = pm * m
	 */
	_alMatrixMul(rm, pm, m);

	for(i = 0; i < 3; i++) {
		point[i] = rm->data[0][i];
	}

	_alMatrixFree(m);
	_alMatrixFree(pm);
	_alMatrixFree(rm);

	return;
}
#endif

/*
 * _alSlurp( const char *fname, void **buffer )
 *
 * slurp file named by fname to into *buffer, mallocing memory.
 */
int _alSlurp(const char *fname, void **buffer) {
	struct stat buf;
	FILE *fh;
	size_t len;

	if((fname == NULL) || (buffer == NULL)) {
		return -1;
	}

	if(stat(fname, &buf) == -1) {
		/* couldn't stat file */
		return -1;
	}

	len = buf.st_size;
	if(len <= 0) {
		return -1;
	}

	fh = fopen(fname, "rb");
	if(fh == NULL) {
		/* couldn't open file */
		return -1;
	}

	*buffer = malloc(len);
	if(*buffer == NULL) {
		return -1;
	}

	if(fread(*buffer, len, 1, fh) < 1) {
		free(*buffer);

		return -1;
	}

	fclose( fh );

	return len;
}

/*
 * _al_PCMRatioify( ALuint ffreq, ALuint tfreq,
 *                  ALenum ffmt, ALenum tfmt,
 *                  ALuint samples )
 *
 * Returns the number of byte necessary to contain samples worth of data, if
 * the data undergoes a conversion from ffreq to tfreq in the sampling-rate
 * and from ffmt to tfmt in terms of format.
 */
ALuint _al_PCMRatioify( ALuint ffreq, ALuint tfreq,
			ALenum ffmt, ALenum tfmt,
			ALuint samples ) {
	ALuint ret = samples;

	ret *= ((float) tfreq / (float) ffreq);

	ret *= (_alGetBitsFromFormat( ffmt ) / 8 );
	ret /= (_alGetBitsFromFormat( tfmt ) / 8 );

	return ret;
}

/*
 * _al_AL2FMT( ALuint channels, ALuint bits )
 *
 * Returns the openal format that has the number of channels channels and the
 * bit depth bits.
 */
ALenum _al_AL2FMT(ALuint channels, ALuint bits) {
	switch(channels) {
		case 1:
			if(bits == 8) return AL_FORMAT_MONO8;
			if(bits == 16) return AL_FORMAT_MONO16;
			break;
		case 2:
			if(bits == 8) return AL_FORMAT_STEREO8;
			if(bits == 16) return AL_FORMAT_STEREO16;
			break;
		case 4:
			if(bits == 8) return AL_FORMAT_QUAD8_LOKI;
			if(bits == 16) return AL_FORMAT_QUAD16_LOKI;
			break;
	}

	return -1;
}

/* Code adapted from freealut's src/alutUtil.c */
void
_alMicroSleep(unsigned int microSeconds)
{
	unsigned int seconds = microSeconds / 1000000;
	unsigned int microSecondsRest = microSeconds - (seconds * 1000000);

#if HAVE_NANOSLEEP && HAVE_TIME_H

	struct timespec t, remainingTime;
	t.tv_sec = (time_t) seconds;
	t.tv_nsec = ((long) microSecondsRest) * 1000;

	/* At least the interaction of nanosleep and signals is specified! */
	while (nanosleep (&t, &remainingTime) < 0) {
		if (errno != EINTR) {
			return;
		}
		/* If we received a signal, let's try again with the remaining time. */
		t.tv_sec = remainingTime.tv_sec;
		t.tv_nsec = remainingTime.tv_nsec;
	}

#elif HAVE_USLEEP && HAVE_UNISTD_H

	while (seconds > 0) {
		usleep (1000000);
		seconds--;
	}
	usleep (microSecondsRest);

#elif HAVE_SLEEP && HAVE_WINDOWS_H

	while (seconds > 0) {
		Sleep (1000);
		seconds--;
	}
	Sleep ((DWORD) (microSecondsRest / 1000));

#endif

}

/*
 * _alDegreeToRadian( ALfloat degree )
 *
 * Returns radian equilvilant of degree.
 *
 */
ALfloat _alDegreeToRadian( ALfloat degree ) {
	return degree * (M_PI / 180.0f);
}

/*
 * _alCheckRangef( ALfloat val, ALfloat min, ALfloat max )
 *
 * Returns AL_TRUE if val is between min and max, inclusive.
 */
ALboolean _alCheckRangef(ALfloat val, ALfloat min, ALfloat max) {
	ALboolean retval = AL_TRUE;

#ifdef DEBUG
	if( _alIsFinite(val) == 0 ) {
		retval = AL_FALSE;
	}
#endif

	if(val < min) {
		retval = AL_FALSE;
	}
	if(val > max) {
		retval = AL_FALSE;
	}

	return retval;
}

/*
 * _alCheckRangeb( ALfloat val )
 *
 * Returns AL_TRUE if val is either AL_TRUE or AL_FALSE.
 */
ALboolean _alCheckRangeb(ALboolean b) {
	switch(b) {
		case AL_TRUE:
		case AL_FALSE:
			return AL_TRUE;
		default:
			break;
	}

	return AL_FALSE;
}

/*
 * ALboolean _alIsZeroVector( const ALfloat *fv1 )
 *
 * Returns true if fv1 == { 0.0f, 0.0f, 0.0f }
 */
ALboolean _alIsZeroVector(const ALfloat *fv)
{
	if(fv[0] != 0.0f) {
		return AL_FALSE;
	}

	if(fv[1] != 0.0f) {
		return AL_FALSE;
	}

	if(fv[2] != 0.0f) {
		return AL_FALSE;
	}

	return AL_TRUE;
}

/*
 *  _al_RAWFORMAT( ALenum format )
 *
 *  Returns AL_TRUE if format is an openal format specifying raw pcm data,
 *  AL_FALSE otherwise.
 */
ALboolean _al_RAWFORMAT(ALenum format)
{
	switch(format) {
		case AL_FORMAT_MONO16:
		case AL_FORMAT_MONO8:
		case AL_FORMAT_STEREO16:
		case AL_FORMAT_STEREO8:
		case AL_FORMAT_QUAD16_LOKI:
		case AL_FORMAT_QUAD8_LOKI:
			return AL_TRUE;
		default:
			break;
	}

	return AL_FALSE;
}

/*
 * _alGetBitsFromFormat( ALenum format )
 *
 * Returns the number of bits per sample for the given format.
 */
ALbyte _alGetBitsFromFormat(ALenum format)
{
	switch(format) {
		case AL_FORMAT_MONO16:
		case AL_FORMAT_STEREO16:
		case AL_FORMAT_QUAD16_LOKI:
		case AL_FORMAT_IMA_ADPCM_MONO16_EXT:
		case AL_FORMAT_IMA_ADPCM_STEREO16_EXT:
			return 16;
			break;
		case AL_FORMAT_MONO8:
		case AL_FORMAT_STEREO8:
		case AL_FORMAT_QUAD8_LOKI:
			return 8;
			break;
	}

	assert(0);

	return -1;
}

/*
 * _alSmallestPowerOfTwo( ALuint num )
 *
 * Returns smallest power of two large that meets or exceeds num.
 */
ALuint _alSmallestPowerOfTwo( ALuint num )
{
	ALuint retval = 1;

	while( retval < num ) {
		retval <<= 1;
	}

	return retval;
}

/*
 * _alIsFinite( ALfloat v )
 *
 * Returns AL_TRUE if v is a finite, non NaN value, AL_FALSE otherwise.
 */
ALboolean _alIsFinite( ALfloat v )
{
	/* skip infinite test for now */
	if(v == v) {
		return AL_TRUE;
	}

	return AL_FALSE;
}


/*
 * Returns smallest power of two that meets or exceeds num.
 */
ALuint
_alSpot( ALuint num )
{
	ALuint retval = 0;
	num >>= 1;
	while(num) {
		retval++;
		num >>= 1;
	}
	return retval;
}
