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
ALboolean alutInit(ALCchar *szDeviceName, ALCdevice **ppDevice, ALCcontext **ppContext) {
	ALCdevice *device = alcOpenDevice( szDeviceName );
	ALCcontext *context = NULL;
	if( device != NULL ) {
		context = alcCreateContext( device, NULL );
		if( context != NULL ) {
			alcMakeContextCurrent( context );
		}
	}
	if (ppDevice)
		*ppDevice = device;

	if (ppContext)
		*ppContext = context;

	return AL_TRUE;
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
