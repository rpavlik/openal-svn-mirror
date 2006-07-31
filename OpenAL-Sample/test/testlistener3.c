#include "testlib.h"

#define WAVEFILE "sample.wav"

#define TORAD(d) ((d / 180.0) * M_PI)

static void iterate( void );

static ALuint rightSid;

static void iterate( void )
{
	ALfloat orientation[] = { 0.0f, 0.0f, -1.0f,
		0.0f, 1.0f, 0.0f
	};
	static ALint angle = 0;

	/*
	 * rotate at vector about up vector by angle degrees.
	 */
	_RotatePointAboutAxis( TORAD( angle ), orientation, &orientation[3] );

	angle += 15;		/* increment fifeteen degrees degree */

	fprintf( stderr, "orientation: \n\tAT(%f %f %f)\n\tUP(%f %f %f)\n",
		 orientation[0], orientation[1], orientation[2],
		 orientation[3], orientation[4], orientation[5] );

	alListenerfv( AL_ORIENTATION, orientation );

	microSleep( 1500000 );
}

static void init( const ALbyte *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat position[] = { 50.0f, 0.0f, 0.0f };
	ALuint boom;

	alListenerfv( AL_POSITION, zeroes );

	boom = CreateBufferFromFile( fname );
	
	alGenSources( 1, &rightSid );

	alSourcefv( rightSid, AL_POSITION, position );
	alSourcei( rightSid, AL_BUFFER, boom );
	alSourcei( rightSid, AL_LOOPING, AL_TRUE );
}

int main( int argc, char *argv[] )
{
	time_t start;
	time_t shouldend;

	start = time( NULL );
	shouldend = time( NULL );

	testInit( &argc, argv );

	init( ( const ALbyte * ) ( ( argc == 1 ) ? WAVEFILE : argv[1] ) );

	alSourcePlay( rightSid );

	while( shouldend - start < 20 ) {
		shouldend = time( NULL );

		iterate(  );
	}

	testExit();

	return EXIT_SUCCESS;
}
