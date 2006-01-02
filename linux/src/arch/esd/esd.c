/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * esd.c
 *
 * esd backend.
 */
#include "al_siteconfig.h"

#include <AL/al.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "al_main.h"
#include "al_debug.h"

#include "arch/esd/esd.h"
#include "arch/interface/interface_sound.h"

#include "alc/alc_context.h"

#if OPENAL_DLOPEN_ESD
#include <dlfcn.h>
#endif

#include <esd.h>

#define DEF_SPEED	_ALC_CANON_SPEED
#define DEF_SIZE	_AL_DEF_BUFSIZ
#define DEF_SAMPLES     (DEF_SIZE / 2)
#define DEF_CHANNELS	2
#define DEF_FORMAT      AL_FORMAT_STEREO16

#define ESD_KEY        "openal"
#define ESD_NAMELEN    1024

static const char *genesdkey(void);
static int openal_load_esd_library(void);


/*
 * ESD library functions.
 */
static int (*pesd_open_sound)( const char *host );
static int (*pesd_play_stream)( esd_format_t format, int rate, 
		     const char *host, const char *name );
static int (*pesd_standby)( int esd );
static int (*pesd_resume)( int esd );
static int (*pesd_close)( int esd );

/*
 * ESD library handle.
 */
static void * esd_lib_handle = NULL;

