/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * bsd_dsp.h
 *
 * Native BSD backend.
 */
#ifndef BSD_DSP_H_
#define BSD_DSP_H_

#include <AL/alc.h>

	/* is this right? */
#define _AL_DEF_BUFSIZ _ALC_DEF_BUFSIZ

void *grab_native(void);

/* native bsd (/dev/dsp) stuff */
void *grab_read_native(void);
void *grab_write_native(void);

ALboolean set_write_native(void *handle,
		     unsigned int *bufsiz,
		     ALenum *fmt,
		     unsigned int *speed);
ALboolean set_read_native(void *handle,
		     unsigned int *bufsiz,
		     ALenum *fmt,
		     unsigned int *speed);

int   grab_mixerfd(void);
void  native_blitbuffer(void *handle, void *data, int bytes);
void  release_native(void *handle);

float get_nativechannel(void *handle, ALuint channel);
int   set_nativechannel(void *handle, ALuint channel, float volume);

void pause_nativedevice(void *handle);
void resume_nativedevice(void *handle);

ALsizei capture_nativedevice(void *handle, void *capture_buffer, int bufsiz);

#endif
