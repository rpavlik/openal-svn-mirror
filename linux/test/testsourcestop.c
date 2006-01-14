#include "testlib.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define WAVEFILE      "sample.wav"
#define NUMSOURCES    3000

static ALuint multis[NUMSOURCES] = { 0 };

static time_t start;
static ALuint boom;

static void init( const ALbyte *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat back[] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat front[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat position[] = { 2.0f, 0.0f, -4.0f };
	int i;

	alListenerfv( AL_POSITION, zeroes );
	alListenerfv( AL_VELOCITY, zeroes );
	alListenerfv( AL_ORIENTATION, front );

	boom = CreateBufferFromFile( fname );

	alGenSources( NUMSOURCES, multis );

	alSourcefv( multis[0], AL_POSITION, position );
	alSourcefv( multis[0], AL_VELOCITY, zeroes );
	alSourcefv( multis[0], AL_ORIENTATION, back );
	alSourcei( multis[0], AL_BUFFER, boom );

	for ( i = 1; i < NUMSOURCES; i++ ) {
		position[0] = -2.0f * i;
		position[1] = 0.0f;
		position[2] = -4.0 * i;

		alSourcefv( multis[i], AL_POSITION, position );
		alSourcefv( multis[i], AL_VELOCITY, zeroes );
		alSourcefv( multis[i], AL_ORIENTATION, back );
		alSourcei( multis[i], AL_BUFFER, boom );
	}
}

int main( int argc, char *argv[] )
{
	time_t shouldend;

	start = time( NULL );
	shouldend = time( NULL );

	testInit( &argc, argv );

	getExtensionEntries(  );

	init( ( const ALbyte * ) ( ( argc == 1 ) ? WAVEFILE : argv[1] ) );

	palBombOnError(  );

#if 0
	alSourceStop( multis[0] );
	fprintf( stderr, "Play a source\n" );
	alSourcePlay( multis[0] );
	fprintf( stderr, "Stop previous source\n" );
	alSourceStop( multis[0] );
	fprintf( stderr, "Stop a stopped source\n" );
	alSourceStop( multis[0] );
	fprintf( stderr, "Play a stopped source\n" );
	alSourcePlay( multis[0] );
	fprintf( stderr, "Play a playing source\n" );
	alSourcePlay( multis[0] );
	fprintf( stderr, "Delete a playing Source\n" );
	alDeleteSources( 1, multis );
	fprintf( stderr, "Play a deleted source\n" );
	alSourcePlay( multis[0] );
#endif

	alGetError(  );

	fprintf( stderr, "play\n" );
	alSourcePlay( multis[0] );
	sleep( 2 );
	fprintf( stderr, "delete\n" );
	alDeleteBuffers( 1, &boom );
	fprintf( stderr, "Checking deferred deletion (shouldn't be valid)\n" );
	if( alIsBuffer( boom ) ) {
		fprintf( stderr, "deleted buffer is valid buffer\n" );
	} else {
		fprintf( stderr, "deleted buffer is not valid buffer\n" );
	}
	sleep( 2 );
	fprintf( stderr, "stop\n" );
	alSourceStop( multis[0] );
	sleep( 2 );
	fprintf( stderr, "Checking after source stop (shouldn't be valid)\n" );
	if( alIsBuffer( boom ) ) {
		fprintf( stderr, "deleted buffer is valid buffer\n" );
	} else {
		fprintf( stderr, "deleted buffer is not valid buffer\n" );
	}
	fprintf( stderr, "play again: prepare to hose\n" );
	alSourcePlay( multis[0] );
	fprintf( stderr, "error: %s\n", alGetString( alGetError(  ) ) );
	sleep( 2 );

	testExit();

	return EXIT_SUCCESS;
}
