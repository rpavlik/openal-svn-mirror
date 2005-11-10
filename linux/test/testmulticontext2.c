#include "testlib.h"

#include <AL/al.h>
#include <AL/alc.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define DATABUFFERSIZE (10 * (512 * 3) * 1024)

static void iterate( void );
static void init( const char *fname );
static void cleanup( void );

static ALuint movingSource = 0;

static time_t start;
static void *data = ( void * ) 0xDEADBEEF;

static ALCcontext *context;

static void iterate( void )
{
	static ALfloat position[] = { 10.0f, 0.0f, 4.0f };
	static ALfloat movefactor = 4.5;
	static time_t then = 0;
	time_t now;

	now = time( NULL );

	/* Switch between left and right stereo sample every two seconds. */
	if( now - then > 2 ) {
		then = now;

		movefactor *= -1.0;
	}

	position[0] += movefactor;
	alSourcefv( movingSource, AL_POSITION, position );

	microSleep( 500000 );
}

static void init( const char *fname )
{
	FILE *fh;
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat front[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat position[] = { 0.0f, 0.0f, -4.0f };
	ALuint stereo;
	int filelen;

	data = malloc( DATABUFFERSIZE );

	start = time( NULL );

	alListenerfv( AL_POSITION, zeroes );
	/* alListenerfv(AL_VELOCITY, zeroes ); */
	alListenerfv( AL_ORIENTATION, front );

	alGenBuffers( 1, &stereo );

	fh = fopen( fname, "rb" );
	if( fh == NULL ) {
		fprintf( stderr, "Couldn't open fname\n" );
		exit( EXIT_FAILURE );
	}

	filelen = fread( data, 1, DATABUFFERSIZE, fh );
	fclose( fh );

	alGetError(  );
	alBufferData( stereo, AL_FORMAT_WAVE_EXT, data, filelen, 0 );
	if( alGetError(  ) != AL_NO_ERROR ) {
		fprintf( stderr, "Could not BufferData\n" );
		exit( EXIT_FAILURE );
	}

	free( data );

	alGenSources( 1, &movingSource );

	alSourcefv( movingSource, AL_POSITION, position );
	/* alSourcefv( movingSource, AL_VELOCITY, zeroes ); */
	alSourcei( movingSource, AL_BUFFER, stereo );
	alSourcei( movingSource, AL_LOOPING, AL_TRUE );
}

int main( int argc, char *argv[] )
{
	ALCdevice *device;
	int attributeList[] = { ALC_FREQUENCY, 44100,
		0
	};
	time_t shouldend;
	void *dummy;

	device = alcOpenDevice( NULL );
	if( device == NULL ) {
		return EXIT_FAILURE;
	}

	/* Initialize ALUT. */
	context = alcCreateContext( device, attributeList );
	if( context == NULL ) {
		return EXIT_FAILURE;
	}

	dummy = alcCreateContext( device, attributeList );

	alcMakeContextCurrent( dummy );

	alcDestroyContext( context );

	getExtensionEntries(  );

	palBombOnError(  );

	if( argc == 1 ) {
		init( "sample.wav" );
	} else {
		init( argv[1] );
	}

	alSourcePlay( movingSource );
	while( sourceIsPlaying( movingSource ) == AL_TRUE ) {
		iterate(  );

		shouldend = time( NULL );
		if( ( shouldend - start ) > 10 ) {
			alSourceStop( movingSource );
		}
	}

	alcDestroyContext( context );

	return EXIT_SUCCESS;
}
