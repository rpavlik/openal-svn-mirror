#include "testlib.h"

#define WAVEFILE      "boom.wav"
#define NUMSOURCES    1

static void iterate( void );

static ALuint multis;

static void iterate( void )
{
	fprintf( stderr, "NOW\n" );
	alSourcePlay( multis );
	fprintf( stderr, "OVER\n" );

	microSleep( 1000000 );
}

static void init( const ALbyte *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat back[] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat position[] = { 2.0f, 0.0f, -4.0f };
	ALuint boom;

	alListenerfv( AL_POSITION, zeroes );
	alListenerfv( AL_VELOCITY, zeroes );
	/* alListenerfv(AL_ORIENTATION, front ); */

	boom = CreateBufferFromFile( fname );

	alGenSources( NUMSOURCES, &multis );

	alSourcefv( multis, AL_POSITION, position );
	alSourcefv( multis, AL_VELOCITY, zeroes );
	alSourcefv( multis, AL_ORIENTATION, back );
	alSourcef( multis, AL_GAIN, 1.0 );

	alSourceQueueBuffers( multis, 1, &boom );
	alSourceQueueBuffers( multis, 1, &boom );
}

int main( int argc, char *argv[] )
{
	testInit( &argc, argv );

	init( ( const ALbyte * ) ( ( argc == 1 ) ? WAVEFILE : argv[1] ) );

	iterate(  );

	while( sourceIsPlaying( multis ) == AL_TRUE ) {
		microSleep( 1000000 );
	}

	testExit();

	return EXIT_SUCCESS;
}
