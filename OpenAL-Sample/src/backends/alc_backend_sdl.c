/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * sdl.c
 *
 * SDL backend.
 */
#include "al_siteconfig.h"
#include "backends/alc_backend.h"
#include <stdlib.h>

#ifndef USE_BACKEND_SDL

void
alcBackendOpenSDL_ (UNUSED(ALC_OpenMode mode), UNUSED(ALC_BackendOps **ops),
		    ALC_BackendPrivateData **privateData)
{
	*privateData = NULL;
}

#else

#include <AL/al.h>
#include <string.h>
#include <SDL.h>
#include <SDL_audio.h>

#include "al_debug.h"
#include "al_main.h"
#include "alc/alc_context.h"

#ifdef OPENAL_DLOPEN_SDL
#include <dlfcn.h>
#endif


#define DEF_SPEED	_ALC_CANON_SPEED
#define DEF_SIZE	ALC_DEFAULT_DEVICE_BUFFER_SIZE_IN_BYTES
#define DEF_SAMPLES     (DEF_SIZE / 2)
#define DEF_CHANNELS	2
#define SDL_DEF_FMT	AUDIO_S16

static struct {
	SDL_AudioSpec spec;
	ALboolean firstTime;
	ALC_OpenMode mode;
} sdl_info;

static void *ringbuffer;
static Uint32 ringbuffersize;
static Uint32 readOffset;
static Uint32 writeOffset;

static int openal_load_sdl_library(void);

/*
 * sdl library functions.
 */
static void SDLCALL (*pSDL_Delay)(Uint32 ms);
static void SDLCALL (*pSDL_PauseAudio)(int pause_on);
static void SDLCALL (*pSDL_CloseAudio)(void);
static int SDLCALL (*pSDL_OpenAudio)(SDL_AudioSpec *desired, SDL_AudioSpec *obtained);
static int SDLCALL (*pSDL_Init)(Uint32 flags);
static char* SDLCALL (*pSDL_GetError)(void);
static void SDLCALL (*pSDL_LockAudio)(void);
static void SDLCALL (*pSDL_UnlockAudio)(void);

/*
 * sdl library handle.
 */
static void * sdl_lib_handle = NULL;

