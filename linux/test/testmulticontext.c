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
#define NUMCONTEXTS    2

static void init( const char *fname );

static void *data;

static ALCcontext *contexts[NUMCONTEXTS];
static ALuint movingSource[NUMCONTEXTS];
static ALuint bid;

#if 0
static void iterate( void )
{
	static ALfloat position[] = { 10.0f, 0.0f, -4.0f };
	static ALfloat movefactor = 4.5;
	static time_t then = 0;
	time_t now;
	int i;

	now = time( NULL );

	/* Switch between left and right bid sample every two seconds. */
	if( now - then > 2 ) {
		then = now;

		movefactor *= -1.0;
	}

	position[0] += movefactor;

	for ( i = 0; i < NUMCONTEXTS; i++ ) {
		alcMakeContextCurrent( contexts[i] );
		alSourcefv( movingSource[i], AL_POSITION, position );
	}

	microSleep( 500000 );
}
#endif

static void init( const char *fname )
{
	FILE *fh;
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat front[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat position[] = { 0.0f, 0.0f, -4.0f };
	int filelen;
	int i;

	data = malloc( DATABUFFERSIZE );

	alListenerfv( AL_POSITION, zeroes );
	/* alListenerfv(AL_VELOCITY, zeroes ); */
	alListenerfv( AL_ORIENTATION, front );

	alGenBuffers( 1, &bid );

	fh = fopen( fname, "rb" );
	if( fh == NULL ) {
		fprintf( stderr, "Couldn't open fname\n" );
		exit( EXIT_FAILURE );
	}

	filelen = fread( data, 1, DATABUFFERSIZE, fh );
	fclose( fh );

	alGetError(  );

	alBufferData( bid, AL_FORMAT_WAVE_EXT, data, filelen, 0 );
	if( alGetError(  ) != AL_NO_ERROR ) {
		fprintf( stderr, "Could not BufferData\n" );
		exit( EXIT_FAILURE );
	}

	free( data );

	for ( i = 0; i < NUMCONTEXTS; i++ ) {
		alcMakeContextCurrent( contexts[i] );

		alGenSources( 1, &movingSource[i] );

		alSourcefv( movingSource[i], AL_POSITION, position );
		/* alSourcefv( movingSource[i], AL_VELOCITY, zeroes ); */
		alSourcei( movingSource[i], AL_BUFFER, bid );
		alSourcei( movingSource[i], AL_LOOPING, AL_TRUE );
	}
}

int main( int argc, char *argv[] )
{
	ALCdevice *device;
	int attributeList[] = { ALC_FREQUENCY, 44100,
		ALC_INVALID
	};
	time_t start;
	int i;

	device = alcOpenDevice( NULL );
	if( device == NULL ) {
		return EXIT_FAILURE;
	}

	/* Initialize ALUT. */
	for ( i = 0; i < NUMCONTEXTS; i++ ) {
		contexts[i] = alcCreateContext( device, attributeList );
		if( contexts[i] == NULL ) {
			return EXIT_FAILURE;
		}
	}

	getExtensionEntries(  );

	palBombOnError(  );

	if( argc == 1 ) {
		init( "sample.wav" );
	} else {
		init( argv[1] );
	}

	start = time( NULL );

	for ( i = 0; i < NUMCONTEXTS; i++ ) {
		alcMakeContextCurrent( contexts[i] );
		alSourcePlay( movingSource[i] );
		sleep( 1 );
	}

#if 0
	while( done == AL_FALSE ) {
		iterate(  );

		shouldend = time( NULL );
		if( ( shouldend - start ) > 10 ) {
			for ( i = 0; i < NUMCONTEXTS; i++ ) {
				alcMakeContextCurrent( contexts[i] );
				alSourceStop( movingSource[i] );
			}
		}

		done = AL_TRUE;
		for ( i = 0; i < NUMCONTEXTS; i++ ) {
			alcMakeContextCurrent( contexts[i] );
			done = done && !sourceIsPlaying( movingSource[i] );
		}
	}
#else
	for ( i = 0; i < 10; i++ ) {
		fprintf( stderr, "i = %d\n", i );
		sleep( 1 );
	}
#endif

	for ( i = 0; i < NUMCONTEXTS; i++ ) {
		alcDestroyContext( contexts[i] );
	}

	return EXIT_SUCCESS;
}
