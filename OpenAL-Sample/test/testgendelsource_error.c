#include <AL/al.h>
#include <AL/alc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main( void )
{
	ALCdevice *device;
	ALCcontext *context;
	ALuint sid;

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

	fprintf( stderr, "alGenSources(0, &sid): should be a NOP\n" );
	/* Should be a NOP */
	alGenSources( 0, &sid );
	fprintf( stderr, "              result : %s\n",
		 alGetString( alGetError(  ) ) );

	fprintf( stderr, "alGenSources(-1, &sid): should error\n" );
	/* Should be an error */
	alGenSources( -1, &sid );
	fprintf( stderr, "              result : %s\n",
		 alGetString( alGetError(  ) ) );

	fprintf( stderr, "alDeleteSources(0, &sid): should be a NOP\n" );
	/* Should be a NOP */
	alDeleteSources( 0, &sid );
	fprintf( stderr, "              result : %s\n",
		 alGetString( alGetError(  ) ) );

	fprintf( stderr, "alDeleteSources(-1, &sid): should error\n" );
	/* Should be an error */
	alDeleteSources( -1, &sid );
	fprintf( stderr, "              result : %s\n",
		 alGetString( alGetError(  ) ) );

	alcDestroyContext( context );

	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
