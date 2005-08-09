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
static void init( char *fname );
static void cleanup( void );

static ALuint multis;

static ALCcontext *context_id;
static void *wave = NULL;

static void start( void )
{
	alSourcePlay( multis );

	return;
}

static void init( char *fname )
{
	ALuint boom;
	ALsizei size;
	ALsizei freq;
	ALsizei format;
	ALboolean loop;

	alGenBuffers( 1, &boom );

	alutLoadWAVFile( ( ALbyte * ) fname, &format, &wave, &size, &freq,
			 &loop );
	if( wave == NULL ) {
		fprintf( stderr, "Could not include %s\n", fname );
		exit( 1 );
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

	return;
}

void cleanup( void )
{
	alcDestroyContext( context_id );
#ifdef JLIB
	jv_check_mem(  );
#endif
}

int main( int argc, char *argv[] )
{
	ALCdevice *dev;

	dev = alcOpenDevice( NULL );
	if( dev == NULL ) {
		return 1;
	}

	/* Initialize ALUT. */
	context_id = alcCreateContext( dev, NULL );
	if( context_id == NULL ) {
		alcCloseDevice( dev );

		return 1;
	}

	alcMakeContextCurrent( context_id );

	if( argc == 1 ) {
		init( WAVEFILE );
	} else {
		init( argv[1] );
	}

	start(  );

	while( SourceIsPlaying( multis ) == AL_TRUE ) {
		micro_sleep( 1000000 );
		micro_sleep( 1000000 );
		micro_sleep( 1000000 );
		micro_sleep( 1000000 );
	}

	cleanup(  );

	alcCloseDevice( dev );

	return 0;
}
