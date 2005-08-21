#include "testlib.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define WAVEFILE   "adpcm.adp"
#define NUMSOURCES 8000

static void init( const char *fname );

static ALuint movingSource[NUMSOURCES];
static ALuint buffer;

static void init( const char *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat front[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	FILE *fh;
	ALvoid *data;
	struct stat buf;
	int size;
	int speed;

	alListenerfv( AL_POSITION, zeroes );
	alListenerfv( AL_VELOCITY, zeroes );
	alListenerfv( AL_ORIENTATION, front );

	alGenBuffers( 1, &buffer );

	if( stat( fname, &buf ) < 0 ) {
		fprintf( stderr, "Could not stat %s\n", fname );
		exit( EXIT_FAILURE );
	}

	fh = fopen( fname, "rb" );
	if( fh == NULL ) {
		fprintf( stderr, "Could not fopen %s\n", fname );
		exit( EXIT_FAILURE );
	}

	size = buf.st_size;
	data = malloc( size );

	fread( data, 1, size, fh );

	speed = *( int * ) data;

	if( palutLoadRAW_ADPCMData( buffer,
				    ( char * ) data + 4, size - 4,
				    speed, AL_FORMAT_MONO16 ) == AL_FALSE ) {
		fprintf( stderr, "Could not alutLoadADPCMData_LOKI\n" );
		exit( EXIT_FAILURE );
	}

	free( data );
}

int main( int argc, char *argv[] )
{
	int i = 0;
	int j = 0;

	/* Initialize ALUT. */
	alutInit( &argc, argv );

	getExtensionEntries(  );

	if( argc == 1 ) {
		init( WAVEFILE );
	} else {
		init( argv[1] );
	}

	for ( i = 0; i < 8; i++ ) {
		fprintf( stderr, "i = %d\n", i );

		for ( j = 0; j < NUMSOURCES; j++ ) {
			alGenSources( 1, &movingSource[j] );
			alIsSource( movingSource[j] );
			alDeleteSources( 1, &movingSource[j] );
		}

	}

	alutExit(  );

	return EXIT_SUCCESS;
}
