#include <stdlib.h>
#include <stdio.h>
#include <AL/al.h>
#include <AL/alc.h>

int main( int argc, char *argv[] )
{
	ALCcontext *context;
	ALCdevice *device;

	int attributeList[] = { ALC_FREQUENCY, 44100,
		0
	};

	device = alcOpenDevice( NULL );
	if( device == NULL ) {
		return EXIT_FAILURE;
	}

	context = alcCreateContext( device, attributeList );
	if( context == NULL ) {
		alcCloseDevice( device );
		return EXIT_FAILURE;
	}
	alcMakeContextCurrent( context );

	printf( "AL_VENDOR: %s\n", alGetString( AL_VENDOR ) );
	printf( "AL_VERSION: %s\n", alGetString( AL_VERSION ) );
	printf( "AL_RENDERER: %s\n", alGetString( AL_RENDERER ) );
	printf( "AL_EXTENSIONS: %s\n", alGetString( AL_EXTENSIONS ) );

	if( alIsExtensionPresent
	    ( ( const ALCchar * ) "AL_LOKI_attenuation_scale" ) ) {
		printf( "Found AL_LOKI_attenuation_scale\n" );
	}

	alcMakeContextCurrent( NULL );
	alcDestroyContext( context );
	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
