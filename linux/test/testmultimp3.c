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
#define MP3_FILE    "mp3.mp3"
#define MP3_FUNC    "alutLoadMP3_LOKI"
#define MAX_SOURCES 4

static void init( void );
static void cleanup( void );

static ALuint mp3buf[MAX_SOURCES];	/* our buffer */
static ALuint mp3source[MAX_SOURCES];

static time_t start;

static ALCcontext *context;

/* our mp3 extension */
typedef ALboolean ( mp3Loader ) ( ALuint, ALvoid *, ALint );
mp3Loader *alutLoadMP3p = NULL;

static void init( void )
{
	int i;

	start = time( NULL );

	alGenBuffers( MAX_SOURCES, mp3buf );
	alGenSources( MAX_SOURCES, mp3source );

	for ( i = 0; i < MAX_SOURCES; i++ ) {
		alSourcei( mp3source[i], AL_BUFFER, mp3buf[i] );
	}
}

static void cleanup( void )
{

	alcDestroyContext( context );
#ifdef JLIB
	jv_check_mem(  );
#endif
}

int main( int argc, char *argv[] )
{
	ALCdevice *device;
	FILE *fh;
	struct stat sbuf;
	void *data;
	int size;
	char *fname;
	int i = 0;

	device = alcOpenDevice( NULL );
	if( device == NULL ) {
		return EXIT_FAILURE;
	}

	/* Initialize ALUT. */
	context = alcCreateContext( device, NULL );
	if( context == NULL ) {
		alcCloseDevice( device );
		return EXIT_FAILURE;
	}

	alcMakeContextCurrent( context );

	init(  );

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

	for ( i = 0; i < MAX_SOURCES; i++ ) {
		if( alutLoadMP3p( mp3buf[i], data, size ) != AL_TRUE ) {
			fprintf( stderr, "alutLoadMP3p failed\n" );
			exit( EXIT_FAILURE );
		}

		alSourcePlay( mp3source[i] );
		microSleep( 80000 );
	}

	free( data );

	while( sourceIsPlaying( mp3source[0] ) == AL_TRUE ) {
		sleep( 1 );
	}

	cleanup(  );

	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
