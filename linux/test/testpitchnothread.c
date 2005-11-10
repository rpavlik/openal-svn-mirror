#include <AL/al.h>
#include <AL/alext.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <math.h>

#define WAVEFILE "boom.wav"

static void iterate( void );

static ALuint movingSource = 0;

static ALCcontext *context;
static void *wave = NULL;
static time_t start;

extern int mixer_iterate( void *dummy );

static void iterate( void )
{
	static float f = 0;
	float g;

	f += .001;

	g = ( sin( f ) + 1.0 ) / 2.0;

/*
	fprintf(stderr, "AL_PITCH = %f\n", g);
	alSourcef(movingSource, AL_PITCH, g);
	*/
	alcProcessContext( context );
}

static void init( const ALbyte *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat front[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	ALuint boom;
	ALsizei size;
	ALsizei freq;
	ALsizei format;
	ALboolean loop;

	start = time( NULL );

	alListenerfv( AL_POSITION, zeroes );
	alListenerfv( AL_VELOCITY, zeroes );
	alListenerfv( AL_ORIENTATION, front );

	alGenBuffers( 1, &boom );

	alutLoadWAVFile( fname, &format, &wave, &size, &freq, &loop );
	if( wave == NULL ) {
		fprintf( stderr, "Could not include %s\n",
			 ( const char * ) fname );
		exit( EXIT_FAILURE );
	}

	alBufferData( boom, format, wave, size, freq );
	free( wave );		/* openal makes a local copy of wave data */

	alGenSources( 1, &movingSource );

	alSourcef( movingSource, AL_GAIN, 0.25 );
	alSourcei( movingSource, AL_BUFFER, boom );
	alSourcei( movingSource, AL_LOOPING, AL_TRUE );
	alSourcef( movingSource, AL_PITCH, 1.00 );
}

int main( int argc, char *argv[] )
{
	ALCdevice *device;
	int attributeList[] = { ALC_SYNC, AL_TRUE, 0 };
	time_t shouldend;

	device = alcOpenDevice( NULL );
	if( device == NULL ) {
		return EXIT_FAILURE;
	}

	/* Initialize ALUT. */
	context = alcCreateContext( device, attributeList );
	if( context == NULL ) {
		alcCloseDevice( device );

		return EXIT_FAILURE;
	}

	alcMakeContextCurrent( context );

	init( ( const ALbyte * ) ( ( argc == 1 ) ? WAVEFILE : argv[1] ) );

	alSourcef( movingSource, AL_PITCH, 1.19 );
	alSourcePlay( movingSource );

	shouldend = time( NULL );

	while( ( shouldend - start ) < 4 ) {
		shouldend = time( NULL );

		iterate(  );
	}

	alcDestroyContext( context );
	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
