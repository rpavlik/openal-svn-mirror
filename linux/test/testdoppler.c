/* testdoppler.c - move the source towards, through, and away from
   the listener, while accelerating constantly. */

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

#define WAVEFILE "sample.wav"

static void iterate( void );

static ALuint movingSource = 0;

static void *wave = NULL;
static time_t start;
static void *cc;		/* al context */

static ALfloat srcposition[] = { 0.1f, 0.0f, -4.0f };

static void iterate( void )
{
	static ALfloat speed[3] = { 0.0, 0.0, 0.0 };

	speed[2] += .0005;

	srcposition[0] += speed[0];
	srcposition[1] += speed[1];
	srcposition[2] += speed[2];

	alSourcefv( movingSource, AL_VELOCITY, speed );
	alSourcefv( movingSource, AL_POSITION, srcposition );
	microSleep( 20000 );
}

static void init( const ALbyte *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat back[] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat front[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	ALuint boom;
	ALsizei size;
	ALsizei freq;
	ALsizei format;
	ALboolean loop;

	start = time( NULL );

	alListenerfv( AL_POSITION, zeroes );
	alListenerfv( AL_VELOCITY, zeroes );
	alListenerfv( AL_ORIENTATION, front );

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

	alSourcefv( movingSource, AL_POSITION, srcposition );
	alSourcefv( movingSource, AL_VELOCITY, zeroes );
	alSourcefv( movingSource, AL_DIRECTION, back );
	alSourcei( movingSource, AL_BUFFER, boom );
	alSourcei( movingSource, AL_LOOPING, AL_TRUE );
}

int main( int argc, char *argv[] )
{
	ALCdevice *device;
	time_t shouldend;

	device = alcOpenDevice( NULL );
	if( device == NULL ) {
		return EXIT_FAILURE;
	}

	cc = alcCreateContext( device, NULL );
	if( cc == NULL ) {
		alcCloseDevice( device );

		return EXIT_FAILURE;
	}

	alcMakeContextCurrent( cc );

	getExtensionEntries(  );

	palBombOnError(  );

	init( ( const ALbyte * ) ( ( argc == 1 ) ? WAVEFILE : argv[1] ) );

	alSourcePlay( movingSource );

	shouldend = time( NULL );
	while( ( shouldend - start ) <= 10 ) {
		iterate(  );

		shouldend = time( NULL );
		if( ( shouldend - start ) > 10 ) {
			alSourceStop( movingSource );
		}
	}

	alcDestroyContext( cc );
	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
