#include "testlib.h"

#define WAVEFILE      "makepcm.wav"
#define NUMSOURCES    1

static void start( void );

static ALuint multis;

static void start( void )
{
	alSourcePlay( multis );
}

static void init( const ALbyte *fname )
{
	ALuint boom;

	boom = CreateBufferFromFile( fname );

	alGenSources( NUMSOURCES, &multis );

	alSourcei( multis, AL_LOOPING, AL_FALSE );
	alSourcef( multis, AL_GAIN, 1.0 );

	alSourceQueueBuffers( multis, 1, &boom );
	alSourceQueueBuffers( multis, 1, &boom );
	alSourceQueueBuffers( multis, 1, &boom );
	alSourceQueueBuffers( multis, 1, &boom );
	alSourceQueueBuffers( multis, 1, &boom );
}

int main( int argc, char *argv[] )
{
	testInit( &argc, argv );

	init( ( const ALbyte * ) ( ( argc == 1 ) ? WAVEFILE : argv[1] ) );

	start(  );

	while( sourceIsPlaying( multis ) == AL_TRUE ) {
		microSleep( 1000000 );
		microSleep( 1000000 );
		microSleep( 1000000 );
		microSleep( 1000000 );
	}

	testExit();

	return EXIT_SUCCESS;
}
