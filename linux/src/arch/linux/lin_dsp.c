/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * lin_dsp.c
 *
 * functions related to the aquisition and management of /dev/dsp under
 * linux.
 *
 */
#include "al_siteconfig.h"

#include <AL/al.h>
#include <AL/alext.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/soundcard.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "al_config.h"
#include "al_main.h"
#include "al_debug.h"
#include "alc/alc_context.h"
#include "arch/interface/interface_sound.h"

#ifdef WORDS_BIGENDIAN
#define AFMT_S16 AFMT_S16_BE
#else
#define AFMT_S16 AFMT_S16_LE
#endif /* WORDS_BIGENDIAN */

/*
 * DONTCARE was (1 << 16), but that breaks 4Front's commercial drivers.
 *  According to their reference manual, it musts be (2 << 16) or higher
 *  or it is ignored, which is not good. Thanks to 4Front for tracking this
 *  down.   --ryan.
 */
#define DONTCARE ( 8 << 16)

/* convert an alc channel to a linux dsp channel */
static int alcChannel_to_dsp_channel(ALCenum alcc);

/* /dev/dsp variables */
static fd_set dsp_fd_set;
static int mixer_fd    = -1; /* /dev/mixer file read/write descriptor */
static ALboolean use_select = AL_TRUE;

/* gets user prefered path */
static const char *lin_getwritepath(void);
static const char *lin_getreadpath(void);
static int aquire_read(void);
static int grab_mixerfd(void);
static int try_to_open(const char **paths, int n_paths, const char **used_path, int mode);

/* set the params associated with a file descriptor */
static int set_fd(int dsp_fd, ALboolean readable,
			      ALuint *bufsiz,
			      ALuint *fmt,
			      ALuint *speed,
			      ALuint *channels);


/* convert the format channel from /dev/dsp to openal format */
static int LIN2ALFMT(int fmt, int channels)
{
	switch(fmt)
	{
		case AFMT_U8:
			switch(channels)
			{
				case 1: return AL_FORMAT_MONO8;
				case 2: return AL_FORMAT_STEREO8;
				case 4: return AL_FORMAT_QUAD8_LOKI;
				default: return -1;
			}
			break;
		case AFMT_S16:
			switch(channels)
			{
				case 1: return AL_FORMAT_MONO16;
				case 2: return AL_FORMAT_STEREO16;
				case 4: return AL_FORMAT_QUAD16_LOKI;
				default: return -1;
			}
			break;
		default:
#ifdef DEBUG_MAXIMUS
			fprintf(stderr, "unsupported dsp format\n");
#endif
			return -1;
			break;
	}

	return -1;
}

/* convert the format channel from openal to /dev/dsp format */
static int AL2LINFMT(int fmt)
{
	switch(fmt) {
		case AL_FORMAT_MONO16:
		case AL_FORMAT_STEREO16:
		case AL_FORMAT_QUAD16_LOKI:
			return AFMT_S16;
			break;
		case AL_FORMAT_MONO8:
		case AL_FORMAT_STEREO8:
		case AL_FORMAT_QUAD8_LOKI:
			return AFMT_U8;
			break;
		default:
#ifdef DEBUG_MAXIMUS
		  fprintf(stderr, "unknown format 0x%x\n", fmt);
#endif
		  break;
	}

	return -1;
}


/*
 * Disable non-blocking on a file descriptor. Returns non-zero on
 *  success, zero on failure.  --ryan.
 */
static int toggle_nonblock(int fd, int state)
{
	int retval = 0;
	int flags = fcntl(fd, F_GETFL);
	if (flags != -1) {
		if (state) {
			flags |= O_NONBLOCK;
		} else {
			flags &= ~O_NONBLOCK;
		}

		if(fcntl(fd, F_SETFL, flags) != -1) {
			retval = 1;
		}
	}

	if (!retval) {
		perror("fcntl");
	}

	return(retval);
}


/*
 *
 *  Format of divisor is bit field where:
 *
 *
 *         MSHW          LSHW
 *  [ some big value |     x  ]
 *
 *  where x is translated into 2^x, and used as the
 *  dma buffer size.
 *
 */
