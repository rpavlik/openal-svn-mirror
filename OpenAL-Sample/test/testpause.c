#include "testlib.h"

#define WAVEFILE "sample.wav"

static void iterate( void );

static ALuint movingSource = 0;

static time_t start;
static ALCcontext *context = NULL;

static void iterate( void )
{
	sleep( 1 );
}

static void init( const ALbyte *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat back[] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat front[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat position[] = { 0.0f, 0.0f, -4.0f };
	ALuint sample;

	start = time( NULL );

	alListenerfv( AL_POSITION, zeroes );
	alListenerfv( AL_VELOCITY, zeroes );
	alListenerfv( AL_ORIENTATION, front );

	sample = CreateBufferFromFile( fname );

	alGenSources( 1, &movingSource );

	alSourcefv( movingSource, AL_POSITION, position );
	alSourcefv( movingSource, AL_VELOCITY, zeroes );
	alSourcefv( movingSource, AL_ORIENTATION, back );
	alSourcei( movingSource, AL_BUFFER, sample );
	alSourcei( movingSource, AL_LOOPING, AL_FALSE );
}

int main( int argc, char *argv[] )
{
	ALCdevice *device;
	static ALboolean paused = AL_FALSE;
	time_t shouldend;

	device = alcOpenDevice( NULL );
	if( device == NULL ) {
		return EXIT_FAILURE;
	}

	context = alcCreateContext( device, NULL );
	if( context == NULL ) {
		alcCloseDevice( device );

		return EXIT_FAILURE;
	}

	alcMakeContextCurrent( context );

	testInitWithoutContext( &argc, argv );

	init( ( const ALbyte * ) ( ( argc == 1 ) ? WAVEFILE : argv[1] ) );

	alSourcePlay( movingSource );
	while( 1 ) {
		shouldend = time( NULL );

		if( ( shouldend - start ) == 3 ) {
			if( paused == AL_TRUE ) {
				continue;
			}
			paused = AL_TRUE;

			fprintf( stderr, "Pause\n" );
			alcSuspendContext( context );

			continue;
		}

		if( ( shouldend - start ) == 5 ) {
			if( paused == AL_FALSE ) {
				continue;
			}

			paused = AL_FALSE;
			fprintf( stderr, "Unpause\n" );
			alcProcessContext( context );

			continue;
		}

		if( ( shouldend - start ) > 10 ) {
			break;
		}

		iterate(  );
	}

	testExit();

	alcDestroyContext( context );
	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
