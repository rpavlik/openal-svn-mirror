#include <AL/al.h>
#include <AL/alc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main( void )
{
	ALCdevice *device;
	ALCcontext *context;
	ALuint bid;

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

	fprintf( stderr, "alGenBuffers(0, &bid): should be a NOP\n" );
	/* Should be a NOP */
	alGenBuffers( 0, &bid );
	fprintf( stderr, "              result : %s\n",
		 alGetString( alGetError(  ) ) );

	fprintf( stderr, "alGenBuffers(-1, &bid): should error\n" );
	/* Should be an error */
	alGenBuffers( -1, &bid );
	fprintf( stderr, "              result : %s\n",
		 alGetString( alGetError(  ) ) );

	fprintf( stderr, "alDeleteBuffers(0, &bid): should be a NOP\n" );
	/* Should be a NOP */
	alDeleteBuffers( 0, &bid );
	fprintf( stderr, "              result : %s\n",
		 alGetString( alGetError(  ) ) );

	fprintf( stderr, "alDeleteBuffers(-1, &bid): should error\n" );
	/* Should be an error */
	alDeleteBuffers( -1, &bid );
	fprintf( stderr, "              result : %s\n",
		 alGetString( alGetError(  ) ) );

	alcDestroyContext( context );

	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
