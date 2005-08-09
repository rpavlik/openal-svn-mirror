#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <stdio.h>

static ALCcontext *context_id;

#define BADPROC           "lokitest"
#define GOODPROC          "alLokiTest"

typedef void blah_type( void * );

int main( int argc, char *argv[] )
{
	ALCdevice *dev;
	blah_type *blah;

	dev = alcOpenDevice( NULL );
	if( dev == NULL ) {
		return 1;
	}

	context_id = alcCreateContext( dev, NULL );
	if( context_id == NULL ) {
		alcCloseDevice( dev );

		return 1;
	}

	alcMakeContextCurrent( context_id );

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

	alcDestroyContext( context_id );

	alcCloseDevice( dev );

	return 0;
}
