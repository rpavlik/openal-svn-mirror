#include "testlib.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define WAVEFILE "boom.wav"

static ALuint movingSource = 0;

static void *wave = NULL;
static time_t start;

static void init( const ALbyte *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat position[] = { 0.0f, 0.0f, 0.0f };
	ALuint boom;
	ALsizei size;
	ALsizei freq;
	ALsizei format;
#ifndef __APPLE__
	ALboolean loop;
#endif
	start = time( NULL );

	alListenerfv( AL_POSITION, zeroes );
	/*
	   alListenerfv(AL_VELOCITY, zeroes );
	   alListenerfv(AL_ORIENTATION, front );
	 */

	alGenBuffers( 1, &boom );
#ifndef __APPLE__
	alutLoadWAVFile( fname, &format, &wave, &size, &freq, &loop );
#else
	alutLoadWAVFile( fname, &format, &wave, &size, &freq );
#endif
	if( wave == NULL ) {
		fprintf( stderr, "Could not open %s\n",
			 ( const char * ) fname );
		exit( EXIT_FAILURE );
	}

	fprintf( stderr, "(format 0x%x size %d freq %d\n", format, size, freq );

	switch ( format ) {
	case AL_FORMAT_MONO8:
	case AL_FORMAT_MONO16:
		fprintf( stderr, "Not using MULTICHANNEL, format = 0x%x\n",
			 format );
		break;
	default:
		fprintf( stderr, "Using MULTICHANNEL\n" );
		/* talBufferi(boom, AL_CHANNELS, 2); */
		break;
	}

	palBufferWriteData( boom, format, wave, size, freq, format );
	free( wave );		/* openal makes a local copy of wave data */

	alGenSources( 1, &movingSource );

	alSourcefv( movingSource, AL_POSITION, position );
	alSourcef( movingSource, AL_PITCH, 1.0 );
	/*
	   alSourcefv( movingSource, AL_VELOCITY, zeroes );
	   alSourcefv( movingSource, AL_ORIENTATION, back );
	 */
	alSourcei( movingSource, AL_BUFFER, boom );
	alSourcei( movingSource, AL_LOOPING, AL_FALSE );
}

int main( int argc, char *argv[] )
{
	ALCdevice *device;
	void *ci;
	int attributeList[] = { ALC_FREQUENCY, 44100, 0 };

	device = alcOpenDevice( NULL );
	if( device == NULL ) {
		return EXIT_FAILURE;
	}

	/* Initialize ALUT. */
	ci = alcCreateContext( device, attributeList );
	if( ci == NULL ) {
		alcCloseDevice( device );

		exit( EXIT_FAILURE );
	}

	alcMakeContextCurrent( ci );

	getExtensionEntries(  );

	init( ( const ALbyte * ) ( ( argc == 1 ) ? WAVEFILE : argv[1] ) );

	alSourcePlay( movingSource );
	while( sourceIsPlaying( movingSource ) ) {
		microSleep( 100000 );
	}

	alcDestroyContext( ci );

	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
