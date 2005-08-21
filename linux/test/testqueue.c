#include "testlib.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define WAVEFILE      "makepcm.wav"
#define NUMSOURCES    1

static void start( void );

static ALuint multis;

static ALCcontext *context;
static void *wave = NULL;

static void start( void )
{
	alSourcePlay( multis );
}

static void init( const ALbyte *fname )
{
	ALuint boom;
	ALsizei size;
	ALsizei freq;
	ALsizei format;
	ALboolean loop;

	alGenBuffers( 1, &boom );

	alutLoadWAVFile( fname, &format, &wave, &size, &freq, &loop );
	if( wave == NULL ) {
		fprintf( stderr, "Could not include %s\n",
			 ( const char * ) fname );
		exit( EXIT_FAILURE );
	}

	alBufferData( boom, format, wave, size, freq );
	free( wave );		/* openal makes a local copy of wave data */

	alGenSources( NUMSOURCES, &multis );

	alSourcei( multis, AL_LOOPING, AL_FALSE );
	alSourcef( multis, AL_GAIN_LINEAR_LOKI, 1.0 );

	alSourceQueueBuffers( multis, 1, &boom );
	alSourceQueueBuffers( multis, 1, &boom );
	alSourceQueueBuffers( multis, 1, &boom );
	alSourceQueueBuffers( multis, 1, &boom );
	alSourceQueueBuffers( multis, 1, &boom );
}

int main( int argc, char *argv[] )
{
	ALCdevice *device;

	device = alcOpenDevice( NULL );
	if( device == NULL ) {
		return EXIT_FAILURE;
	}

	/* Initialize ALUT. */
	context = alcCreateContext( device, NULL );
	if( context == NULL ) {
		alcCloseDevice( device );

		return EXIT_FAILURE;
	}

	alcMakeContextCurrent( context );

	init( ( const ALbyte * ) ( ( argc == 1 ) ? WAVEFILE : argv[1] ) );

	start(  );

	while( sourceIsPlaying( multis ) == AL_TRUE ) {
		microSleep( 1000000 );
		microSleep( 1000000 );
		microSleep( 1000000 );
		microSleep( 1000000 );
	}

	alcDestroyContext( context );
	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
