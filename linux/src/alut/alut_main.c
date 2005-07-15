/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * alut_main.c
 *
 * Top level functions for alut stuff.
 */
#include "al_siteconfig.h"
#include "al_main.h"

#include <AL/alc.h>
#include <AL/alut.h>

/*void alutInit(UNUSED(int *argc), UNUSED(char *argv[])) {*/
void alutInit(int *argc, char *argv[]) {
	ALCdevice *device;
	ALCcontext *context;

	/* Open device */
	device = alcOpenDevice(NULL);
	if (device)
	{
		/* Create context */
		context = alcCreateContext(device,NULL);
		if (context)
		{
			/* Set active context */
			alcMakeContextCurrent(context);
		}
	}
}

void alutExit(void) {
	ALCcontext *context = alcGetCurrentContext();
	if( context != NULL ) {
		ALCdevice *device = alcGetContextsDevice( context );
		alcMakeContextCurrent( NULL );
		alcDestroyContext( context );
		if( device != NULL ) {
			alcCloseDevice( device );
		}
	}
}
