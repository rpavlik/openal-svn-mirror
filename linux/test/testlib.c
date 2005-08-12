#include "testlib.h"

#include <AL/al.h>

#include <stdio.h>
#include <stdlib.h>
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
	return 0;
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
	GP( PFNALUTLOADRAW_ADPCMDATAPROC, palutLoadRAW_ADPCMData,
	    "alutLoadRAW_ADPCMData_LOKI" );
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
