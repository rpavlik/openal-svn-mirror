/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * alsa.c
 *
 * alsa backend
 */

#ifndef _SVID_SOURCE
#define _SVID_SOURCE
#endif /* _SVID_SOURCE */

#include "arch/interface/interface_sound.h"
#include "arch/alsa/alsa.h"

#include <alsa/asoundlib.h>

#include "al_config.h"
#include "al_debug.h"
#include "al_main.h"
#include "al_siteconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

/* alsa stuff */
#define DEFAULT_DEVICE "plughw:0,0"

/* convert from AL to ALSA format */
static int AL2ALSAFMT(ALenum format);
static ALenum ALSA2ALFMT(int fmt, int chans);

/*
 * get either the default device name or something the
 * user specified
 */
void get_device_name(char *retref, int retsize);

struct alsa_info
{
	snd_pcm_t *handle;
	int format;
	fd_set fd_set;
};

void *release_alsa(void *handle)
{
	struct alsa_info *ai = handle;

	if(handle == NULL)
	{
		return NULL;
	}

	snd_pcm_close(ai->handle);

	free(ai);

	return NULL;
}

void *grab_read_alsa( void )
{
	/* no capture support */
	return NULL;
}

void get_device_name(char *retref, int retsize)
{
	Rcvar rcv;

	assert(retref);

	rcv = rc_lookup("alsa-device");
	if(rcv != NULL)
	{
		if(rc_type(rcv) == ALRC_STRING)
		{
			int alsa_card = -1;

			rc_tostr0(rcv, retref, retsize);

			alsa_card = snd_card_get_index(retref);
			if(alsa_card >= 0)
			{
				return;
			}
		}
	}

	assert((int) strlen(DEFAULT_DEVICE) < retsize);
	strcpy(retref, DEFAULT_DEVICE);

	return;
}

void *grab_write_alsa( void )
{
	struct alsa_info *retval;
	snd_pcm_t *handle;
	char card_name[256];
	int err;

	get_device_name(card_name, 256);

	err = snd_pcm_open(&handle, card_name, 0, SND_PCM_STREAM_PLAYBACK);
	if(err < 0)
	{
		const char *serr = snd_strerror(err);

		_alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
			"grab_alsa: init failed %s\n", serr);

		return NULL;
	}

	retval = malloc(sizeof *retval);
	retval->handle = handle;
	retval->format = 0;

	_alBlitBuffer = alsa_blitbuffer;

	_alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
		 "grab_alsa: init ok, using %s\n", card_name);

	return retval;
}

ALboolean set_read_alsa( UNUSED(void *handle),
			 UNUSED(ALuint *bufsiz),
			 UNUSED(ALenum *fmt),
			 UNUSED(ALuint *speed))
{
	/* no capture support */
	return AL_FALSE;
}

