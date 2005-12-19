#include "testlib.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef M_PI
#define M_PI 3.14
#endif				/* M_PI */

#define WAVEFILE "sample.wav"

#define TORAD(d) (d * (M_PI  / 180.0))
#define TODEG(r) (r * (180.0 / M_PI))

static void iterate( void );

static ALuint rightSid;

static ALCcontext *context;
static void *wave = NULL;

static void iterate( void )
{
	ALfloat orientation[] = { 0.0f, 0.0f, -1.0f,
		0.0f, 1.0f, 0.0f
	};
	ALfloat *at = orientation;
	ALfloat *up = &orientation[3];
	static ALint angle = 0;

	/*
	 * rotate up vector about at vector by angle degrees.
	 */
	_RotatePointAboutAxis( TORAD( angle ), up, at );

	fprintf( stderr, "orientation: \n\tAT(%f %f %f)\n\tUP(%f %f %f)\n",
		 orientation[0], orientation[1], orientation[2],
		 orientation[3], orientation[4], orientation[5] );

	fprintf( stderr, "angle = %d\n", angle );

	angle += 15;		/* increment fifeteen degrees degree */

	alListenerfv( AL_ORIENTATION, orientation );

	microSleep( 900000 );
}

static void init( const ALbyte *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat position[] = { 0.0f, 0.0f, -4.0f };
	ALuint boom;
	ALsizei size;
	ALsizei freq;
	ALsizei format;
	ALboolean loop;

	alListenerfv( AL_POSITION, zeroes );

	alGenBuffers( 1, &boom );

	alutLoadWAVFile( fname, &format, &wave, &size, &freq, &loop );
	if( wave == NULL ) {
		fprintf( stderr, "Could not include %s\n",
			 ( const char * ) fname );
		exit( EXIT_FAILURE );
	}

	alBufferData( boom, format, wave, size, freq );
	free( wave );		/* openal makes a local copy of wave data */

	alGenSources( 1, &rightSid );

	alSourcefv( rightSid, AL_POSITION, position );
	alSourcei( rightSid, AL_BUFFER, boom );
	alSourcei( rightSid, AL_LOOPING, AL_TRUE );
}

int main( int argc, char *argv[] )
{
	ALCdevice *device;
	time_t start;
	time_t shouldend;

	start = time( NULL );
	shouldend = time( NULL );

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

	alSourcePlay( rightSid );

	while( shouldend - start < 20 ) {
		shouldend = time( NULL );

		iterate(  );
	}

	alcDestroyContext( context );
	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
