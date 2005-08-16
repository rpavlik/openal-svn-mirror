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

static void iterate( void );
static void cleanup( void );

static ALuint movingSource = 0;

static void *wave = NULL;
static time_t start;
static void *cc;		/* al context */

static void iterate( void )
{
	ALfloat position[] = { 2.0f, 0.0f, -4.0f };
	static ALfloat movefactor = 10.0;
	static time_t then = 0;
	time_t now;

	now = time( NULL );

	/* Switch between left and right boom every five seconds. */
	if( now - then > 5 ) {
		then = now;

		movefactor *= -1.0;
	}

	position[0] += movefactor;
	alListenerfv( AL_POSITION, position );

	microSleep( 500000 );
}

static void init( const ALbyte *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
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
		fprintf( stderr, "Could not include %s\n",
			 ( const char * ) fname );
		exit( EXIT_FAILURE );
	}

	alBufferData( boom, format, wave, size, freq );
	free( wave );		/* openal makes a local copy of wave data */

	alGenSources( 1, &movingSource );

	alSourcefv( movingSource, AL_POSITION, position );
	alSourcefv( movingSource, AL_VELOCITY, zeroes );
	alSourcei( movingSource, AL_BUFFER, boom );
	alSourcei( movingSource, AL_LOOPING, AL_TRUE );
	alSourcei( movingSource, AL_SOURCE_RELATIVE, AL_TRUE );
}

static void cleanup( void )
{
	alcDestroyContext( cc );
#ifdef DMALLOC
	dmalloc_verify( 0 );
	dmalloc_log_unfreed(  );

#endif
#ifdef JLIB
	jv_check_mem(  );
#endif
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

	cleanup(  );

	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