ALboolean set_write_alsa(void *handle,
			 ALuint *bufsiz,
			 ALenum *fmt,
			 ALuint *speed)
{
	struct alsa_info *ai = handle;
	snd_pcm_hw_params_t *setup;
	snd_pcm_t *phandle = 0;
	int err;
	ALuint channels = _al_ALCHANNELS(*fmt);

	if(ai == NULL)
	{
		return AL_FALSE;
	}

	if(ai->handle == NULL)
	{
		return AL_FALSE;
	}

	phandle = ai->handle;

	/* did I mention alsa has an ugly api */
	snd_pcm_hw_params_alloca(&setup);
	err = snd_pcm_hw_params_any(phandle, setup);
	if(err < 0)
	{
		_alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
			 "set_write_alsa: Could not query parameters");

		return AL_FALSE;
	}

	/* set format */
	err = snd_pcm_hw_params_set_format(phandle,
					   setup,
					   AL2ALSAFMT(*fmt));
	if(err < 0)
	{
		_alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
			 "set_write_alsa: could not set format");

		return AL_FALSE;
	}

	/* channels */
	err = snd_pcm_hw_params_set_channels(phandle, setup, channels);
	if(err < 0)
	{
		err = snd_pcm_hw_params_get_channels(setup);
		if((err <= 0) || (err > 4)) {
			_alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
				 "set_write_alsa: could not set channels");

			return AL_FALSE;
		}

		channels = err;
	}

	/* sampling rate */
	err = snd_pcm_hw_params_set_rate_near(phandle, setup, *speed, NULL);
	if(err < 0)
	{
		_alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
			 "set_write_alsa: could not set speed");

		return AL_FALSE;
	}
	*speed = err; /* err is sampling rate if >= 0, I guess */

	/*
	 * bufsiz is in bytes, I assume they want samples
	 *
	 * JIV FIXME: obviously I shoudln't be dividing by two,
	 * we need to get the bit depth of the format.
	 */
	*bufsiz = 2 * snd_pcm_hw_params_set_period_size_near(phandle,
							     setup,
							     *bufsiz/2,
							     NULL);
	snd_pcm_hw_params_set_periods_near(phandle, setup, 2, NULL);

	/* please work ugly api */
	err = snd_pcm_hw_params(phandle, setup);
	if(err < 0)
	{
		_alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
			"set_alsa: %s\n", snd_strerror(err));
		return AL_FALSE;
	}

	*fmt = ALSA2ALFMT(*fmt, channels);

	err = snd_pcm_prepare(phandle);
	if(err < 0)
	{
		_alDebug(ALD_MAXIMUS,  __FILE__, __LINE__,
			"set_alsa %s\n", snd_strerror(err));
		return AL_FALSE;
	}

	ai->format = *fmt;

	return AL_TRUE;
}

void alsa_blitbuffer(void *handle, void *data, int bytes)
{
	struct alsa_info *ai = handle;
	snd_pcm_t *phandle = 0;
	char *pdata = data;
	int data_len = bytes;
	int channels = 0;
	int err;

	if(ai == NULL)
	{
		return;
	}

	if(ai->handle == NULL)
	{
		return;
	}

	phandle = ai->handle;

	channels = _al_ALCHANNELS(ai->format);
	assert((channels > 1) && (channels <= 4));

	/*
	 * I'm taking a page from sam's book and not using blocking alsa
	 * writes anymore.   Which is why this code should look familiar
	 */
	while(data_len > 0)
	{
		err = snd_pcm_writei(phandle, pdata, bytes);
		switch(err)
		{
			case -EAGAIN:
				continue;
				break;
			case -ESTRPIPE:
				do
				{
					err = snd_pcm_resume(phandle);
				} while ( err == -EAGAIN );
				break;
			default:
				pdata += err * channels;
				data_len -= err;
				break;
		}

		if(err < 0)
		{
			err = snd_pcm_prepare(phandle);
			if(err < 0)
			{
				const char *serr = snd_strerror(err);

				_alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
			 		"alsa_blitbuffer %s\n", serr);

				return;
			}
		}
	}

	return;
}
	
static int AL2ALSAFMT(ALenum format) {
	switch(format) {
		case AL_FORMAT_STEREO8:  return SND_PCM_FORMAT_U8;
		case AL_FORMAT_MONO8:    return SND_PCM_FORMAT_U8;
		case AL_FORMAT_STEREO16: return SND_PCM_FORMAT_S16;
		case AL_FORMAT_MONO16:   return SND_PCM_FORMAT_S16;
		default: break;
	}

	return -1;
}

static ALenum ALSA2ALFMT(int fmt, int chans) {
	switch(fmt) {
		case SND_PCM_FORMAT_U8:
			if(chans == 1) {
				return AL_FORMAT_MONO8;
			} else if (chans == 2) {
				return AL_FORMAT_STEREO8;
			}
			break;
		case SND_PCM_FORMAT_S16:
			if(chans == 1) {
				return AL_FORMAT_MONO16;
			} else if (chans == 2) {
				return AL_FORMAT_STEREO16;
			}
			break;
		}

	return -1;
}

