/*
 * This test creates 2 contexts and dumps all its attributes.
 */

#include "testlib.h"

static void dumpContext( ALCdevice *device, const char *name )
{
	ALCint numAttributes;
	ALCint *attributeList;
	int i;

	printf( "*** %s context *** \n", name );

	alcGetIntegerv( device, ALC_ATTRIBUTES_SIZE, 1, &numAttributes );
	printf( "attributes size %d\n", numAttributes );

	attributeList = malloc( numAttributes * sizeof( ALCint ) );
	if( attributeList == NULL ) {
		fprintf( stderr, "Couldn't open allocate attribute buffer\n" );
		exit( EXIT_FAILURE );
	}

	alcGetIntegerv( device, ALC_ALL_ATTRIBUTES, numAttributes,
			attributeList );

	for ( i = 0; i < numAttributes - 1; i += 2 ) {
		printf( "key 0x%x : value %d\n", attributeList[i],
			attributeList[i + 1] );
	}

	if( attributeList[numAttributes - 1] != 0 ) {
		fprintf( stderr, "attribute list must be 0-terminated\n" );
		exit( EXIT_FAILURE );
	}
}

static void
testContext( ALCdevice *device, const char *name, const ALCint *attributeList )
{
	ALCcontext *context = alcCreateContext( device, attributeList );
	if( context == NULL ) {
		fprintf( stderr, "Couldn't create context\n" );
		exit( EXIT_FAILURE );
	}
	alcMakeContextCurrent( context );
	dumpContext( device, name );
	alcMakeContextCurrent( NULL );
	alcDestroyContext( context );
}

int main( int argc, char *argv[] )
{
	ALCint attributeList[] = {
		ALC_FREQUENCY, 44100,
		ALC_SYNC, AL_TRUE,
		0
	};
	ALCdevice *device = alcOpenDevice( ( const ALCchar * )
					   "'((sampling-rate 44100))" );
	if( device == NULL ) {
		fprintf( stderr, "Couldn't open device\n" );
		return EXIT_FAILURE;
	}

	testContext( device, "default", NULL );
	printf( "\n" );
	testContext( device, "custom", attributeList );

	return EXIT_SUCCESS;
}
