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

#define WAVEFILE "boom.wav"
#define SCALE 5

static void iterate( void );
static void cleanup( void );

static ALuint movingSource = 0;

static void *wave = NULL;
static time_t start;
static void *cc;		/* al context */

static void iterate( void )
{
	static ALfloat newgain = 1.0;
	static int direction = -1;

	newgain += ( 1.0 / SCALE ) * direction;

	if( newgain < 0.0 ) {
		newgain = 0.0;

		direction = -direction;
	}

	if( newgain >= 1.0 ) {
		newgain = 1.0;

		direction = -direction;

	}

	alSourcef( movingSource, AL_GAIN_LINEAR_LOKI, newgain );

	fprintf( stderr, "GAIN_LINEAR = %f\n", newgain );

	microSleep( 500000 );
}

static void init( const ALbyte *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat back[] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat side[] = { 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f };
	ALfloat position[] = { 0.0f, 0.0f, -4.0f };
	ALuint boom;
	ALsizei size;
	ALsizei freq;
	ALsizei format;
	ALboolean loop;

	start = time( NULL );

	alListenerfv( AL_POSITION, zeroes );
	alListenerfv( AL_VELOCITY, zeroes );
	alListenerfv( AL_ORIENTATION, side );

	alGenBuffers( 1, &boom );

	alutLoadWAVFile( fname, &format, &wave, &size, &freq, &loop );
	if( wave == NULL ) {
		fprintf( stderr, "Could not load %s\n",
			 ( const char * ) fname );
		exit( EXIT_FAILURE );
	}

	alBufferData( boom, format, wave, size, freq );
	free( wave );		/* openal makes a local copy of wave data */

	alGenSources( 1, &movingSource );

	alSourcefv( movingSource, AL_POSITION, position );
	alSourcefv( movingSource, AL_VELOCITY, zeroes );
	alSourcefv( movingSource, AL_ORIENTATION, back );
	alSourcei( movingSource, AL_BUFFER, boom );
	alSourcei( movingSource, AL_LOOPING, AL_TRUE );
}

int main( int argc, char *argv[] )
{
	ALCdevice *device;
	time_t shouldend;
	int attributeList[3];

	attributeList[0] = ALC_FREQUENCY;
	attributeList[1] = 22050;
	attributeList[2] = 0;

	device = alcOpenDevice( NULL );
	if( device == NULL ) {
		return EXIT_FAILURE;
	}

	cc = alcCreateContext( device, attributeList );
	if( cc == NULL ) {
		alcCloseDevice( device );

		return EXIT_FAILURE;
	}

	alcMakeContextCurrent( cc );

	init( ( const ALbyte * ) ( ( argc == 1 ) ? WAVEFILE : argv[1] ) );

	alSourcePlay( movingSource );

	shouldend = time( NULL );
	while( ( shouldend - start ) <= 10 ) {
		iterate(  );

		shouldend = time( NULL );
	}

	alcDestroyContext( cc );
	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
