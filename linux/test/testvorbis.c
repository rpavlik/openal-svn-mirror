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

#define DATABUFSIZE 4096
#define VORBIS_FILE    "boom.ogg"
#define VORBIS_FUNC    "alutLoadVorbis_LOKI"
#define NUMSOURCES  1

static void init( void );
static void cleanup( void );

static ALuint vorbbuf;		/* our buffer */
static ALuint vorbsource = ( ALuint ) -1;

static time_t start;

static ALCcontext *context;

/* our vorbis extension */
typedef ALboolean ( vorbisLoader ) ( ALuint, ALvoid *, ALint );
vorbisLoader *alutLoadVorbisp = NULL;

static void init( void )
{
	start = time( NULL );

	alGenBuffers( 1, &vorbbuf );
	alGenSources( 1, &vorbsource );

	alSourcei( vorbsource, AL_BUFFER, vorbbuf );
	alSourcei( vorbsource, AL_LOOPING, AL_TRUE );
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
	char *fname;
	int size;

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

	fname = ( argc == 1 ) ? VORBIS_FILE : argv[1];

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

	fread( data, size, 1, fh );

	alutLoadVorbisp =
	    ( vorbisLoader * ) alGetProcAddress( ( ALchar * ) VORBIS_FUNC );
	if( alutLoadVorbisp == NULL ) {
		free( data );

		fprintf( stderr, "Could not GetProc %s\n",
			 ( ALubyte * ) VORBIS_FUNC );
		exit( EXIT_FAILURE );
	}

	if( alutLoadVorbisp( vorbbuf, data, size ) != AL_TRUE ) {
		fprintf( stderr, "alutLoadVorbis failed\n" );
		exit( EXIT_FAILURE );
	}

	free( data );

	alSourcePlay( vorbsource );

	while( sourceIsPlaying( vorbsource ) == AL_TRUE ) {
		sleep( 1 );
	}

	cleanup(  );

	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
