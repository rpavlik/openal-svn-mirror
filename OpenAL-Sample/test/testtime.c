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

static ALCdevice *device = NULL;
static ALCcontext *context;

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

	alListenerfv( AL_POSITION, zeroes );
	alListenerfv( AL_VELOCITY, zeroes );
	/* alListenerfv(AL_ORIENTATION, front ); */

	boom = CreateBufferFromFile( fname );

	alGenSources( NUMSOURCES, &multis );

	alSourcefv( multis, AL_POSITION, position );
	alSourcefv( multis, AL_VELOCITY, zeroes );
	alSourcefv( multis, AL_ORIENTATION, back );
	alSourcei( multis, AL_LOOPING, AL_FALSE );
	alSourcef( multis, AL_GAIN, 1.0 );
	alSourcei( multis, AL_BUFFER, boom );
}

int main( int argc, char *argv[] )
{
	int attributeList[] = { ALC_FREQUENCY, 22050, 0 };
#if 0
	const ALchar *devspec =
	    ( const ALchar * ) "'( ( sampling-rate 22050 ) ( devices '(null)))";
#else
	const ALchar *devspec = NULL;
#endif

	/* Initialize device and context. */
	device = alcOpenDevice( devspec );
	if( device == NULL ) {
		return EXIT_FAILURE;
	}

	context = alcCreateContext( device, attributeList );
	if( context == NULL ) {
		alcCloseDevice( device );

		return EXIT_FAILURE;
	}

	alcMakeContextCurrent( context );

	testInitWithoutContext( &argc, argv );

	init( ( const ALbyte * ) ( ( argc == 1 ) ? WAVEFILE : argv[1] ) );

	iterate(  );

	while( sourceIsPlaying( multis ) == AL_TRUE ) {
		microSleep( 1000000 );
	}

	testExit();

	alcDestroyContext( context );
	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
