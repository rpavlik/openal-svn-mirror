#include <AL/al.h>
#include <AL/alext.h>
#include <AL/alc.h>

#include <stdio.h>
#include <stdlib.h>

#define FLOAT_VAL        0.65

int main( void )
{
	ALCdevice *device;
	ALCcontext *context = NULL;
	ALfloat pregain = FLOAT_VAL;
	ALfloat postgain = 0.0;
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

	alListenerf( AL_GAIN, pregain );
	alGetListenerfv( AL_GAIN, &postgain );

	if( postgain != pregain ) {
		fprintf( stderr, "Listener GAIN f***ed up: %f vs %f\n",
			 pregain, postgain );
	}

	alGenSources( 1, &sid );

	alSourcef( sid, AL_GAIN, pregain );
	alGetSourcefv( sid, AL_GAIN, &postgain );

	if( postgain != pregain ) {
		fprintf( stderr, "Source GAIN f***ed up: %f vs %f\n",
			 pregain, postgain );
	}

	fprintf( stderr, "All tests okay\n" );

	fflush( stderr );

	alcMakeContextCurrent( NULL );

	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
