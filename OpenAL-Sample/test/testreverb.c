#include "testlib.h"

#define WAVEFILE "sample.wav"

static ALuint reverb_sid = 0;

static void init( const ALbyte *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat back[] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat front[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat sourcepos[] = { 2.0f, 0.0f, 4.0f };
	ALuint locutus;

	alListenerfv( AL_POSITION, zeroes );
	alListenerfv( AL_VELOCITY, zeroes );
	alListenerfv( AL_ORIENTATION, front );

	locutus = CreateBufferFromFile( fname );

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
	int i;

	testInit( &argc, argv );

	getExtensionEntries(  );

	init( ( const ALbyte * ) ( ( argc == 1 ) ? WAVEFILE : argv[1] ) );

	alSourcePlay( reverb_sid );

	for ( i = 0; i < 10; i++ ) {
		microSleep( 1000000 );
	}

	testExit();

	return EXIT_SUCCESS;
}
