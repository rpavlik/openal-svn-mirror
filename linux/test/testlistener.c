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

#define WAVEFILE "boom.wav"

#define ALMAXDISTANCE 60.0f

static void iterate( void );
static void cleanup( void );

static ALuint multis[2] = { 0 };

static ALCcontext *context;
static void *wave = NULL;

static void iterate( void )
{
	static ALfloat lispos[] = { 0.0, 0.0, 0.0 };
	static float change_factor = 0.85;

	lispos[0] += change_factor;

	if( lispos[0] < -ALMAXDISTANCE ) {
		lispos[0] = -ALMAXDISTANCE;
		change_factor *= -1.0;
	} else if( lispos[0] > ALMAXDISTANCE ) {
		lispos[0] = ALMAXDISTANCE;
		change_factor *= -1.0;
	}

	fprintf( stderr, "lispos = (%f %f %f)\n", lispos[0], lispos[1],
		 lispos[2] );

	alListenerfv( AL_POSITION, lispos );

	microSleep( 100000 );
}

static void init( const ALbyte *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat front[] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat position[] = { 20.0f, 0.0f, 5.0f };
	ALuint boom;
	ALsizei size;
	ALsizei freq;
	ALsizei format;
	ALboolean loop;

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

	alGenSources( 2, multis );

	alSourcefv( multis[0], AL_POSITION, position );
	alSourcei( multis[0], AL_BUFFER, boom );
	alSourcei( multis[0], AL_LOOPING, AL_TRUE );

	position[0] *= -1.0f;
	alSourcefv( multis[1], AL_POSITION, position );
	alSourcei( multis[1], AL_BUFFER, boom );
	alSourcei( multis[1], AL_LOOPING, AL_TRUE );
}

void cleanup( void )
{
	alcDestroyContext( context );

#ifdef JLIB
	jv_check_mem(  );
#endif
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

	alSourcePlay( multis[0] );
	microSleep( 80000 );
	alSourcePlay( multis[1] );

	while( shouldend - start < 20 ) {
		shouldend = time( NULL );

		iterate(  );
	}

	cleanup(  );

	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
