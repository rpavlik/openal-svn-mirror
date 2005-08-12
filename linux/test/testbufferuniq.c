#include <AL/al.h>
#include <AL/alc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "testlib.h"

#define NUM_BUFFERS 4000

static int bid_compare( const void *bid1p, const void *bid2p )
{
	const ALuint *bid1 = bid1p;
	const ALuint *bid2 = bid2p;

	return bid1 - bid2;
}

static ALboolean find_duplicate( ALuint *bids, int nbids )
{
	int i;
	ALuint last = bids[0];
	ALboolean retval = AL_TRUE;

	for ( i = 1; i < nbids; i++ ) {
		if( bids[i] == last ) {
			fprintf( stderr, "Duplicate buffer ID %d\n", last );
			retval = AL_FALSE;
		}

		last = bids[i];
	}

	return retval;
}

int main( void )
{
	ALCdevice *device;
	ALCcontext *context;
	ALuint bids_first[NUM_BUFFERS];
	ALuint bids_second[NUM_BUFFERS];
	ALuint bids_total[2 * NUM_BUFFERS];
	int i;

	device = alcOpenDevice( NULL );
	if( device == NULL ) {
		return EXIT_FAILURE;
	}

	context = alcCreateContext( device, NULL );
	if( context == NULL ) {
		return EXIT_FAILURE;
	}
	alcMakeContextCurrent( context );

	getExtensionEntries(  );
	palBombOnError(  );

	/* generate buffer IDs */
	alGenBuffers( NUM_BUFFERS, bids_first );

	/* copy them */
	memcpy( bids_second, bids_first, NUM_BUFFERS * sizeof *bids_first );

	/* delete original ones */
	alDeleteBuffers( NUM_BUFFERS, bids_first );

	/* generate a new set of buffer IDs */
	alGenBuffers( NUM_BUFFERS, bids_first );

	/* copy both buffer IDs into bids_total */
	for ( i = 0; i < NUM_BUFFERS; i++ ) {
		bids_total[i] = bids_first[i];
		bids_total[i + NUM_BUFFERS] = bids_second[i];
	}

	/* sort bids_total */
	qsort( bids_total, 2 * NUM_BUFFERS, sizeof *bids_total, bid_compare );

	if( find_duplicate( bids_total, 2 * NUM_BUFFERS ) == AL_TRUE ) {
		fprintf( stderr, "No Duplicate buffer IDs.\n" );
	}

	alcMakeContextCurrent( NULL );
	alcDestroyContext( context );
	alcCloseDevice( device );
	return EXIT_SUCCESS;
}
