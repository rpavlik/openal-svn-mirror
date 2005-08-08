#include "testlib.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define WAVEFILE      "boom.wav"
#define NUMSOURCES    1

static void iterate( void );
static void init( char *fname );
static void cleanup(void);

static ALuint multis;

static ALCcontext *context_id;
static void *wave = NULL;

static void iterate( void ) {
	fprintf(stderr, "NOW\n");
	alSourcePlay( multis);
	fprintf(stderr, "OVER\n");

	micro_sleep(1000000);
}

static void init( char *fname ) {
	ALfloat zeroes[]   = { 0.0f, 0.0f,  0.0f };
	ALfloat back[]     = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat position[] = { 2.0f, 0.0f, -4.0f };
	ALuint boom;
	ALsizei size;
	ALsizei freq;
	ALsizei format;
	ALboolean loop;

	alListenerfv(AL_POSITION, zeroes );
	alListenerfv(AL_VELOCITY, zeroes );
	/* alListenerfv(AL_ORIENTATION, front ); */

	alGenBuffers( 1, &boom );

	alutLoadWAVFile( (ALbyte*)fname, &format, &wave, &size, &freq, &loop );
	if(wave == NULL) {
		fprintf(stderr, "Could not include %s\n", fname);
		exit(1);
	}


	alBufferData( boom, format, wave, size, freq );
	free(wave); /* openal makes a local copy of wave data */

	alGenSources( NUMSOURCES ,&multis);

	alSourcefv( multis, AL_POSITION, position );
	alSourcefv( multis, AL_VELOCITY, zeroes );
	alSourcefv( multis, AL_ORIENTATION, back );
	alSourcef(  multis, AL_GAIN_LINEAR_LOKI, 1.0f );

	alSourceQueueBuffers( multis, 1, &boom );
	alSourceQueueBuffers( multis, 1, &boom );
	alSourcei(   multis, AL_BUFFER, boom );

	return;
}

void cleanup(void) {
	alcDestroyContext(context_id);
#ifdef JLIB
	jv_check_mem();
#endif

	return;
}

int main( int argc, char* argv[] ) {
	ALCdevice *dev;

	dev = alcOpenDevice( NULL );
	if( dev == NULL ) {
		return 1;
	}

	/* Initialize ALUT. */
	context_id = alcCreateContext( dev, NULL );
	if(context_id == NULL) {
		alcCloseDevice( dev );

		return 1;
	}

	alcMakeContextCurrent( context_id );

	fixup_function_pointers();

	if(argc == 1) {
		init(WAVEFILE);
	} else {
		init(argv[1]);
	}

	iterate();

	while(SourceIsPlaying(multis) == AL_TRUE) {
		micro_sleep(1000000);
	}

	cleanup();

	alcCloseDevice( dev );

	return 0;
}
