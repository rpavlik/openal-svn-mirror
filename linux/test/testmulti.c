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

#define WAVEFILE      "boom.wav"
#define NUMSOURCES    15

static void iterate( void );
static void cleanup( void );

static ALuint multis[NUMSOURCES] = { 0 };

static ALCcontext *context;
static void *wave = NULL;

static void iterate( void )
{
	int i;

	for ( i = 0; i < NUMSOURCES; i++ ) {
		if( sourceIsPlaying( multis[i] ) != AL_TRUE ) {
			alSourcePlay( multis[i] );
		}
		microSleep( 20000 );
	}

}

static void init( const ALbyte *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat position[] = { 2.0f, 0.0f, 4.0f };
	ALuint boom;
	ALsizei size;
	ALsizei freq;
	ALsizei format;
	ALboolean loop;
	int i = 0;

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
	memset( wave, size, 0 );

	alGenSources( NUMSOURCES, multis );

	alSourcefv( multis[0], AL_POSITION, position );
	alSourcei( multis[0], AL_BUFFER, boom );
	alSourcef( multis[i], AL_MAX_GAIN, 1.0 );
	alSourcef( multis[i], AL_REFERENCE_DISTANCE, 1.0 );

	for ( i = 1; i < NUMSOURCES; i++ ) {
		position[0] = -2.0f * i;
		position[1] = 0.0;
		position[2] = -4.0f * i;

		alSourcefv( multis[i], AL_POSITION, position );
		alSourcei( multis[i], AL_BUFFER, boom );
		alSourcef( multis[i], AL_MAX_GAIN, 0 );
		alSourcef( multis[i], AL_REFERENCE_DISTANCE, 0.1 );
	}
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
	int attributeList[] = { ALC_FREQUENCY, 22050, 0 };
	time_t start;
	time_t shouldend;

	start = time( NULL );
	shouldend = time( NULL );

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

	while( shouldend - start < 30 ) {
		shouldend = time( NULL );

		iterate(  );
	}

	cleanup(  );

	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