static void *grab_write_native(void)
{
	static int write_fd;
	Rcvar rc_use_select;
	const char *writepath = NULL;
	int divisor = DONTCARE | _alSpot( _ALC_DEF_BUFSIZ );
	const char *tried_paths[] = {
		"",
		"/dev/sound/dsp",
		"/dev/dsp"
	};
	tried_paths[0] = lin_getwritepath();
	write_fd = try_to_open(tried_paths, 3, &writepath, O_WRONLY | O_NONBLOCK);
	if(write_fd < 0) {
		perror("open /dev/[sound/]dsp");
		return NULL;
	}

	if(ioctl(write_fd, SNDCTL_DSP_SETFRAGMENT, &divisor) < 0) {
		perror("ioctl SETFRAGMENT grab");
	}

	toggle_nonblock(write_fd, 0);

	/* now get mixer_fd */
	mixer_fd = grab_mixerfd();

	/*
	 * Some drivers, ie aurreal ones, don't implemented select.
	 * I like to use select, as it ensures that we don't hang on
	 * a bum write forever, but if you're in the unadmirable position
	 * of having one of these cards I'm sure it's a small comfort.
	 *
	 * So, we have a special alrc var: native-use-select, that, if
	 * set to #f, causes us to not do a select on the fd before
	 * writing to it.
	 */
	use_select = AL_TRUE;

	rc_use_select = rc_lookup("native-use-select");
	if(rc_use_select != NULL) {
		if(rc_type(rc_use_select) == ALRC_BOOL) {
			use_select = rc_tobool(rc_use_select);
		}
	}

#ifdef DEBUG
	fprintf(stderr, "grab_native: (path %s fd %d)\n", writepath, write_fd);
#endif

	return &write_fd;
}

/*
 *
 *  Format of divisor is bit field where:
 *
 *
 *         MSHW          LSHW
 *  [ some big value |     x  ]
 *
 *  where x is translated into 2^x, and used as the
 *  dma buffer size.
 *
 */
static void *grab_read_native(void)
{
	static int read_fd;

	read_fd = aquire_read();
	if( read_fd < 0) {
		return NULL;
	}

	return &read_fd;
}

void *
_alcBackendOpenNative( _ALCOpenMode mode )
{
	return mode == _ALC_OPEN_INPUT ? grab_read_native() : grab_write_native();
}

static int grab_mixerfd(void) {
	const char *tried_paths[] = {
		"/dev/sound/mixer",
		"/dev/mixer"
	};
	mixer_fd = try_to_open(tried_paths, 2, NULL, O_WRONLY | O_NONBLOCK);

	if(mixer_fd > 0) {
		toggle_nonblock(mixer_fd, 0);
		return mixer_fd;
	} else {
		perror("open /dev/[sound/]mixer");
	}

	return -1;
}

void release_native(void *handle) {
	int handle_fd;

	if(handle == NULL) {
		return;
	}

	handle_fd = *(int *) handle;

	if(ioctl(handle_fd, SNDCTL_DSP_RESET) < 0) {
#ifdef DEBUG
		fprintf(stderr, "Couldn't reset dsp\n");
#endif
	}

	ioctl(handle_fd, SNDCTL_DSP_SYNC, NULL);
	if((close(handle_fd) < 0) || (close(mixer_fd) < 0)) {
		return;
	}

	*(int *) handle = -1;
	mixer_fd        = -1;

	return;
}

ALfloat
get_nativechannel(UNUSED(void *handle), ALuint channel)
{
	int request = alcChannel_to_dsp_channel(channel);
	int retval;
	if(ioctl(mixer_fd, MIXER_READ(request), &retval) < 0) {
		return -1;
	}
	return (retval >> 8) / 100.0;
}


/*
 * Okay:
 *
 * Set audio channel expects an integer, in the range of
 * 0 - 100.  But wait!  It expects the integer to be
 * partitioned into a 16bit empty, L/R channel pair (high bits left,
 * low bits right), each 8 bit pair in the range 0 - 100.
 *
 * Kludgey, and obviously not the right way to do this
 */
int set_nativechannel(UNUSED(void *handle), ALuint channel, ALfloat volume) {
	int request = alcChannel_to_dsp_channel(channel);
	int unnormalizedvolume;

	unnormalizedvolume = volume * 100;
	unnormalizedvolume <<= 8;
	unnormalizedvolume += (volume * 100);

	if(ioctl(mixer_fd, MIXER_WRITE(request), &unnormalizedvolume) < 0) {
		return -1;
	}

	return 0;
}

/* convert the mixer channel from ALC to /dev/mixer format */
static int alcChannel_to_dsp_channel(ALCenum alcc) {
	switch(alcc) {
		case ALC_CHAN_MAIN_LOKI: return SOUND_MIXER_VOLUME;
		case ALC_CHAN_CD_LOKI:   return SOUND_MIXER_CD;
		case ALC_CHAN_PCM_LOKI:  return SOUND_MIXER_PCM;
		default: return -1;
	}

	return -1;
}

void pause_nativedevice(void *handle) {
	int fd;

	if(handle == NULL) {
		return;
	}

	fd = *(int *) handle;

	toggle_nonblock(fd, 1);

#if 0
	if(ioctl(fd, SNDCTL_DSP_POST, 0) == -1) {
		perror("ioctl");
	}
#endif

	return;
}

