/*
 * This is a test program for the ALC_EXT_capture extension. This
 *  test code is public domain and comes with NO WARRANTY.
 *
 * Written by Ryan C. Gordon <icculus@icculus.org>
 * Modified by Sven Panne <sven.panne@aedion.de>
 */

#include "testlib.h"

#define CAPTURE_FORMAT AL_FORMAT_MONO16
#define FMTSIZE 8
#define CAPTURE_FREQUENCY 44100
#define NUM_SAMPLES (CAPTURE_FREQUENCY * 5)
static ALbyte captureData[NUM_SAMPLES * FMTSIZE];

static void recordSomething( ALCdevice *inputDevice )
{
	ALint samples;
	printf( "recording...\n" );
	alcCaptureStart( inputDevice );
	do {
		microSleep( 10000 );
		alcGetIntegerv( inputDevice, ALC_CAPTURE_SAMPLES, 1, &samples );
	} while( samples < NUM_SAMPLES );
	alcCaptureSamples( inputDevice, captureData, NUM_SAMPLES );
	alcCaptureStop( inputDevice );
}

int main( int argc, char **argv )
{
	ALuint source;
	ALuint buffer;
	ALint state;
	ALCdevice *inputDevice;

	alutInit( &argc, argv );

	if( !alcIsExtensionPresent( NULL, "ALC_EXT_capture" ) ) {
		fprintf( stderr, "No ALC_EXT_capture support.\n" );
		return EXIT_FAILURE;
	}

	printf( "ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER: %s\n",
		alcGetString( NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER ) );

	if( alcIsExtensionPresent( NULL, "ALC_ENUMERATION_EXT" ) == ALC_FALSE ) {
		printf( "No ALC_ENUMERATION_EXT support.\n" );
	} else {
		const char *devList = ( const char * ) alcGetString( NULL,
								     ALC_CAPTURE_DEVICE_SPECIFIER );
		printf( "ALC_ENUMERATION_EXT:\n" );
		while( *devList ) {
			printf( "  - %s\n", devList );
			devList += strlen( devList ) + 1;
		}
	}

	inputDevice =
	    alcCaptureOpenDevice( NULL, CAPTURE_FREQUENCY, CAPTURE_FORMAT,
				  NUM_SAMPLES );
	if( inputDevice == NULL ) {
		fprintf( stderr, "Couldn't open capture device.\n" );
		alutExit(  );
		return EXIT_FAILURE;
	}
	recordSomething( inputDevice );
	alcCaptureCloseDevice( inputDevice );

	alGenSources( 1, &source );
	alGenBuffers( 1, &buffer );

	printf( "Playing...\n" );

	alBufferData( buffer, CAPTURE_FORMAT, captureData,
		      sizeof( captureData ), CAPTURE_FREQUENCY );
	alSourcei( source, AL_BUFFER, buffer );
	alSourcePlay( source );

	do {
		microSleep( 100000 );
		alGetSourcei( source, AL_SOURCE_STATE, &state );
	} while( state == AL_PLAYING );

	printf( "Cleaning up...\n" );

	alDeleteSources( 1, &source );
	alDeleteBuffers( 1, &buffer );

	alutExit(  );
	return EXIT_SUCCESS;
}
