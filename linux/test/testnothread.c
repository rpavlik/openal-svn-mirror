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

#define WAVEFILE   "sample.wav"
#define NUMSOURCES 7

static void cleanup( void );

static ALuint movingSource[NUMSOURCES] = { 0 };
static ALCcontext *context;
static void *wave = NULL;
static time_t start;

static void init( const ALbyte *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat back[] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat front[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat position[] = { 0.0f, 0.0f, -4.0f };
	ALuint boom;
	ALsizei size;
	ALsizei freq;
	ALsizei format;
	ALboolean loop;
	int i;

	start = time( NULL );

	alListenerfv( AL_POSITION, zeroes );
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

	alGenSources( NUMSOURCES, movingSource );

	for ( i = 0; i < NUMSOURCES; i++ ) {
		alSourcefv( movingSource[i], AL_POSITION, position );
		alSourcefv( movingSource[i], AL_ORIENTATION, back );
		alSourcei( movingSource[i], AL_BUFFER, boom );
		alSourcei( movingSource[i], AL_LOOPING, AL_TRUE );
	}
}

static void cleanup( void )
{
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
	int attributeList[] = { ALC_SYNC, AL_TRUE, 0 };
	time_t shouldend;
	int i, j;

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

	init( ( const ALbyte * ) ( ( argc == 1 ) ? WAVEFILE : argv[1] ) );

	/*
	 * First, play one sources,
	 * then, play two sources and so on until NUMSOURCES are playing.
	 */
	for ( i = 1; i <= NUMSOURCES; i++ ) {
		fprintf( stderr, "Playing %d source(s)\n", i );

		alSourceStopv( i, movingSource );

		for ( j = 0; j < i; j++ ) {
			alSourcePlay( movingSource[j] );
			alcProcessContext( context );
			microSleep( 40000 );
		}

		while( 1 ) {
			alcProcessContext( context );

			shouldend = time( NULL );
			if( ( shouldend - start ) > 40 ) {
				break;
			}
		}

		start = time( NULL );
	}

	alcDestroyContext( context );
	alcCloseDevice( device );

	cleanup(  );

	return EXIT_SUCCESS;
}
