/*
 * This test creates and destroys 75 contexts for the default device, one each
 * 0.8 seconds.
 */

#include "testlib.h"

#define NUM_CONTEXTS 75

static void iterate( ALCdevice *device )
{
	ALCcontext *context = alcCreateContext( device, NULL );

	/* If we don't pause, we'll in all likelyhood hose the soundcard. */
	microSleep( 800000 );

	alcDestroyContext( context );
}

int main( int argc, char *argv[] )
{
	int i;
	ALCdevice *device = alcOpenDevice( NULL );
	if( device == NULL ) {
		return EXIT_FAILURE;
	}

	for ( i = 0; i < NUM_CONTEXTS; i++ ) {
		printf( "iteration %d\n", i );
		iterate( device );
	}

	alcCloseDevice( device );
	return EXIT_SUCCESS;
}
