#include <stdlib.h>
#include <stdio.h>
#include <AL/al.h>
#include <AL/alc.h>

int main( int argc, char *argv[] )
{
	ALCcontext *context_id;
	ALCdevice *dev;

	int attrlist[] = { ALC_FREQUENCY, 44100,
		0
	};

	dev = alcOpenDevice( NULL );
	if( dev == NULL ) {
		return EXIT_FAILURE;
	}

	context_id = alcCreateContext( dev, attrlist );
	if( context_id == NULL ) {
		alcCloseDevice( dev );
		return EXIT_FAILURE;
	}
	alcMakeContextCurrent( context_id );

	printf( "AL_VENDOR: %s\n", alGetString( AL_VENDOR ) );
	printf( "AL_VERSION: %s\n", alGetString( AL_VERSION ) );
	printf( "AL_RENDERER: %s\n", alGetString( AL_RENDERER ) );
	printf( "AL_EXTENSIONS: %s\n", alGetString( AL_EXTENSIONS ) );

	if( alIsExtensionPresent
	    ( ( const ALCchar * ) "AL_LOKI_attenuation_scale" ) ) {
		printf( "Found AL_LOKI_attenuation_scale\n" );
	}

	alcMakeContextCurrent( NULL );
	alcDestroyContext( context_id );
	alcCloseDevice( dev );

	return EXIT_SUCCESS;
}
