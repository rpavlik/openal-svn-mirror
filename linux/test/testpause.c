#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define WAVEFILE "sample.wav"

static void iterate( void );
static void cleanup( void );

static ALuint movingSource = 0;

static void *wave = NULL;
static time_t start;
static ALCcontext *context = NULL;

static void iterate( void )
{
	sleep( 1 );
}

static void init( const ALbyte *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat back[] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat front[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat position[] = { 0.0f, 0.0f, -4.0f };
	ALuint sample;
	ALsizei size;
	ALsizei freq;
	ALsizei format;
	ALboolean loop;

	start = time( NULL );

	alListenerfv( AL_POSITION, zeroes );
	alListenerfv( AL_VELOCITY, zeroes );
	alListenerfv( AL_ORIENTATION, front );

	alGenBuffers( 1, &sample );

	alutLoadWAVFile( fname, &format, &wave, &size, &freq, &loop );
	if( wave == NULL ) {
		fprintf( stderr, "Could not load %s\n",
			 ( const char * ) fname );
		exit( EXIT_FAILURE );
	}

	alBufferData( sample, format, wave, size, freq );
	free( wave );		/* openal makes a local copy of wave data */

	alGenSources( 1, &movingSource );

	alSourcefv( movingSource, AL_POSITION, position );
	alSourcefv( movingSource, AL_VELOCITY, zeroes );
	alSourcefv( movingSource, AL_ORIENTATION, back );
	alSourcei( movingSource, AL_BUFFER, sample );
	alSourcei( movingSource, AL_LOOPING, AL_FALSE );
}

static void cleanup( void )
{
	alcDestroyContext( context );

#ifdef DMALLOC
	dmalloc_verify( 0 );
	dmalloc_log_unfreed(  );

#endif

#ifdef JLIB
	jv_check_mem(  );
#endif
}

int main( int argc, char *argv[] )
{
	ALCdevice *device;
	static ALboolean paused = AL_FALSE;
	time_t shouldend;

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

	init( ( const ALbyte * ) ( ( argc == 1 ) ? WAVEFILE : argv[1] ) );

	alSourcePlay( movingSource );
	while( 1 ) {
		shouldend = time( NULL );

		if( ( shouldend - start ) == 3 ) {
			if( paused == AL_TRUE ) {
				continue;
			}
			paused = AL_TRUE;

			fprintf( stderr, "Pause\n" );
			alcSuspendContext( context );

			continue;
		}

		if( ( shouldend - start ) == 5 ) {
			if( paused == AL_FALSE ) {
				continue;
			}

			paused = AL_FALSE;
			fprintf( stderr, "Unpause\n" );
			alcProcessContext( context );

			continue;
		}

		if( ( shouldend - start ) > 10 ) {
			break;
		}

		iterate(  );
	}

	cleanup(  );

	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
