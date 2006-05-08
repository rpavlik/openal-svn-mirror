#include "testlib.h"

#include <AL/al.h>
#include <AL/alext.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <math.h>

#define NUMSOURCES 10
#define WAVEFILE   "boom.wav"

static void iterate( void );

static ALuint movingSource[NUMSOURCES];

static ALCcontext *context;
static time_t start;

extern int mixer_iterate( void *dummy );

static void iterate( void )
{
	static float f = 0;
	float g;

	f += .001;

	g = ( sin( f ) + 1.0 ) / 2.0;

/*
	fprintf(stderr, "AL_PITCH = %f\n", g);
	alSourcef(movingSource, AL_PITCH, g);
	*/
	alcProcessContext( context );
}

static void init( const ALbyte *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat front[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	ALuint boom;
	int i;

	start = time( NULL );

	alListenerfv( AL_POSITION, zeroes );
	alListenerfv( AL_VELOCITY, zeroes );
	alListenerfv( AL_ORIENTATION, front );

	boom = CreateBufferFromFile( fname );

	alGenSources( NUMSOURCES, movingSource );

	for ( i = 0; i < NUMSOURCES; i++ ) {
		alSourcef( movingSource[i], AL_GAIN, 0.25 );
		alSourcei( movingSource[i], AL_BUFFER, boom );
		alSourcei( movingSource[i], AL_LOOPING, AL_TRUE );
		alSourcef( movingSource[i], AL_PITCH, 1.00 );
	}
}

int main( int argc, char *argv[] )
{
	ALCdevice *device;
	int attributeList[] = {
		ALC_SYNC, AL_TRUE,
		0
	};
	time_t shouldend;
	int i;

	device = alcOpenDevice( NULL );
	if( device == NULL ) {
		return EXIT_FAILURE;
	}

	/* Initialize ALUT. */
	context = alcCreateContext( device, attributeList );
	if( context == NULL ) {
		alcCloseDevice( device );

		return EXIT_FAILURE;
	}

	alcMakeContextCurrent( context );

	testInitWithoutContext( &argc, argv );

	init( ( const ALbyte * ) ( ( argc == 1 ) ? WAVEFILE : argv[1] ) );

	for ( i = 0; i < NUMSOURCES; i++ ) {
		alSourcef( movingSource[i], AL_PITCH, 0.45 );
	}

	alSourcePlayv( NUMSOURCES, movingSource );

	shouldend = time( NULL );

	while( ( shouldend - start ) < 300 ) {
		shouldend = time( NULL );

		iterate(  );
	}

	testExit();

	alcDestroyContext( context );
	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