void resume_nativedevice(void *handle) {
	int fd;

	if(handle == NULL) {
		return;
	}

	fd = *(int *) handle;

	toggle_nonblock(fd, 0);

/*
	if(ioctl(fd, SNDCTL_DSP_SYNC, 0) == -1) {
		perror("ioctl");
	}
 */

	return;
}

static const char *lin_getwritepath(void) {
	Rcvar devdsp_path = rc_lookup("lin-dsp-path");
	static char retval[65]; /* FIXME */

	if(devdsp_path == NULL) {
		return NULL;
	}

	switch(rc_type(devdsp_path)) {
		case ALRC_STRING:
			rc_tostr0(devdsp_path, retval, 64);
			break;
		default:
			return NULL;
	}

	return retval;
}

static const char *lin_getreadpath(void) {
	Rcvar devdsp_path = rc_lookup("lin-dsp-read-path");
	static char retval[65]; /* FIXME */

	if(devdsp_path == NULL) {
		/*
		 * no explicit read path?  try the default
		 * path.
		 */
		devdsp_path = rc_lookup("lin-dsp-path");
	}

	if(devdsp_path == NULL) {
		return NULL;
	}

	switch(rc_type(devdsp_path)) {
		case ALRC_STRING:
			rc_tostr0(devdsp_path, retval, 64);
			break;
		default:
			return NULL;
	}


	/*
	 * stupid.  /dev/dsp1 cannot be used for reading,
	 * only /dev/dsp
	 */
	if(retval[strlen(retval) - 1] == '1') {
		retval[strlen(retval) - 1] = '\0';
	}

	return retval;
}

/* capture data from the audio device */
ALsizei capture_nativedevice(void *handle,
			  void *capture_buffer,
			  int bufsiz) {
	int read_fd = *(int *)handle;
	int retval;

	retval = read(read_fd, capture_buffer, bufsiz);
	return retval > 0 ? retval : 0;
}

static int aquire_read(void) {
	int read_fd;
	const char *readpath = NULL;
	int divisor = _alSpot(_ALC_DEF_BUFSIZ) | (1<<16);
	const char *tried_paths[] = {
		"",
		"/dev/sound/dsp",
		"/dev/dsp"
	};
	tried_paths[0] = lin_getreadpath();
	read_fd = try_to_open(tried_paths, 3, &readpath, O_RDONLY | O_NONBLOCK);
	if(read_fd >= 0) {
#if 0 /* Reads should be non-blocking */
		toggle_nonblock(read_fd, 0);
#endif
		if(ioctl(read_fd, SNDCTL_DSP_SETFRAGMENT, &divisor) < 0) {
			perror("ioctl SETFRAGMENT");
		}
	}

	return read_fd;
}

static ALboolean set_write_native(UNUSED(void *handle),
				  ALuint *bufsiz,
				  ALenum *fmt,
				  ALuint *speed) {
	int write_fd = *(int *)handle;
	ALuint linformat;
	ALuint channels = _alGetChannelsFromFormat(*fmt);
	int err;

	if(write_fd < 0) {
		return AL_FALSE;
	}

	linformat = AL2LINFMT(*fmt);

	err = set_fd(write_fd, AL_FALSE, bufsiz, &linformat, speed, &channels);
	if(err < 0) {
#ifdef DEBUG
		fprintf(stderr, "Could not do write_fd\n");
#endif
		return AL_FALSE;
	}

	/* set format for caller */
	*fmt = LIN2ALFMT(linformat, channels);

	return AL_TRUE;
}

static ALboolean set_read_native(UNUSED(void *handle),
				 ALuint *bufsiz,
				 ALenum *fmt,
				 ALuint *speed) {
	int read_fd = *(int *)handle;
	ALuint linformat;
	ALuint channels = 1;

	linformat = AL2LINFMT(*fmt);

	if(set_fd(read_fd, AL_TRUE, bufsiz, &linformat, speed, &channels) >= 0) {
		/* set format for caller */
		*fmt = LIN2ALFMT(linformat, channels);

		return AL_TRUE;
	}

	return AL_FALSE;
}

ALboolean
_alcBackendSetAttributesNative(_ALCOpenMode mode, void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed)
{
	return mode == _ALC_OPEN_INPUT ?
		set_read_native(handle, bufsiz, fmt, speed) :
		set_write_native(handle, bufsiz, fmt, speed);
}

