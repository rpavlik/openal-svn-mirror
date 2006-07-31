#include "testlib.h"

#define WAVEFILE "boom.wav"

static void iterate( void );

static ALuint rightSid;

static void iterate( void )
{
#if 0
	/* works ok! */
	ALfloat orientation[] = { 0.0f, 0.0f, -1.0f,
		0.0f, 1.0f, 0.0f
	};
#else
	/* does not work! */
	ALfloat orientation[] = { 0.0f, 0.0f, -1.0f,	/* at */
		0.0f, 1.0f, 0.0f
	};			/* up */
#endif

	static ALint iter = 0;

	if( iter++ % 2 ) {
#if 0
		orientation[2] *= -1;
#else
		orientation[4] *= -1;
#endif
	}

	fprintf( stderr, "orientation: \n\tAT(%f %f %f)\n\tUP(%f %f %f)\n",
		 orientation[0], orientation[1], orientation[2],
		 orientation[3], orientation[4], orientation[5] );

	alListenerfv( AL_ORIENTATION, orientation );

	microSleep( 900000 );
}

static void init( const ALbyte *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat front[] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat position[] = { 40.0f, 0.0f, 0.0f };
	ALuint boom;

	alListenerfv( AL_POSITION, zeroes );
	alListenerfv( AL_VELOCITY, zeroes );
	alListenerfv( AL_ORIENTATION, front );

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

	testInit (&argc, argv);

	init( ( const ALbyte * ) ( ( argc == 1 ) ? WAVEFILE : argv[1] ) );

	alSourcePlay( rightSid );

	while( shouldend - start < 20 ) {
		shouldend = time( NULL );

		iterate(  );
	}

	testExit ();
	
	return EXIT_SUCCESS;
}
