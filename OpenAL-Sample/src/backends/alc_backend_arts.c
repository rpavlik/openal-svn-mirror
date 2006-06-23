/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * arts.c
 *
 * arts backend.
 */
#include "al_siteconfig.h"
#include <stdlib.h>
#include "backends/alc_backend.h"

#ifndef USE_BACKEND_ARTS

void
alcBackendOpenARts_ (UNUSED(ALC_OpenMode mode), UNUSED(ALC_BackendOps **ops),
		     ALC_BackendPrivateData **privateData)
{
	*privateData = NULL;
}

#else



#include <AL/al.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <artsc.h>
#include "al_dlopen.h"

#include "al_main.h"
#include "al_debug.h"

#define DEF_SPEED	_ALC_CANON_SPEED
#define DEF_BITS        16
#define DEF_CHANNELS	2

static int openal_arts_ref_count = 0;

typedef struct {
	arts_stream_t stream;
	ALC_OpenMode mode;
} t_arts_handle;

static const char *genartskey(void);
static int openal_load_arts_library(void);

/*
 * aRts library functions.
 */
static int (*parts_init)(void);
static void (*parts_free)(void);
static int (*parts_suspend)(void);
static int (*parts_suspended)(void);
static const char *(*parts_error_text)(int errorcode);
static arts_stream_t (*parts_play_stream)(int rate, int bits, int channels, const char *name);
static arts_stream_t (*parts_record_stream)(int rate, int bits, int channels, const char *name);
static void (*parts_close_stream)(arts_stream_t stream);
static int (*parts_read)(arts_stream_t stream, void *buffer, int count);
static int (*parts_write)(arts_stream_t stream, const void *buffer, int count);
static int (*parts_stream_set)(arts_stream_t stream, arts_parameter_t param, int value);
static int (*parts_stream_get)(arts_stream_t stream, arts_parameter_t param);

#define ARTS_LIBRARY "libartsc.so"

#ifdef OPENAL_DLOPEN_ARTS
#define myDlopen(n) alDLOpen_(n)
#define myDlerror() alDLError_()
#define myDlsym(h,t,s) (t alDLFunSym_(h,#s))
#define myDlclose(h) alDLClose_(h)
#else
#define myDlopen(n) ((AL_DLHandle)0xF00DF00D)
#define myDlerror() ""
#define myDlsym(h,t,s) (&s)
#define myDlclose(h)
#endif