void native_blitbuffer(void *handle, void *dataptr, int bytes_to_write) {
	struct timeval tv = { 0, 800000 }; /* at most .8 secs */
	int iterator = 0;
	int err;
	int fd;

	if(handle == NULL) {
		return;
	}

	fd = *(int *) handle;

	assert( fd >= 0 );

	for(iterator = bytes_to_write; iterator > 0; ) {
		FD_ZERO(&dsp_fd_set);
		FD_SET(fd, &dsp_fd_set);

		if(use_select == AL_TRUE) {
			err = select(fd + 1, NULL, &dsp_fd_set, NULL, &tv);
			if(FD_ISSET(fd, &dsp_fd_set) == 0) {
				/*
				 * error or timeout occured, don't try
				 * and write.
				 */
				fprintf(stderr,
				"native_blitbuffer: select error occured\n");
				return;
			}
		}

		assert(iterator > 0);
		assert(iterator <= bytes_to_write);

		err = write(fd,
			    (char *) dataptr + bytes_to_write - iterator,
			    iterator);

		if(err < 0) {
#ifdef DEBUG_MAXIMUS
			fprintf( stderr, "write error: ( fd %d error %s )\n",
				fd, strerror(err));
#endif
			assert( 0 );
			return;
		}

		iterator -= err;
	};

	return;
}

static int set_fd(int dsp_fd, ALboolean readable,
		     ALuint *bufsiz,
		     ALuint *fmt,
		     ALuint *speed,
		     ALuint *channels)
{
	struct audio_buf_info info;

	if(dsp_fd < 0) {
		return -1;
	}

#ifdef DEBUG
	fprintf( stderr, "set_fd in: bufsiz %d fmt 0x%x speed %d channels %d\n",
		 *bufsiz, *fmt, *speed, *channels );
#endif


#if 0 /* This code breaks 4Front's commercial drivers. Just say no. --ryan. */
{
	int divisor = DONTCARE | _alSpot( *bufsiz );
	if( ioctl(dsp_fd, SNDCTL_DSP_SETFRAGMENT, &divisor ) < 0) {
#ifdef DEBUG
		perror("ioctl SETFRAGMENT");
#endif
	}
}
#endif


	/* reset card defaults */
	if(ioctl(dsp_fd, SNDCTL_DSP_RESET, NULL) < 0) {
#ifdef DEBUG
		perror("set_devsp reset ioctl");
#endif
		return AL_FALSE;
	}

	if(ioctl(dsp_fd, SNDCTL_DSP_SETFMT, fmt) < 0) {
#ifdef DEBUG
		fprintf(stderr, "fmt %d\n", *fmt);
		perror("set_devsp format ioctl");
#endif
		return AL_FALSE;
	}

	if(ioctl(dsp_fd, SNDCTL_DSP_CHANNELS, channels)) {
#ifdef DEBUG
		fprintf(stderr, "channels %d\n", *channels);
		perror("set_devsp channels ioctl");
#endif
		return AL_FALSE;
	}

	if( readable == AL_TRUE ) {
		/*
		 * This is for reading.  Don't really use
		 * the speed argument.
		 */
		*speed = 16000;

		/* Try to set the speed (ignore value), then read it back */
                ioctl(dsp_fd, SNDCTL_DSP_SPEED, speed);
                if (ioctl(dsp_fd, SOUND_PCM_READ_RATE, speed) < 0) {
#ifdef DEBUG
			char errbuf[256];

			snprintf(errbuf, sizeof(errbuf), "(fd %d speed %d)", dsp_fd, *speed);

			perror(errbuf);
			return AL_FALSE;
#endif
                }
/*printf("Set recording rate: %d\n", *speed);*/

	} else {
		/* writable, set speed, otherwise no */
		if(ioctl(dsp_fd, SNDCTL_DSP_SPEED, speed) < 0) {
#ifdef DEBUG
			char errbuf[256];

			snprintf(errbuf, sizeof(errbuf), "(fd %d speed %d)", dsp_fd, *speed);

			perror(errbuf);
			return AL_FALSE;
#endif
		}

		if(ioctl(dsp_fd, SNDCTL_DSP_GETOSPACE, &info) < 0) {
#ifdef DEBUG
			perror("ioctl SNDCTL_DSP_GETOSPACE");
#endif
		}

		/* set bufsiz correctly */

		*bufsiz = info.fragsize;

#ifdef DEBUG
		if( *bufsiz & (*bufsiz - 1) ) {
			fprintf( stderr, "Non power-of-two bufsiz %d\n",
				*bufsiz );
		}
#endif
	}

#ifdef DEBUG
	fprintf( stderr, "set_fd out: bufsiz %d fmt 0x%x speed %d channels %d\n",
		 *bufsiz, *fmt, *speed, *channels );
#endif

	return 0;
}

int try_to_open(const char **paths, int n_paths, const char **used_path, int mode)
{
	int i, fd = -1;
	for(i = 0; i != n_paths; ++i)
	{
		if(paths[i])
		{
			if(used_path) *used_path = paths[i];
			fd = open(paths[i], mode);
			if(fd >= 0) return fd;
		}
	}
	return fd;
}

