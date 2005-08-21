#include "testlib.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define WAVEFILE "sample.wav"

static ALuint reverb_sid = 0;

static ALCcontext *context;
static void *wave = NULL;

static void init( const ALbyte *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat back[] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat front[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat sourcepos[] = { 2.0f, 0.0f, 4.0f };
	ALuint locutus;
	ALsizei size;
	ALsizei freq;
	ALsizei format;
	ALboolean loop;

	alListenerfv( AL_POSITION, zeroes );
	alListenerfv( AL_VELOCITY, zeroes );
	alListenerfv( AL_ORIENTATION, front );

	alGenBuffers( 1, &locutus );

	alutLoadWAVFile( fname, &format, &wave, &size, &freq, &loop );

	if( wave == NULL ) {
		fprintf( stderr, "Could not include %s\n",
			 ( const char * ) fname );
		exit( EXIT_FAILURE );
	}

	alBufferData( locutus, format, wave, size, freq );
	free( wave );		/* openal makes a local copy of wave data */

	alGenSources( 1, &reverb_sid );

	alSourcefv( reverb_sid, AL_POSITION, sourcepos );
	alSourcefv( reverb_sid, AL_VELOCITY, zeroes );
	alSourcefv( reverb_sid, AL_ORIENTATION, back );
	alSourcei( reverb_sid, AL_BUFFER, locutus );

	palReverbScale( reverb_sid, 0.35 );
	palReverbDelay( reverb_sid, 1 );
}

int main( int argc, char *argv[] )
{
	ALCdevice *device;
	int i;

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

	getExtensionEntries(  );

	init( ( const ALbyte * ) ( ( argc == 1 ) ? WAVEFILE : argv[1] ) );

	alSourcePlay( reverb_sid );

	for ( i = 0; i < 10; i++ ) {
		microSleep( 1000000 );
	}

	alcDestroyContext( context );
	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