static int openal_load_sdl_library(void)
{
#ifdef OPENAL_DLOPEN_SDL
        char * error = NULL;
#endif
    
	if (sdl_lib_handle != NULL)
		return 1;  /* already loaded. */

	#ifdef OPENAL_DLOPEN_SDL
		#define OPENAL_LOAD_SDL_SYMBOL(x) p##x = dlsym(sdl_lib_handle, #x); \
                                                   error = dlerror(); \
                                                   if (p##x == NULL) { \
                                                           fprintf(stderr,"Could not resolve SDL symbol %s: %s\n", #x, ((error!=NULL)?(error):("(null)"))); \
                                                           dlclose(sdl_lib_handle); sdl_lib_handle = NULL; \
                                                           return 0; }
                dlerror(); /* clear error state */
		sdl_lib_handle = dlopen("libSDL.so", RTLD_LAZY | RTLD_GLOBAL);
                error = dlerror();
		if (sdl_lib_handle == NULL) {
                        fprintf(stderr,"Could not open SDL library: %s\n",((error!=NULL)?(error):("(null)")));
			return 0;
                }
	#else
		#define OPENAL_LOAD_SDL_SYMBOL(x) p##x = x;
		sdl_lib_handle = (void *) 0xF00DF00D;
	#endif

        OPENAL_LOAD_SDL_SYMBOL(SDL_Delay);
        OPENAL_LOAD_SDL_SYMBOL(SDL_PauseAudio);
        OPENAL_LOAD_SDL_SYMBOL(SDL_CloseAudio);
        OPENAL_LOAD_SDL_SYMBOL(SDL_OpenAudio);
        OPENAL_LOAD_SDL_SYMBOL(SDL_Init);
        OPENAL_LOAD_SDL_SYMBOL(SDL_GetError);
        OPENAL_LOAD_SDL_SYMBOL(SDL_LockAudio);
        OPENAL_LOAD_SDL_SYMBOL(SDL_UnlockAudio);

	return 1;
}


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
	if (!openal_load_sdl_library())
		return NULL;

        sdl_info.spec.freq     = DEF_SPEED;
        sdl_info.spec.channels = DEF_CHANNELS;
        sdl_info.spec.samples  = DEF_SAMPLES;
        sdl_info.spec.size     = DEF_SIZE;
        sdl_info.spec.format   = SDL_DEF_FMT;
        sdl_info.spec.callback = dummy;
	sdl_info.firstTime     = AL_TRUE;
	sdl_info.mode          = ALC_OPEN_OUTPUT_;

        if(pSDL_OpenAudio(&sdl_info.spec, NULL) < 0) {
		/* maybe we need SDL_Init? */
		pSDL_Init(SDL_INIT_AUDIO);

		if(pSDL_OpenAudio(&sdl_info.spec, NULL) < 0) {
			_alDebug(ALD_CONTEXT, __FILE__, __LINE__,
				"No SDL: %s", pSDL_GetError());
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

static void
sdl_blitbuffer(UNUSED(void *handle), const void *data, int bytesToWrite)
{
	if (sdl_info.firstTime == AL_TRUE) {
		sdl_info.firstTime = AL_FALSE;
		offset_memcpy(ringbuffer, writeOffset, data, (size_t)bytesToWrite);
		writeOffset = bytesToWrite;
		/* start SDL callback mojo */
		pSDL_PauseAudio(0);
	} else {
		pSDL_LockAudio();
		while(writeOffset >= ringbuffersize) {
			pSDL_UnlockAudio();
			pSDL_Delay(1);
			pSDL_LockAudio();
		}

		offset_memcpy(ringbuffer, writeOffset, data, (size_t)bytesToWrite);
		writeOffset += bytesToWrite;

		pSDL_UnlockAudio();
	}
}

static void
release_sdl(UNUSED(void *handle))
{
	pSDL_CloseAudio();
}

static ALboolean
set_write_sdl(UNUSED(void *handle), ALuint *deviceBufferSizeInBytes, ALenum *fmt, ALuint *speed)
{
	ALuint bytesPerSample   = _alGetBitsFromFormat(*fmt) >> 3;
	ALuint channels = _alGetChannelsFromFormat(*fmt);

        memset(&sdl_info, '\0', sizeof (sdl_info));
        sdl_info.spec.freq     = *speed;
        sdl_info.spec.channels = channels;
        sdl_info.spec.samples  = *deviceBufferSizeInBytes / bytesPerSample;
        sdl_info.spec.format   = _al_AL2ACFMT(*fmt);
        sdl_info.spec.callback = dummy;
	sdl_info.firstTime     = AL_TRUE;

        pSDL_CloseAudio();

        if(pSDL_OpenAudio(&sdl_info.spec, NULL) < 0) {
		fprintf(stderr,
			"No SDL: %s\n", pSDL_GetError());

                return AL_FALSE;
        }

	*deviceBufferSizeInBytes = sdl_info.spec.size;

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
set_read_sdl(UNUSED(void *handle), UNUSED(ALuint *deviceBufferSizeInBytes), UNUSED(ALenum *fmt),
	     UNUSED(ALuint *speed))
{
	return AL_FALSE;
}

static ALboolean
alcBackendSetAttributesSDL_(void *handle, ALuint *deviceBufferSizeInBytes, ALenum *fmt, ALuint *speed)
{
	return sdl_info.mode == ALC_OPEN_INPUT_ ?
		set_read_sdl(handle, deviceBufferSizeInBytes, fmt, speed) :
		set_write_sdl(handle, deviceBufferSizeInBytes, fmt, speed);
}

static void
pause_sdl( UNUSED(void *handle) )
{
}

static void
resume_sdl( UNUSED(void *handle) )
{
}

static ALsizei
capture_sdl( UNUSED(void *handle), UNUSED(void *capture_buffer), UNUSED(int bytesToRead) )
{
	return 0;
}

static ALfloat
get_sdlchannel( UNUSED(void *handle), UNUSED(ALuint channel) )
{
	return 0.0;
}

static int
set_sdlchannel( UNUSED(void *handle), UNUSED(ALuint channel), UNUSED(ALfloat volume) )
{
	return 0;
}

static ALC_BackendOps sdlOps = {
	release_sdl,
	pause_sdl,
	resume_sdl,
	alcBackendSetAttributesSDL_,
	sdl_blitbuffer,
	capture_sdl,
	get_sdlchannel,
	set_sdlchannel
};

void
alcBackendOpenSDL_ (ALC_OpenMode mode, ALC_BackendOps **ops, ALC_BackendPrivateData **privateData)
{
	*privateData = (mode == ALC_OPEN_INPUT_) ? grab_read_sdl() : grab_write_sdl();
	if (*privateData != NULL) {
		*ops = &sdlOps;
	}
}

#endif /* USE_BACKEND_SDL */
