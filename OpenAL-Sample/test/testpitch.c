#include "testlib.h"

#define WAVEFILE "sample.wav"

static void iterate( void );

static ALuint movingSource = 0;

static time_t start;
static void *cc;		/* al context */

static void iterate( void )
{
/*
	static float pitch = 1.0;

	pitch -= .0021;

	alSourcef( movingSource, AL_PITCH, pitch );
*/

	microSleep( 80000 );
}

static void init( const ALbyte *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat front[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	ALuint boom;

	start = time( NULL );

	alListenerfv( AL_POSITION, zeroes );
	/* alListenerfv(AL_VELOCITY, zeroes ); */
	alListenerfv( AL_ORIENTATION, front );

	boom = CreateBufferFromFile( fname );

	alGenSources( 1, &movingSource );

	alSourcef( movingSource, AL_GAIN, 0.40 );
	alSourcei( movingSource, AL_BUFFER, boom );
	alSourcei( movingSource, AL_LOOPING, AL_TRUE );
}

int main( int argc, char *argv[] )
{
	ALCdevice *device;
	time_t shouldend;
	int attributeList[] = { ALC_FREQUENCY, 22050,
		0
	};

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

	testInitWithoutContext( &argc, argv );

	init( ( const ALbyte * ) ( ( argc == 1 ) ? WAVEFILE : argv[1] ) );

	alSourcef( movingSource, AL_PITCH, 1.15 );

	alSourcePlay( movingSource );

	shouldend = time( NULL );
	while( ( shouldend - start ) <= 20 ) {
		iterate(  );

		shouldend = time( NULL );
		if( ( shouldend - start ) > 20 ) {
			alSourceStop( movingSource );
		}
		sleep( 1 );
	}

	testExit();

	alcDestroyContext( cc );
	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