static int openal_load_esd_library(void)
{
#if OPENAL_DLOPEN_ESD
        char * error = NULL;
#endif
    
	if (esd_lib_handle != NULL)
		return 1;  /* already loaded. */

	#if OPENAL_DLOPEN_ESD
		#define OPENAL_LOAD_ESD_SYMBOL(x) p##x = dlsym(esd_lib_handle, #x); \
                                                   error = dlerror(); \
                                                   if (p##x == NULL) { \
                                                           fprintf(stderr,"Could not resolve ESoundD symbol %s: %s\n", #x, ((error!=NULL)?(error):("(null)"))); \
                                                           dlclose(esd_lib_handle); esd_lib_handle = NULL; \
                                                           return 0; }
                dlerror(); /* clear error state */
		esd_lib_handle = dlopen("libesd.so", RTLD_LAZY | RTLD_GLOBAL);
                error = dlerror();
		if (esd_lib_handle == NULL) {
                        fprintf(stderr,"Could not open ESoundD library: %s\n",((error!=NULL)?(error):("(null)")));
			return 0;
                }
        #else
		#define OPENAL_LOAD_ESD_SYMBOL(x) p##x = x;
		esd_lib_handle = (void *) 0xF00DF00D;
	#endif

        OPENAL_LOAD_ESD_SYMBOL(esd_open_sound);
        OPENAL_LOAD_ESD_SYMBOL(esd_standby);
        OPENAL_LOAD_ESD_SYMBOL(esd_resume);
        OPENAL_LOAD_ESD_SYMBOL(esd_play_stream);
        OPENAL_LOAD_ESD_SYMBOL(esd_close);
                
	return 1;
}

static fd_set esd_fd_set;

typedef struct esd_info_s {
	esd_format_t fmt;
	ALuint speed;
	const char *espeaker;
	char name[ESD_NAMELEN];
	int socket;

	ALboolean paused;
	int esdhandle; /* esd handle */
} esd_openal_info_t;

static esd_openal_info_t esd_info;

void *grab_write_esd(void) {
	esd_format_t fmt;
	int socket;
	const char *esdkey = genesdkey();
	const char *espeaker = getenv("ESPEAKER");
	int esd;

        if (!openal_load_esd_library()) {
                return NULL;
        }
    
	esd = pesd_open_sound(espeaker);
	if(esd < 0) {
		fprintf(stderr, "esd open sound failed.\n");
		return NULL;
	}

	fmt = ESD_STREAM | ESD_PLAY;

	switch(DEF_CHANNELS) {
		case 1: fmt |= ESD_MONO; break;
		case 2: fmt |= ESD_STEREO; break;
		default: break;
	}

	switch(_alGetBitsFromFormat(DEF_FORMAT)) {
		case 8:  fmt |= ESD_BITS8; break;
		case 16: fmt |= ESD_BITS16; break;
		default: break;
	}

	socket = pesd_play_stream(fmt, DEF_SPEED, espeaker, esdkey);

	if(socket < 0) {
		fprintf(stderr, "esd play stream failed.\n");
		return NULL;
	}

	fprintf(stderr, "esd grab audio ok\n");

	_alDebug(ALD_CONTEXT, __FILE__, __LINE__,
		"esd grab audio ok");

	esd_info.speed     = DEF_SPEED;
	esd_info.fmt       = fmt;
	esd_info.socket    = socket;
	esd_info.espeaker  = espeaker;
	esd_info.paused    = AL_FALSE;
	esd_info.esdhandle = esd;

	strncpy(esd_info.name, esdkey, ESD_NAMELEN);

        return &esd_info;
}

void *grab_read_esd(void) {
	return NULL;
}

void esd_blitbuffer(void *handle, void *dataptr, int bytes_to_write)  {
	esd_openal_info_t *eh;
	struct timeval tv = { 0, 9000000 };
	int iterator = 0;
	int err;
	int fd;

	if(handle == NULL) {
		return;
	}

	eh = (esd_openal_info_t *) handle;

	if(eh->paused == AL_TRUE) {
		/* don't write to paused audio devices, just sleep */
		tv.tv_usec = 10000;

		select(0, NULL, NULL, NULL, &tv);

		return;
	}

	fd = eh->socket;

	for(iterator = bytes_to_write; iterator > 0; ) {
		FD_ZERO(&esd_fd_set);
		FD_SET(fd, &esd_fd_set);

		err = select(fd + 1, NULL, &esd_fd_set, NULL, &tv);
		if(FD_ISSET(fd, &esd_fd_set) == 0) {
			/* timeout occured, don't try and write */
			fprintf(stderr, "esd_blitbuffer: timeout occured\n");

			assert(0);
			return;
		}

		assert(iterator > 0);
		assert(iterator <= bytes_to_write);

		err = write(fd,
			    (char *) dataptr + bytes_to_write - iterator,
			    iterator);
		if(err < 0) {
			assert(0);
#ifdef DEBUG_MAXIMUS
			perror("write");
#endif
			return;
		}

		iterator -= err;
	};

        return;
}

void release_esd(void *handle) {
	esd_openal_info_t *eh;

	if(handle == NULL) {
		return;
	}

	eh = (esd_openal_info_t *) handle;

	pesd_close(eh->esdhandle);

	return;
}

static const char *genesdkey(void) {
	static char retval[ESD_NAMELEN];

	snprintf(retval, sizeof(retval), "openal-%d\n", (int) getpid());

	return retval;
}

void pause_esd(void *handle) {
	esd_openal_info_t *eh;

	if(handle == NULL) {
		return;
	}

	eh = (esd_openal_info_t *) handle;

	eh->paused = AL_TRUE;
	pesd_standby(eh->esdhandle);

	return;
}

void resume_esd(void *handle) {
	esd_openal_info_t *eh;

	if(handle == NULL) {
		return;
	}

	eh = (esd_openal_info_t *) handle;

	eh->paused = AL_FALSE;
	pesd_resume(eh->esdhandle);

	return;
}

ALboolean set_write_esd(UNUSED(void *handle),
		  UNUSED(ALuint *bufsiz),
		  ALenum *fmt,
		  ALuint *speed) {
	esd_openal_info_t *eh;
	int socket;
	ALuint chans = _alGetChannelsFromFormat(*fmt);

	if(handle == NULL) {
		return AL_FALSE;
	}

	eh = (esd_openal_info_t *) handle;

	close(eh->socket);

	eh->fmt = ESD_STREAM | ESD_PLAY;

	switch(chans) {
		case 1: eh->fmt |= ESD_MONO; break;
		case 2: eh->fmt |= ESD_STEREO; break;
		default: break;
	}

	switch(_alGetBitsFromFormat(*fmt)) {
		case 8:  eh->fmt |= ESD_BITS8; break;
		case 16: eh->fmt |= ESD_BITS16; break;
		default: break;
	}

	eh->speed = *speed;

	socket = pesd_play_stream(eh->fmt,
				 eh->speed,
				 eh->espeaker,
				 eh->name);
	if(socket < 0) {
		return AL_FALSE;
	}

	eh->socket = socket;
	eh->paused = AL_FALSE;

        return AL_TRUE;
}

ALboolean set_read_esd(UNUSED(void *handle),
		  UNUSED(ALuint *bufsiz),
		  UNUSED(ALenum *fmt),
		  UNUSED(ALuint *speed)) {
	return AL_FALSE;
}
