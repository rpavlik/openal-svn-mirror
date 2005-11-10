#include "testlib.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define WAVEFILE      "boom.wav"
#define NUMSOURCES    1

static void iterate( void );

static ALuint multis;

static ALCcontext *context;
static void *wave = NULL;

static void iterate( void )
{
	fprintf( stderr, "NOW\n" );
	alSourcePlay( multis );
	fprintf( stderr, "OVER\n" );

	microSleep( 1000000 );
}

static void init( const ALbyte *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat back[] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat position[] = { 2.0f, 0.0f, -4.0f };
	ALuint boom;
	ALsizei size;
	ALsizei freq;
	ALsizei format;
	ALboolean loop;

	alListenerfv( AL_POSITION, zeroes );
	alListenerfv( AL_VELOCITY, zeroes );
	/* alListenerfv(AL_ORIENTATION, front ); */

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

	alSourcefv( multis, AL_POSITION, position );
	alSourcefv( multis, AL_VELOCITY, zeroes );
	alSourcefv( multis, AL_ORIENTATION, back );
	alSourcef( multis, AL_GAIN, 1.0f );

	alSourceQueueBuffers( multis, 1, &boom );
	alSourceQueueBuffers( multis, 1, &boom );
	alSourcei( multis, AL_BUFFER, boom );
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

	iterate(  );

	while( sourceIsPlaying( multis ) == AL_TRUE ) {
		microSleep( 1000000 );
	}

	alcDestroyContext( context );
	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
