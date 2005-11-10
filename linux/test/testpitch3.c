#include "testlib.h"

#include <AL/al.h>
#include <AL/alc.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <math.h>

#ifndef M_PI
# define M_PI		3.14159265358979323846	/* pi */
#endif

#define WAVEFILE "sample.wav"

#define MIDDLEC 523.25
#define DEFFREQ 22050

/*            a c d e f g a b C D E F G A B            */
char *musicstr = "C 3 C 3 C 3 g 2 a 2 a 2 g 2 E 3 E 3 D 3 D 3 C 3";
char *dmusicstr = "E 2 D 2 C 2 D 2 E 2 E 2 E 2 D 2 D 2 D 2 E 2 G 2 G 3";

static void init( const char *fname );

static ALuint movingSource = 0;

static time_t start;
static void *cc;		/* al context */

static void init( const char *fname )
{
	ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
	ALfloat front[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	ALuint boom;
	ALshort *data;
	int i;
	const int canon_max = ( ( 1 << ( 16 - 1 ) ) - 1 );

	start = time( NULL );

	alListenerfv( AL_POSITION, zeroes );
	alListenerfv( AL_VELOCITY, zeroes );
	alListenerfv( AL_ORIENTATION, front );

	alGenBuffers( 1, &boom );

	data = malloc( DEFFREQ * sizeof *data );
	if( data == NULL ) {
		exit( EXIT_FAILURE );
	}

	/* populate data with a middle C */
	for ( i = 0; i < DEFFREQ; i++ ) {
		data[i] = canon_max * sin( MIDDLEC * 2.0 * M_PI * i / DEFFREQ );
	}

	alBufferData( boom, AL_FORMAT_MONO16, data, DEFFREQ, DEFFREQ );
	free( data );		/* openal makes a local copy of wave data */

	alGenSources( 1, &movingSource );

	alSourcef( movingSource, AL_GAIN, 0.20 );
	alSourcei( movingSource, AL_BUFFER, boom );
	alSourcei( movingSource, AL_LOOPING, AL_FALSE );
}

int main( int argc, char *argv[] )
{
	ALCdevice *device;
	int attributeList[] = { ALC_FREQUENCY, DEFFREQ,
		ALC_INVALID
	};
	char *musicitr = musicstr;
	ALfloat pitch = 1.0;
	int beats;
	char note;

	device = alcOpenDevice( NULL );
	if( device == NULL ) {
		return EXIT_FAILURE;
	}

	cc = alcCreateContext( device, attributeList );
	if( cc == NULL ) {
		alcCloseDevice( device );

		return EXIT_FAILURE;
	}

	alcMakeContextCurrent( cc );

	if( argc == 1 ) {
		init( WAVEFILE );
	} else {
		init( argv[1] );
	}

	while( *musicitr ) {
		alSourceStop( movingSource );

		while( *musicitr == ' ' ) {
			musicitr++;
		}

		switch ( *musicitr ) {
		case 'c':
			pitch = 0.500010;
			break;
		case 'd':
			pitch = 0.561223;
			break;
		case 'e':
			pitch = 0.629967;
			break;
		case 'f':
			pitch = 0.667425;
			break;
		case 'g':
			pitch = 0.749164;
			break;
		case 'a':
			pitch = 0.840898;
			break;
		case 'b':
			pitch = 0.943870;
			break;
		case 'C':
			pitch = 1.0;
			break;
		case 'D':
			pitch = 1.122465;
			break;
		case 'E':
			pitch = 1.259933;
			break;
		case 'F':
			pitch = 1.339704;
			break;
		case 'G':
			pitch = 1.498309;
			break;
		case 'A':
			pitch = 1.681796;
			break;
		case 'B':
			pitch = 1.897754;
			break;
		default:
			fprintf( stderr, "unknown note %d\n", *musicitr );
			break;
		}

		note = *musicitr;
		musicitr++;	/* skip note */

		while( *musicitr == ' ' ) {
			musicitr++;
		}

		beats = ( int ) *musicitr - '0';

		musicitr++;

		fprintf( stderr, "note %c beats %d\n", note, beats );

		alSourcef( movingSource, AL_PITCH, pitch );
		alSourcePlay( movingSource );
		microSleep( beats / 4.0 * 1000000 );
	}

	alcDestroyContext( cc );
	alcCloseDevice( device );

	return EXIT_SUCCESS;
}
