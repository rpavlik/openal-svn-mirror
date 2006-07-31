/* testdoppler.c - move the source towards, through, and away from
   the listener, while accelerating constantly. */

#include "testlib.h"

#define WAVEFILE "sample.wav"

static void iterate( void );

static ALuint movingSource = 0;

static time_t start;

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

	start = time( NULL );

	alListenerfv( AL_POSITION, zeroes );
	alListenerfv( AL_VELOCITY, zeroes );
	alListenerfv( AL_ORIENTATION, front );

	boom = CreateBufferFromFile( fname );
	
	alGenSources( 1, &movingSource );

	alSourcefv( movingSource, AL_POSITION, srcposition );
	alSourcefv( movingSource, AL_VELOCITY, zeroes );
	alSourcefv( movingSource, AL_DIRECTION, back );
	alSourcei( movingSource, AL_BUFFER, boom );
	alSourcei( movingSource, AL_LOOPING, AL_TRUE );
}

int main( int argc, char *argv[] )
{
	time_t shouldend;

	testInit( &argc, argv );

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

	testExit();

	return EXIT_SUCCESS;
}
