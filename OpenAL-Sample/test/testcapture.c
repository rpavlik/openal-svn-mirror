#include "testlib.h"

#define NUMCAPTURES   3
#define WAVEFILE      "boom.wav"
#define FREQ          22050
#define SAMPLES       (5 * FREQ)

static ALCcontext *context;

int main( int argc, char *argv[] )
{
	ALCdevice *device;
	ALuint sid = 0;
	ALuint sbid;
	ALuint retval;
	ALvoid *buffer = NULL;
	FILE *fh;

	device = alcOpenDevice( NULL );
	if( device == NULL ) {
		return EXIT_FAILURE;
	}

	/* Initialize openal. */
	context = alcCreateContext( device, NULL );
	if( context == NULL ) {
		alcCloseDevice( device );

		return EXIT_FAILURE;
	}

	alcMakeContextCurrent( context );

	getExtensionEntries(  );

	if( !palCaptureInit( AL_FORMAT_MONO16, FREQ, 1024 ) ) {
		alcCloseDevice( device );

		printf( "Unable to initialize capture\n" );
		return EXIT_FAILURE;
	}

	buffer = malloc( SAMPLES * 2 );

	/*                    test 1              */
	fprintf( stderr, "test1\n" );

	fprintf( stderr, "recording..." );
	palCaptureStart(  );
	retval = 0;
	while( retval < ( SAMPLES * 2 ) ) {
		retval += palCaptureGetData( &( ( char * ) buffer )[retval],
					     ( SAMPLES * 2 ) - retval,
					     AL_FORMAT_MONO16, FREQ );
	}
	palCaptureStop(  );
	fprintf( stderr, "\n" );

	fh = fopen( "outpcm.pcm", "wb" );
	if( fh != NULL ) {
		fwrite( buffer, retval, 1, fh );
		fclose( fh );
	}

	fprintf( stderr, "Sleeping for 5 seconds\n" );
	sleep( 5 );

	/*                    test 2              */
	fprintf( stderr, "test2\n" );

	fprintf( stderr, "recording..." );
	palCaptureStart(  );
	retval = 0;
	while( retval < ( SAMPLES * 2 ) ) {
		retval += palCaptureGetData( &( ( char * ) buffer )[retval],
					     ( SAMPLES * 2 ) - retval,
					     AL_FORMAT_MONO16, FREQ );
	}
	palCaptureStop(  );
	fprintf( stderr, "\n" );

	fprintf( stderr, "playback..." );
	alGenSources( 1, &sid );
	alGenBuffers( 1, &sbid );
	alSourcei( sid, AL_BUFFER, sbid );

	alBufferData( sbid, AL_FORMAT_MONO16, buffer, retval, FREQ );

	alSourcePlay( sid );

	while( sourceIsPlaying( sid ) == AL_TRUE ) {
		sleep( 1 );
	}

	fprintf( stderr, "\n" );

	free( buffer );

	alcDestroyContext( context );
	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
