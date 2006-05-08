#include "testlib.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define DATABUFSIZE 4098
#define MP3_FILE    "boom.mp3"
#define WAVE_FILE   "sample.wav"
#define MP3_FUNC    "alutLoadMP3_LOKI"
#define NUMSOURCES  1

static ALuint mp3buf;		/* our buffer */
static ALuint mp3source = ( ALuint ) -1;

static time_t start;

/* our mp3 extension */
typedef ALboolean ( mp3Loader ) ( ALuint, ALvoid *, ALint );
mp3Loader *alutLoadMP3p = NULL;

static void initmp3( void )
{
	start = time( NULL );

	alGenBuffers( 1, &mp3buf );
	alGenSources( 1, &mp3source );

	alSourcei( mp3source, AL_BUFFER, mp3buf );
}

static void initwav( const ALbyte *fname )
{
	mp3buf = CreateBufferFromFile( fname );
}

int main( int argc, char *argv[] )
{
	FILE *fh;
	struct stat sbuf;
	void *data;
	int size;
	char *fname;

	testInit( &argc, argv );

	initmp3(  );

	if( argc == 1 ) {
		fname = MP3_FILE;
	} else {
		fname = argv[1];
	}

	if( stat( fname, &sbuf ) == -1 ) {
		perror( fname );
		return errno;
	}

	size = sbuf.st_size;
	data = malloc( size );
	if( data == NULL ) {
		exit( EXIT_FAILURE );
	}

	fh = fopen( fname, "rb" );
	if( fh == NULL ) {
		fprintf( stderr, "Could not open %s\n", fname );

		free( data );

		exit( EXIT_FAILURE );
	}

	fread( data, 1, size, fh );

	alutLoadMP3p =
	    ( mp3Loader * ) alGetProcAddress( ( ALchar * ) MP3_FUNC );
	if( alutLoadMP3p == NULL ) {
		free( data );

		fprintf( stderr, "Could not GetProc %s\n",
			 ( ALubyte * ) MP3_FUNC );
		exit( EXIT_FAILURE );
	}

	if( alutLoadMP3p( mp3buf, data, size ) != AL_TRUE ) {
		fprintf( stderr, "alutLoadMP3p failed\n" );
		exit( EXIT_FAILURE );
	}

	free( data );

	alSourcePlay( mp3source );

	while( sourceIsPlaying( mp3source ) == AL_TRUE ) {
		sleep( 1 );
	}

	fprintf( stderr, "Okay, now for the normal wav file\n" );

	initwav( ( const ALbyte * ) WAVE_FILE );

	alSourcePlay( mp3source );

	while( sourceIsPlaying( mp3source ) == AL_TRUE ) {
		sleep( 1 );
	}

	sleep( 1 );

	testExit();

	return EXIT_SUCCESS;
}
