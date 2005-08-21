#include "testlib.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <errno.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define DATABUFSIZE 4098

#define ADPCM_FILE "adpcm.adp"

static void init( void );

static ALuint movingSource[1];

static time_t start;

static ALCcontext *context;
ALuint stereo;			/* our buffer */

static void init( void )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat back[] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat front[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat position[] = { -10.0f, 0.0f, 4.0f };

	start = time( NULL );

	alListenerfv( AL_POSITION, zeroes );
	alListenerfv( AL_ORIENTATION, front );

	alGenBuffers( 1, &stereo );
	alGenSources( 1, movingSource );

	alSourcefv( movingSource[0], AL_POSITION, position );
	alSourcefv( movingSource[0], AL_ORIENTATION, back );
	alSourcei( movingSource[0], AL_BUFFER, stereo );
	alSourcei( movingSource[0], AL_LOOPING, AL_FALSE );
}

int main( int argc, char *argv[] )
{
	ALCdevice *device;
	int attributeList[] = { ALC_FREQUENCY, 22050,
		ALC_INVALID, 0
	};
	void *data = NULL;
	struct stat sbuf;
	FILE *fh;
	int speed;
	int size;
	char *fname;

	device = alcOpenDevice( NULL );
	if( device == NULL ) {
		return EXIT_FAILURE;
	}

	/* Initialize ALUT. */
	context = alcCreateContext( device, attributeList );
	if( context == NULL ) {
		return EXIT_FAILURE;
	}

	alcMakeContextCurrent( context );

	getExtensionEntries(  );

	if( argc == 1 ) {
		fname = ADPCM_FILE;
	} else {
		fname = argv[1];
	}

	init(  );

	palBombOnError(  );

	if( stat( fname, &sbuf ) == -1 ) {
		perror( "stat" );
		return EXIT_FAILURE;
	}

	size = sbuf.st_size;

	data = malloc( size );
	if( data == NULL ) {
		perror( "malloc" );
		return EXIT_FAILURE;
	}

	fh = fopen( fname, "rb" );
	if( fh == NULL ) {
		fprintf( stderr, "Could not open %s\n", fname );

		exit( EXIT_FAILURE );
	}

	fread( data, 1, size, fh );

	speed = *( int * ) data;

	if( palutLoadRAW_ADPCMData( stereo,
				    ( char * ) data + 4, size - 4,
				    speed, AL_FORMAT_MONO16 ) == AL_FALSE ) {
		fprintf( stderr, "Could not alutLoadADPCMData_LOKI\n" );
		exit( EXIT_FAILURE );
	}
	free( data );

	alSourcePlay( movingSource[0] );

	while( sourceIsPlaying( movingSource[0] ) == AL_TRUE ) {
		sleep( 1 );
	}

	alcDestroyContext( context );
	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
