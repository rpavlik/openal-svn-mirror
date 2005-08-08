#include "testlib.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


#define WAVEFILE "boom.wav"

static void iterate(void);
static void init(char *fname);
static void cleanup(void);

static ALuint moving_source = 0;

static void *wave = NULL;
static time_t start;

static void iterate( void ) {
	if(SourceIsPlaying(moving_source) == AL_FALSE) {
		alSourcePlay(moving_source);
		fprintf(stderr, "have to sourceplay\n");
	} else {
		micro_sleep(10000);
	}

	return;
}

static void init( char *fname ) {
	ALfloat zeroes[]   = { 0.0f, 0.0f,  0.0f };
	ALfloat back[]     = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat front[]    = { 0.0f, 0.0f,  1.0f, 0.0f, 1.0f, 0.0f };
	ALfloat position[] = { 0.0f, 0.0f, -4.0f };
	ALuint boom;
	ALsizei size;
	ALsizei freq;
	ALsizei format;
	ALboolean loop;

	start = time(NULL);

	alListenerfv(AL_POSITION, zeroes );
	alListenerfv(AL_VELOCITY, zeroes );
	alListenerfv(AL_ORIENTATION, front );

	alGenBuffers( 1, &boom );

	alutLoadWAVFile( (ALbyte*)fname, &format, &wave, &size, &freq, &loop );
	if(wave == NULL) {
		fprintf(stderr, "Could not include %s\n", fname);
		exit(1);
	}

	alBufferData( boom, format, wave, size, freq );
	free(wave); /* openal makes a local copy of wave data */

	alGenSources( 1, &moving_source);

	alSourcei(  moving_source, AL_BUFFER, boom );
	alSourcei(  moving_source, AL_LOOPING, AL_FALSE);
	alSourcefv( moving_source, AL_POSITION, position );
	alSourcefv( moving_source, AL_VELOCITY, zeroes );
	alSourcefv( moving_source, AL_ORIENTATION, back );
	alSourcef(  moving_source, AL_PITCH, 1.0f );

	return;
}

static void cleanup(void) {
	alutExit();

#ifdef DMALLOC
	dmalloc_verify(0);
	dmalloc_log_unfreed();

#endif
#ifdef JLIB
	jv_check_mem();
#endif
}

int main( int argc, char* argv[] ) {
	time_t shouldend;

	/* Initialize ALUT. */
	alutInit(&argc, argv);

	if(argc == 1) {
		init(WAVEFILE);
	} else {
		init(argv[1]);
	}

	alSourcePlay( moving_source );
	while(1) {
		shouldend = time(NULL);
		if((shouldend - start) > 40) {
			break;
		}

		iterate();
	}

	cleanup();

	return 0;
}