#define OPENAL_LOAD_ARTS_SYMBOL(h,t,s)		\
  p##s = myDlsym(h, t, s);			\
  if (p##s == NULL) \
    { \
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__, \
                "could not resolve symbol '%s': %s", \
                #s, myDlerror ()); \
      myDlclose (h); \
      h = NULL; \
      return 0; \
    }

static int openal_load_arts_library(void)
{
	static AL_DLHandle handle = (AL_DLHandle)0;
    
	/* already loaded? */
	if (handle != (AL_DLHandle)0)
		return 1;

	/* clear error state */
	(void) myDlerror ();

	handle = myDlopen (ARTS_LIBRARY);
	if (handle == (AL_DLHandle) 0)
		{
			_alDebug (ALD_CONTEXT, __FILE__, __LINE__,
				  "could not open '%s': %s", ARTS_LIBRARY, myDlerror ());
			return 0;
		}

	OPENAL_LOAD_ARTS_SYMBOL(handle, (int (*)(void)), arts_init);
	OPENAL_LOAD_ARTS_SYMBOL(handle, (void (*)(void)), arts_free);
	OPENAL_LOAD_ARTS_SYMBOL(handle, (int (*)(void)), arts_suspend);
	OPENAL_LOAD_ARTS_SYMBOL(handle, (int (*)(void)), arts_suspended);
	OPENAL_LOAD_ARTS_SYMBOL(handle, (const char *(*)(int)), arts_error_text);
	OPENAL_LOAD_ARTS_SYMBOL(handle, (arts_stream_t (*)(int, int, int, const char*)), arts_play_stream);
	OPENAL_LOAD_ARTS_SYMBOL(handle, (arts_stream_t (*)(int, int, int, const char*)), arts_record_stream);
	OPENAL_LOAD_ARTS_SYMBOL(handle, (void (*)(arts_stream_t)), arts_close_stream);
	OPENAL_LOAD_ARTS_SYMBOL(handle, (int (*)(arts_stream_t, void*, int)), arts_read);
	OPENAL_LOAD_ARTS_SYMBOL(handle, (int (*)(arts_stream_t, const void*, int)), arts_write);
	OPENAL_LOAD_ARTS_SYMBOL(handle, (int (*)(arts_stream_t, arts_parameter_t, int)), arts_stream_set);
	OPENAL_LOAD_ARTS_SYMBOL(handle, (int (*)(arts_stream_t, arts_parameter_t)), arts_stream_get);

	return 1;
}

static void *grab_read_arts(void) {
	return NULL;
}

static void *grab_write_arts(void) {
        int err;
        t_arts_handle * ahandle;

        if (!openal_load_arts_library()) {
                return NULL;
        }
    
        if (!(ahandle=malloc(sizeof(t_arts_handle))))
                return NULL;
        
        if (openal_arts_ref_count==0) {
                err = parts_init();

                if(err < 0) {
                        fprintf(stderr, "aRTs init failed: %s\n",
                                parts_error_text(err));
                        free(ahandle);
                        return NULL;
                }
        }

        openal_arts_ref_count++;
        
#if 1
        ahandle->stream = NULL;
#else
        ahandle->stream = parts_play_stream(DEF_SPEED,
                                            DEF_BITS,
                                            DEF_CHANNELS,
				 	    genartskey());
#endif
        ahandle->mode = ALC_OPEN_OUTPUT_;
	fprintf(stderr, "arts grab audio ok\n");

	_alDebug(ALD_CONTEXT, __FILE__, __LINE__,
		"arts grab audio ok");

        return ahandle;
}

static void arts_blitbuffer(void *handle, const void *data, int bytesToWrite)  {
	t_arts_handle * ahandle = (t_arts_handle *) handle;

	if ((ahandle == NULL)||(ahandle->stream == NULL)) {
		return;
	}

	parts_write(ahandle->stream, data, bytesToWrite);
}

static void release_arts(void *handle) {
	t_arts_handle * ahandle = (t_arts_handle *) handle;

	if ((ahandle == NULL)||(ahandle->stream == NULL)) {
		return;
	}

        openal_arts_ref_count--;
        
	parts_close_stream(ahandle->stream);
        free(ahandle);

        if (openal_arts_ref_count==0)
                parts_free();

	return;
}

static const char *genartskey(void) {
	static char retval[1024];

	snprintf(retval, sizeof(retval), "openal%d", getpid());

	return retval;
}

static ALboolean set_write_arts(void *handle,
				ALuint *deviceBufferSizeInBytes,
				ALenum *fmt,
				ALuint *speed) {
	t_arts_handle * ahandle = (t_arts_handle *) handle;

	if (ahandle == NULL) {
		return AL_FALSE;
	}

#if 0
	fprintf(stderr, "set_arts forcing speed from %d to 44100\n", *speed);

        /* @@@ FIXME: What was this good for?! */
	*speed = 44100;
#endif

        if (ahandle->stream != NULL)
                parts_close_stream(ahandle->stream);

        /* According to the aRtsC library source, this function
         * returns 0 if it fails.
         */
	ahandle->stream = parts_play_stream(*speed,
					    _alGetBitsFromFormat(*fmt),
					    _alGetChannelsFromFormat(*fmt),
                                            genartskey());
        
        if (ahandle->stream == 0) {
                ahandle->stream = NULL;
                return AL_FALSE;
        }
        
	parts_stream_set(ahandle->stream, ARTS_P_BUFFER_SIZE, *deviceBufferSizeInBytes);
	parts_stream_set(ahandle->stream, ARTS_P_BLOCKING, 1);

	*deviceBufferSizeInBytes = parts_stream_get(ahandle->stream, ARTS_P_BUFFER_SIZE);

        return AL_TRUE;
}

static ALboolean set_read_arts(UNUSED(void *handle),
			       UNUSED(ALuint *deviceBufferSizeInBytes),
			       UNUSED(ALenum *fmt),
			       UNUSED(ALuint *speed)) {
	return AL_FALSE;
}

static ALboolean
alcBackendSetAttributesARts_(void *handle, ALuint *deviceBufferSizeInBytes, ALenum *fmt, ALuint *speed)
{
	return ((t_arts_handle *)handle)->mode == ALC_OPEN_INPUT_ ?
		set_read_arts(handle, deviceBufferSizeInBytes, fmt, speed) :
		set_write_arts(handle, deviceBufferSizeInBytes, fmt, speed);
}

static void
pause_arts( UNUSED(void *handle) )
{
}

static void
resume_arts( UNUSED(void *handle) )
{
}


static ALsizei
capture_arts( UNUSED(void *handle), UNUSED(void *capture_buffer), UNUSED(int bytesToRead) )
{
	return 0;
}

static ALfloat
get_artschannel( UNUSED(void *handle), UNUSED(ALuint channel) )
{
	return 0.0;
}

static int
set_artschannel( UNUSED(void *handle), UNUSED(ALuint channel), UNUSED(ALfloat volume) )
{
	return 0;
}

static ALC_BackendOps artsOps = {
	release_arts,
	pause_arts,
	resume_arts,
	alcBackendSetAttributesARts_,
	arts_blitbuffer,
	capture_arts,
	get_artschannel,
	set_artschannel
};

void
alcBackendOpenARts_ (ALC_OpenMode mode, ALC_BackendOps **ops, ALC_BackendPrivateData **privateData)
{
	*privateData = (mode == ALC_OPEN_INPUT_) ? grab_read_arts() : grab_write_arts();
	if (*privateData != NULL) {
		*ops = &artsOps;
	}
}

#endif /* USE_BACKEND_ARTS */
