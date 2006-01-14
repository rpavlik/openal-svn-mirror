#include "testlib.h"

#include <AL/al.h>
#include <AL/alut.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>

void microSleep( unsigned int n )
{
	Sleep( n / 1000 );
}

#elif defined(__MORPHOS__)

#include <clib/amiga_protos.h>

unsigned sleep( unsigned n )
{
	TimeDelay( UNIT_MICROHZ, 0, n * 1000 );
	return EXIT_SUCCESS;
}

void microSleep( unsigned int n )
{
	TimeDelay( UNIT_MICROHZ, n / 1000000, n % 1000000 );
}

#elif defined(__APPLE__)

void microSleep( unsigned int n )
{
	usleep( n );
}

#else

void microSleep( unsigned int n )
{
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = n;
	select( 0, NULL, NULL, NULL, &tv );
}

#endif				/* _WIN32 */

#define GP(type,var,name) \
        var = (type)alGetProcAddress((const ALchar*) name); \
	if( var == NULL ) { \
		fprintf( stderr, "Could not get %s extension entry\n", name ); \
		exit( EXIT_FAILURE ); \
	}

PFNALCGETAUDIOCHANNELPROC palcGetAudioChannel;
PFNALCSETAUDIOCHANNELPROC palcSetAudioChannel;

PFNALREVERBSCALEPROC palReverbScale;
PFNALREVERBDELAYPROC palReverbDelay;

PFNALBOMBONERRORPROC palBombOnError;

PFNALBUFFERIPROC palBufferi;
PFNALBUFFERWRITEDATAPROC palBufferWriteData;
PFNALBUFFERAPPENDWRITEDATAPROC palBufferAppendWriteData;

PFNALCAPTUREINITPROC palCaptureInit;
PFNALCAPTUREDESTROYPROC palCaptureDestroy;
PFNALCAPTURESTARTPROC palCaptureStart;
PFNALCAPTURESTOPPROC palCaptureStop;
PFNALCAPTUREGETDATAPROC palCaptureGetData;

PFNALGENSTREAMINGBUFFERSPROC palGenStreamingBuffers;
PFNALUTLOADRAW_ADPCMDATAPROC palutLoadRAW_ADPCMData;

void getExtensionEntries( void )
{
	GP( PFNALCGETAUDIOCHANNELPROC, palcGetAudioChannel,
	    "alcGetAudioChannel_LOKI" );
	GP( PFNALCSETAUDIOCHANNELPROC, palcSetAudioChannel,
	    "alcSetAudioChannel_LOKI" );

	GP( PFNALREVERBSCALEPROC, palReverbScale, "alReverbScale_LOKI" );
	GP( PFNALREVERBDELAYPROC, palReverbDelay, "alReverbDelay_LOKI" );

	GP( PFNALBOMBONERRORPROC, palBombOnError, "alBombOnError_LOKI" );

	GP( PFNALBUFFERIPROC, palBufferi, "alBufferi_LOKI" );
	GP( PFNALBUFFERWRITEDATAPROC, palBufferWriteData,
	    "alBufferWriteData_LOKI" );
	GP( PFNALBUFFERAPPENDWRITEDATAPROC, palBufferAppendWriteData,
	    "alBufferAppendWriteData_LOKI" );

	GP( PFNALCAPTUREINITPROC, palCaptureInit, "alCaptureInit_EXT" );
	GP( PFNALCAPTUREDESTROYPROC, palCaptureDestroy,
	    "alCaptureDestroy_EXT" );
	GP( PFNALCAPTURESTARTPROC, palCaptureStart, "alCaptureStart_EXT" );
	GP( PFNALCAPTURESTOPPROC, palCaptureStop, "alCaptureStop_EXT" );
	GP( PFNALCAPTUREGETDATAPROC, palCaptureGetData,
	    "alCaptureGetData_EXT" );

	GP( PFNALGENSTREAMINGBUFFERSPROC, palGenStreamingBuffers,
	    "alGenStreamingBuffers_LOKI" );
	/*GP( PFNALUTLOADRAW_ADPCMDATAPROC, palutLoadRAW_ADPCMData,
	    "alutLoadRAW_ADPCMData_LOKI" );*/
}

ALboolean sourceIsPlaying( ALuint sid )
{
	ALint state;
	if( alIsSource( sid ) == AL_FALSE ) {
		return AL_FALSE;
	}
	alGetSourceiv( sid, AL_SOURCE_STATE, &state );
	return ( ( state == AL_PLAYING )
		 || ( state == AL_PAUSED ) ) ? AL_TRUE : AL_FALSE;
}

void _RotatePointAboutAxis( const ALfloat angle, ALfloat *point,
                              const ALfloat *axis ) {
	ALfloat m[3][3];
	ALfloat pm0, pm1, pm2;

	ALfloat s = sin( angle );
	ALfloat c = cos( angle );
	ALfloat t = 1.0f - c;

	ALfloat x = axis[0];
	ALfloat y = axis[1];
	ALfloat z = axis[2];

	if(angle == 0.0f) {
		/* FIXME: use epsilon? */
		return;
	}

	m[0][0] = t * x * x + c;
	m[0][1] = t * x * y - s * z;
	m[0][2] = t * x * z + s * y;

	m[1][0] = t * x * y + s * z;
	m[1][1] = t * y * y + c;
	m[1][2] = t * y * z - s * x;

	m[2][0] = t * x * z - s * y;
	m[2][1] = t * y * z + s * x;
	m[2][2] = t * z * z + c;

	pm0 = point[0];
	pm1 = point[1];
	pm2 = point[2];

	/*
	 * pm * m
	 */
	point[0] = pm0 * m[0][0] + pm1 * m[1][0] + pm2 * m[2][0];
	point[1] = pm0 * m[0][1] + pm1 * m[1][1] + pm2 * m[2][1];
	point[2] = pm0 * m[0][2] + pm1 * m[1][2] + pm2 * m[2][2];

	return;
}

ALuint CreateBufferFromFile( const char *fileName )
{
	ALuint buffer = alutCreateBufferFromFile( fileName );
	if (buffer == AL_NONE) {
		ALenum error = alutGetError();
		fprintf( stderr, "Error loading file: '%s'\n",
			alutGetErrorString( error ) );
		alutExit();
		exit( EXIT_FAILURE );
	}

	return buffer;
}

void testInit( int *argcp, char **argv )
{
	if (!alutInit( argcp, argv )) {
		ALenum error = alutGetError();
		fprintf( stderr, "%s\n", alutGetErrorString( error ) );
		exit( EXIT_FAILURE );
	}
}

void testInitWithoutContext( int *argcp, char **argv )
{
	if (!alutInitWithoutContext( argcp, argv )) {
		ALenum error = alutGetError();
		fprintf( stderr, "%s\n", alutGetErrorString( error ) );
		exit( EXIT_FAILURE );
	}
}

void testExit( void )
{
	if (!alutExit()) {
		ALenum error = alutGetError();
		fprintf( stderr, "%s\n", alutGetErrorString( error ) );
		exit( EXIT_FAILURE );
	}
}
