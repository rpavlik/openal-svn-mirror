/*
 * This test plays a WAVE file for 30 seconds, switching between left and right
 * every 2 seconds. The LOKI_WAVE_format extension is used.
 */

#include <AL/al.h>
#include <AL/alc.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "testlib.h"

#define DATA_BUFFER_SIZE (10 * (512 * 3) * 1024)

static void iterate( ALuint movingSource )
{
	static ALfloat position[] = { 10, 0, 4 };
	static ALfloat moveFactor = 4.5;
	static time_t then = 0;
	time_t now = time( NULL );

	/* Switch between left and right stereo sample every two seconds. */
	if( now - then > 2 ) {
		then = now;
		moveFactor *= -1;
	}

	position[0] += moveFactor;
	alSourcefv( movingSource, AL_POSITION, position );

	microSleep( 500000 );
}

static ALuint init( const char *fname )
{
	FILE *fileHandle;
	ALfloat front[] = { 0, 0, 1, 0, 1, 0 };
	int fileLength;
	ALuint buffer;
	ALuint movingSource;
	void *data = malloc( DATA_BUFFER_SIZE );
	if( data == NULL ) {
		fprintf( stderr, "Couldn't open allocate buffer memory\n" );
		exit( EXIT_FAILURE );
	}

	fileHandle = fopen( fname, "rb" );
	if( fileHandle == NULL ) {
		fprintf( stderr, "Couldn't open fname\n" );
		exit( EXIT_FAILURE );
	}
	fileLength = fread( data, 1, DATA_BUFFER_SIZE, fileHandle );
	fclose( fileHandle );

	alListenerfv( AL_ORIENTATION, front );

	alGenBuffers( 1, &buffer );
	/* sure hope it's a wave file */
	alBufferData( buffer, AL_FORMAT_WAVE_EXT, data, fileLength, 0 );
	free( data );

	alGenSources( 1, &movingSource );
	alSourcei( movingSource, AL_BUFFER, buffer );
	alSourcei( movingSource, AL_LOOPING, AL_TRUE );
	return movingSource;
}

int main( int argc, char *argv[] )
{
	int attrlist[] = {
		ALC_FREQUENCY, 22050,
		0
	};
	time_t start = time( NULL );
	time_t shouldEnd;
	ALCcontext *context;
	ALuint movingSource;

	ALCdevice *device = alcOpenDevice( NULL );
	if( device == NULL ) {
		fprintf( stderr, "Could not open device\n" );
		return EXIT_FAILURE;
	}

	/* Initialize context. */
	context = alcCreateContext( device, attrlist );
	if( context == NULL ) {
		fprintf( stderr, "Could not open context: %s\n",
			 alGetString( alcGetError( device ) ) );
		return EXIT_FAILURE;
	}
	alcMakeContextCurrent( context );

	getExtensionEntries(  );
	palBombOnError(  );

	movingSource = init( ( argc == 1 ) ? "sample.wav" : argv[1] );

	alSourcePlay( movingSource );
	while( sourceIsPlaying( movingSource ) == AL_TRUE ) {
		iterate( movingSource );

		shouldEnd = time( NULL );
		if( ( shouldEnd - start ) > 30 ) {
			alSourceStop( movingSource );
		}
	}

	alcMakeContextCurrent( NULL );
	alcDestroyContext( context );
	alcCloseDevice( device );
	return EXIT_SUCCESS;
}
