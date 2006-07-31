#include "testlib.h"

#define DATABUFFERSIZE (10 * (512 * 3) * 1024)
/*#define SYNCHRONIZED*/

static void iterate( void );
static void init( const char *fname );
static void cleanup( void );

static ALuint movingSource = 0;

static time_t start;
static void *data = ( void * ) 0xDEADBEEF;

static ALCcontext *context;

static void iterate( void )
{
	alSourcePlay( movingSource );
#ifdef SYNCHRONIZED
	alcUpdateContext( context );
#endif
	alSourceStop( movingSource );
}

static void init( const char *fname )
{
	FILE *fh;
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat front[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat position[] = { 0.0f, 0.0f, -4.0f };
	ALuint stereo;
	int filelen;

	data = malloc( DATABUFFERSIZE );

	start = time( NULL );

	alListenerfv( AL_POSITION, zeroes );
	/* alListenerfv(AL_VELOCITY, zeroes ); */
	alListenerfv( AL_ORIENTATION, front );

	alGenBuffers( 1, &stereo );

	fh = fopen( fname, "rb" );
	if( fh == NULL ) {
		fprintf( stderr, "Couldn't open fname\n" );
		exit( EXIT_FAILURE );
	}

	filelen = fread( data, 1, DATABUFFERSIZE, fh );
	fclose( fh );

	alGetError(  );
	alBufferData( stereo, AL_FORMAT_WAVE_EXT, data, filelen, 0 );
	if( alGetError(  ) != AL_NO_ERROR ) {
		fprintf( stderr, "Could not BufferData\n" );
		exit( EXIT_FAILURE );
	}

	free( data );

	alGenSources( 1, &movingSource );

	alSourcefv( movingSource, AL_POSITION, position );
	/* alSourcefv( movingSource, AL_VELOCITY, zeroes ); */
	alSourcei( movingSource, AL_BUFFER, stereo );
	alSourcei( movingSource, AL_LOOPING, AL_TRUE );
}

int main( int argc, char *argv[] )
{
	ALCdevice *device;
	int attributeList[] = { ALC_FREQUENCY, 44100,
#ifdef SYNCHRONIZED
		ALC_SYNC, AL_TRUE,
#endif
		0
	};
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

	getExtensionEntries(  );

	palBombOnError(  );

	if( argc == 1 ) {
		init( "sample.wav" );
	} else {
		init( argv[1] );
	}

	while( 1 ) {
		iterate(  );

		shouldend = time( NULL );
		if( ( shouldend - start ) > 10 ) {
			break;
		}
	}

	alcDestroyContext( context );
	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
