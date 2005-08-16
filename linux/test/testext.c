#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <stdio.h>
#include <stdlib.h>

static ALCcontext *context;

#define BADPROC           "lokitest"
#define GOODPROC          "alLokiTest"

typedef void blah_type( void * );

int main( int argc, char *argv[] )
{
	ALCdevice *device;
	blah_type *blah;

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

	blah = ( blah_type * ) alGetProcAddress( ( ALchar * ) BADPROC );
	if( blah != NULL ) {
		fprintf( stderr, "weird, it seems %s is defined\n", BADPROC );
	}

	blah = ( blah_type * ) alGetProcAddress( ( ALchar * ) GOODPROC );
	if( blah == NULL ) {
		fprintf( stderr, "weird, it seems %s is not defined\n",
			 GOODPROC );
	} else {
		fprintf( stderr, "good, %s is %p\n", GOODPROC,
			 ( void * ) blah );
	}

	blah( NULL );

	alcDestroyContext( context );

	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
