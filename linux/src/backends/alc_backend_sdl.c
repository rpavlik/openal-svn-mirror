/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * sdl.c
 *
 * SDL backend.
 */
#include "al_siteconfig.h"

#include <AL/al.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include <SDL_audio.h>

#include "al_debug.h"
#include "al_main.h"
#include "alc/alc_context.h"
#include "backends/alc_backend.h"

#define DEF_SPEED	_ALC_CANON_SPEED
#define DEF_SIZE	_ALC_DEF_BUFSIZ
#define DEF_SAMPLES     (DEF_SIZE / 2)
#define DEF_CHANNELS	2
#define SDL_DEF_FMT	AUDIO_S16

static struct {
	SDL_AudioSpec spec;
	ALboolean firstTime;
} sdl_info;

static void *ringbuffer;
static Uint32 ringbuffersize;
static Uint32 readOffset;
static Uint32 writeOffset;

static void
dummy(UNUSED(void *unused), Uint8 *stream, int len)
{
	memcpy_offset(stream, ringbuffer, readOffset, (size_t)len);
	readOffset += len;

	if(readOffset >= ringbuffersize) {
		readOffset  = 0;
		writeOffset = 0;
	}
}

static void *
grab_write_sdl(void)
{
        sdl_info.spec.freq     = DEF_SPEED;
        sdl_info.spec.channels = DEF_CHANNELS;
        sdl_info.spec.samples  = DEF_SAMPLES;
        sdl_info.spec.size     = DEF_SIZE;
        sdl_info.spec.format   = SDL_DEF_FMT;
        sdl_info.spec.callback = dummy;
	sdl_info.firstTime     = AL_TRUE;

        if(SDL_OpenAudio(&sdl_info.spec, NULL) < 0) {
		/* maybe we need SDL_Init? */
		SDL_Init(SDL_INIT_AUDIO);

		if(SDL_OpenAudio(&sdl_info.spec, NULL) < 0) {
			_alDebug(ALD_CONTEXT, __FILE__, __LINE__,
				"No SDL: %s", SDL_GetError());
			return NULL;
		}
        }

	if(ringbuffer != NULL) {
		free(ringbuffer);
	}

	ringbuffersize = 2 * sdl_info.spec.size;
	ringbuffer     = malloc(ringbuffersize);
	readOffset      = 0;
	writeOffset     = 0;

	_alDebug(ALD_CONTEXT, __FILE__, __LINE__, "SDL grab audio ok");

        return &sdl_info.spec;
}

static void *
grab_read_sdl(void)
{
	return NULL;
}

void *
alcBackendOpenSDL_( ALC_OpenMode mode )
{
	return mode == ALC_OPEN_INPUT_ ? grab_read_sdl() : grab_write_sdl();
}

void
sdl_blitbuffer(UNUSED(void *handle), void *data, int bytes)
{
	if (sdl_info.firstTime == AL_TRUE) {
		sdl_info.firstTime = AL_FALSE;
		offset_memcpy(ringbuffer, writeOffset, data, (size_t)bytes);
		writeOffset = bytes;
		/* start SDL callback mojo */
		SDL_PauseAudio(0);
	} else {
		SDL_LockAudio();
		while(writeOffset >= ringbuffersize) {
			SDL_UnlockAudio();
			SDL_Delay(1);
			SDL_LockAudio();
		}

		offset_memcpy(ringbuffer, writeOffset, data, (size_t)bytes);
		writeOffset += bytes;

		SDL_UnlockAudio();
	}
}

void
release_sdl(UNUSED(void *handle))
{
	SDL_CloseAudio();
}

static ALboolean
set_write_sdl(UNUSED(void *handle), ALuint *bufsiz, ALenum *fmt, ALuint *speed)
{
	ALuint bytesPerSample   = _alGetBitsFromFormat(*fmt) >> 3;
	ALuint channels = _alGetChannelsFromFormat(*fmt);

        memset(&sdl_info, '\0', sizeof (sdl_info));
        sdl_info.spec.freq     = *speed;
        sdl_info.spec.channels = channels;
        sdl_info.spec.samples  = *bufsiz / bytesPerSample;
        sdl_info.spec.format   = _al_AL2ACFMT(*fmt);
        sdl_info.spec.callback = dummy;
	sdl_info.firstTime     = AL_TRUE;

        SDL_CloseAudio();

        if(SDL_OpenAudio(&sdl_info.spec, NULL) < 0) {
		fprintf(stderr,
			"No SDL: %s\n", SDL_GetError());

                return AL_FALSE;
        }

	*bufsiz = sdl_info.spec.size;

	if(ringbuffer != NULL) {
		free(ringbuffer);
	}

	ringbuffersize = 2 * sdl_info.spec.size;
	ringbuffer     = malloc(ringbuffersize);
	readOffset      = 0;
	writeOffset     = 0;

	memset(ringbuffer, 0, ringbuffersize);

	/* FIXME: should remove extraneous *channels and rely only on format */
	*fmt      = _al_AC2ALFMT(sdl_info.spec.format, sdl_info.spec.channels);
	*speed    = sdl_info.spec.freq;

	_alDebug(ALD_CONTEXT, __FILE__, __LINE__, "set_write_sdl ok");

        return AL_TRUE;
}

static ALboolean
set_read_sdl(UNUSED(void *handle), UNUSED(ALuint *bufsiz), UNUSED(ALenum *fmt),
	     UNUSED(ALuint *speed))
{
	return AL_FALSE;
}

ALboolean
alcBackendSetAttributesSDL_(ALC_OpenMode mode, void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed)
{
	return mode == ALC_OPEN_INPUT_ ?
		set_read_sdl(handle, bufsiz, fmt, speed) :
		set_write_sdl(handle, bufsiz, fmt, speed);
}

void
pause_sdl( UNUSED(void *handle) )
{
}

void
resume_sdl( UNUSED(void *handle) )
{
}

ALsizei
capture_sdl( UNUSED(void *handle), UNUSED(void *capture_buffer), UNUSED(int bufsiz) )
{
	return 0;
}

ALfloat
get_sdlchannel( UNUSED(void *handle), UNUSED(ALuint channel) )
{
	return 0.0;
}

int
set_sdlchannel( UNUSED(void *handle), UNUSED(ALuint channel), UNUSED(ALfloat volume) )
{
	return 0;
}
