#include "testlib.h"

#define WAVEFILE   "sample.wav"
#define NUMSOURCES 7

static ALuint movingSource[NUMSOURCES] = { 0 };
static ALCcontext *context;
static time_t start;

static void init( const ALbyte *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat back[] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat front[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat position[] = { 0.0f, 0.0f, -4.0f };
	ALuint boom;
	int i;

	start = time( NULL );

	alListenerfv( AL_POSITION, zeroes );
	alListenerfv( AL_ORIENTATION, front );

	boom = CreateBufferFromFile( fname );

	alGenSources( NUMSOURCES, movingSource );

	for ( i = 0; i < NUMSOURCES; i++ ) {
		alSourcefv( movingSource[i], AL_POSITION, position );
		alSourcefv( movingSource[i], AL_ORIENTATION, back );
		alSourcei( movingSource[i], AL_BUFFER, boom );
		alSourcei( movingSource[i], AL_LOOPING, AL_TRUE );
	}
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

	testInitWithoutContext( &argc, argv );

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

	testExit();

	alcDestroyContext( context );
	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
