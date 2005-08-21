#include "testlib.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define WAVEFILE "fire2.wav"

static void *wave = NULL;
static ALCcontext *context = NULL;
static ALuint movingSource = 0;

static void init( const ALbyte *fname )
{
	ALfloat weirdpos[] = { 300.0f, 0.0f, 0.0f };
	ALfloat position[] = { 0.0f, 0.0f, 4.0f };
	ALuint boom;
	ALsizei size;
	ALsizei freq;
	ALsizei format;
	ALboolean loop;

	alListenerfv( AL_POSITION, weirdpos );

	alGenBuffers( 1, &boom );

	alutLoadWAVFile( fname, &format, &wave, &size, &freq, &loop );
	if( wave == NULL ) {
		fprintf( stderr, "Could not include %s\n",
			 ( const char * ) fname );
		exit( EXIT_FAILURE );
	}

	alBufferData( boom, format, wave, size, freq );
	free( wave );		/* openal makes a local copy of wave data */

	alGenSources( 1, &movingSource );

	alSourcei( movingSource, AL_BUFFER, boom );
	alSourcei( movingSource, AL_LOOPING, AL_TRUE );
	alSourcei( movingSource, AL_STREAMING, AL_TRUE );
	alSourcei( movingSource, AL_SOURCE_RELATIVE, AL_TRUE );
	alSourcefv( movingSource, AL_POSITION, position );
	alSourcef( movingSource, AL_PITCH, 1.00 );
}

int main( int argc, char *argv[] )
{
	ALCdevice *device;
	int attributeList[] = { ALC_FREQUENCY, 22050, 0 };

	time_t shouldend, start;
	ALint test = AL_FALSE;

	device = alcOpenDevice( NULL );
	if( device == NULL ) {
		return EXIT_FAILURE;
	}

	/* Initialize ALUT. */
	context = alcCreateContext( device, attributeList );
	if( context == NULL ) {
		alcCloseDevice( device );

		exit( EXIT_FAILURE );
	}

	alcMakeContextCurrent( context );

	getExtensionEntries(  );

	palBombOnError(  );

	init( ( const ALbyte * ) ( ( argc == 1 ) ? WAVEFILE : argv[1] ) );

	fprintf( stderr, "Loop for 4 seconds\n" );

	alSourcePlay( movingSource );

	shouldend = start = time( NULL );

	while( shouldend - start <= 4 ) {
		shouldend = time( NULL );

		microSleep( 1000000 );
	}
	alSourceStop( movingSource );

	test = -1;

	alGetSourceiv( movingSource, AL_LOOPING, &test );
	fprintf( stderr, "is looping?  getsi says %s\n",
		 ( test == AL_TRUE ) ? "AL_TRUE" : "AL_FALSE" );

	/* part the second */
	fprintf( stderr, "Play once\n" );
	microSleep( 1000000 );

	alSourcei( movingSource, AL_LOOPING, AL_FALSE );
	alSourcePlay( movingSource );

	do {
		microSleep( 1000000 );
	}
	while( sourceIsPlaying( movingSource ) );

	alcDestroyContext( context );
	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
