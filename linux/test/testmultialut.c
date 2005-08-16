#include "testlib.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define WAVEFILE1 "boom.wav"
#define WAVEFILE2 "sample.wav"

#define	NUMBUFFERS 2
#define	NUMSOURCES 2

static void iterate( void );
static void init( void );
static void cleanup( void );

static ALuint movingSource[NUMSOURCES];

static time_t start;
static void *data = ( void * ) 0xDEADBEEF;
static void *data2 = ( void * ) 0xDEADBEEF;

static ALCcontext *context;

static void iterate( void )
{
	static ALfloat position[] = { 10.0f, 0.0f, 4.0f };
	static ALfloat movefactor = 2.0;
	static time_t then = 0;
	time_t now;

	now = time( NULL );

	/* Switch between left and right every two seconds. */
	if( now - then > 2 ) {
		then = now;

		movefactor *= -1.0;
	}

	position[0] += movefactor;
	alSourcefv( movingSource[1], AL_POSITION, position );

	position[0] *= -1.0;
	alSourcefv( movingSource[0], AL_POSITION, position );
	position[0] *= -1.0;

	microSleep( 500000 );
}

static void init( void )
{
	FILE *fh;
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat back[] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat front[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat position[] = { 0.0f, 0.0f, -4.0f };
	ALuint boomers[NUMBUFFERS];
	int filelen;

	data = malloc( 5 * ( 512 * 3 ) * 1024 );
	data2 = malloc( 5 * ( 512 * 3 ) * 1024 );

	start = time( NULL );

	alListenerfv( AL_POSITION, zeroes );
	alListenerfv( AL_VELOCITY, zeroes );
	alListenerfv( AL_ORIENTATION, front );

	alGenBuffers( NUMBUFFERS, boomers );

	fh = fopen( WAVEFILE1, "rb" );
	if( fh == NULL ) {
		fprintf( stderr, "Couldn't open %s\n", WAVEFILE1 );
		exit( EXIT_FAILURE );

	}
	filelen = fread( data, 1, 1024 * 1024, fh );
	fclose( fh );

	alGetError(  );

	alBufferData( boomers[0], AL_FORMAT_WAVE_EXT, data, filelen, 0 );
	if( alGetError(  ) != AL_NO_ERROR ) {
		fprintf( stderr, "Could not BufferData\n" );
		exit( EXIT_FAILURE );
	}

	fh = fopen( WAVEFILE2, "rb" );
	if( fh == NULL ) {
		fprintf( stderr, "Couldn't open %s\n", WAVEFILE2 );
		exit( EXIT_FAILURE );
	}

	filelen = fread( data2, 1, 1024 * 1024, fh );
	fclose( fh );

	alBufferData( boomers[1], AL_FORMAT_WAVE_EXT, data, filelen, 0 );
	alGenSources( 2, movingSource );

	alSourcefv( movingSource[0], AL_POSITION, position );
	alSourcefv( movingSource[0], AL_VELOCITY, zeroes );
	alSourcefv( movingSource[0], AL_ORIENTATION, back );
	alSourcei( movingSource[0], AL_BUFFER, boomers[1] );
	alSourcei( movingSource[0], AL_LOOPING, AL_TRUE );

	alSourcefv( movingSource[1], AL_POSITION, position );
	alSourcefv( movingSource[1], AL_VELOCITY, zeroes );
	alSourcefv( movingSource[1], AL_ORIENTATION, back );
	alSourcei( movingSource[1], AL_BUFFER, boomers[0] );
	alSourcei( movingSource[1], AL_LOOPING, AL_TRUE );
}

static void cleanup( void )
{
	free( data );
	free( data2 );

	alcDestroyContext( context );
#ifdef JLIB
	jv_check_mem(  );
#endif
}

int main( int argc, char *argv[] )
{
	ALCdevice *device;
	time_t shouldend;
	int attributeList[] =
	    { ALC_FREQUENCY, 22050, ALC_SOURCES_LOKI, 3000, 0 };

	device = alcOpenDevice( NULL );
	if( device == NULL ) {
		return EXIT_FAILURE;
	}

	context = alcCreateContext( device, attributeList );
	if( context == NULL ) {
		alcCloseDevice( device );
		return EXIT_FAILURE;
	}

	alcMakeContextCurrent( context );

	init(  );

	alSourcePlay( movingSource[0] );
	sleep( 1 );
	alSourcePlay( movingSource[1] );

	while( ( sourceIsPlaying( movingSource[0] ) == AL_TRUE ) ||
	       ( sourceIsPlaying( movingSource[1] ) == AL_TRUE ) ) {
		iterate(  );

		shouldend = time( NULL );

		if( ( shouldend - start ) > 10 ) {
			alSourceStop( movingSource[0] );
			alSourceStop( movingSource[1] );
		}
	}

	cleanup(  );

	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
