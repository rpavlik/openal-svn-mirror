/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *  alsa.c

   ALSA backend for OpenAL

    Dirk Ehlke
    EMail: dehlke@mip.informatik.uni-kiel.de

    Multimedia Information Processing
    Christian-Albrechts-University of Kiel

    2002/06/13
*/


#ifndef _SVID_SOURCE
#define _SVID_SOURCE
#endif /* _SVID_SOURCE */

#include "arch/interface/interface_sound.h"
#include "arch/alsa/alsa.h"

#include "al_config.h"
#include "al_debug.h"
#include "al_main.h"
#include "al_siteconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <alsa/asoundlib.h>

/* alsa stuff */
#define DEFAULT_DEVICE "plughw:0,0"

/* convert from AL to ALSA format */
static int AL2ALSAFMT(ALenum format);
static int FRAMESIZE(snd_pcm_format_t fmt, unsigned int chans);


/*
 * get either the default device name or something the
 * user specified
 */
void get_device_name(char *retref, int retsize);

struct alsa_info
{
	snd_pcm_t *handle;
	snd_pcm_format_t format;
	unsigned int speed;
	unsigned int channels;
	unsigned int framesize;
	unsigned int periods;
	snd_pcm_uframes_t bufframesize;
	fd_set fd_set;
};

void *release_alsa(void *handle)
{
	struct alsa_info *ai = handle;

	if(handle == NULL)
	  return NULL;

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
			rc_tostr0(rcv, retref, retsize);
                        return;

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
			"grab_alsa: init failed: %s\n", serr);

		return NULL;
	}

	retval = malloc(sizeof *retval);
	retval->handle      = handle;
	retval->format      = 0;
	retval->channels    = 0;
	retval->speed       = 0;
	retval->framesize   = 0;
	retval->bufframesize= 0;
        retval->periods     = 0;

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
	int err, dir;

	if( (ai == NULL) || (ai->handle == NULL) )
	  return AL_FALSE;


        ai->channels    = (unsigned int) _al_ALCHANNELS(*fmt);
        ai->format      = (unsigned int) AL2ALSAFMT(*fmt);
        ai->speed       = (unsigned int) *speed;
        ai->framesize   = (unsigned int) FRAMESIZE(ai->format, ai->channels);
	ai->bufframesize= (snd_pcm_uframes_t) *bufsiz / ai->framesize * 4;
        ai->periods     = 2;

        _alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
	   "alsa info:\n"\
	   " channels: %u\n"\
	   " format: %u\n"\
	   " speed: %u\n"\
	   " framesize: %u\n"\
	   " bufframesize: %u\n"\
	   " periods: %u",
	 ai->channels, ai->format, ai->speed, ai->framesize, ai->bufframesize, ai->periods);

        phandle = ai->handle;

	snd_pcm_hw_params_alloca(&setup);
	err = snd_pcm_hw_params_any(phandle, setup);
	if(err < 0)
	{
		_alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
			 "set_write_alsa: Could not query parameters: %s",snd_strerror(err));

		return AL_FALSE;
	}

        /* set the interleaved read format */
        err = snd_pcm_hw_params_set_access(phandle, setup, SND_PCM_ACCESS_RW_INTERLEAVED);
        if (err < 0) {
        	_alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
			 "set_write_alsa: Could not set access type: %s",snd_strerror(err));
                return AL_FALSE;
        }

	/* set format */
	err = snd_pcm_hw_params_set_format(phandle, setup, ai->format);
	if(err < 0)
	{
		_alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
			 "set_write_alsa: could not set format: %s",snd_strerror(err));

		return AL_FALSE;
	}


	/* channels */
	err = snd_pcm_hw_params_set_channels(phandle, setup, ai->channels);
	if(err < 0)
	{
		err = snd_pcm_hw_params_get_channels(setup);
		if(err!= (int) (ai->channels)) {
			_alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
				 "set_write_alsa: could not set channels: %s",snd_strerror(err));

			return AL_FALSE;
		}
	}


	/* sampling rate */
	err = snd_pcm_hw_params_set_rate_near(phandle, setup, ai->speed, NULL);
	if(err < 0)
	{
		_alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
			 "set_write_alsa: could not set speed: %s",snd_strerror(err));

		return AL_FALSE;
	}
	/* err is sampling rate if >= 0 */
	ai->speed = (unsigned int) err;


        /* Set number of periods. Periods used to be called fragments. */
        err = snd_pcm_hw_params_set_periods(phandle, setup, ai->periods, 0);
        if (err < 0) {
                _alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
			"set_write_alsa: %s\n", snd_strerror(err));
		return AL_FALSE;
        }

        err = snd_pcm_hw_params_set_buffer_size(phandle, setup, ai->bufframesize);
        if (err < 0) {
                _alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
			"set_write_alsa: %s\n", snd_strerror(err));
		return AL_FALSE;
        }

        _alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
         "set_write_alsa (info): Buffersize = %i (%i)",snd_pcm_hw_params_get_buffer_size(setup), *bufsiz);

        _alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
         "set_write_alsa (info): Periodsize = %i",snd_pcm_hw_params_get_period_size(setup, &dir));

	err = snd_pcm_hw_params(phandle, setup);
	if(err < 0)
	{
		_alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
			"set_alsa: %s\n", snd_strerror(err));
		return AL_FALSE;
	}

	err = snd_pcm_prepare(phandle);
	if(err < 0)
	{
		_alDebug(ALD_MAXIMUS,  __FILE__, __LINE__,
			"set_alsa %s\n", snd_strerror(err));
		return AL_FALSE;
	}

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
        snd_pcm_uframes_t frames;

	if((ai == NULL) || (ai->handle == NULL))
	{
		return;
	}

	phandle = ai->handle;
        channels= ai->channels;
        frames  = (snd_pcm_uframes_t) bytes / ai->framesize;

	while(data_len > 0)
	{
		err = snd_pcm_writei(phandle, pdata, frames);
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
                                pdata += err * ai->framesize;
				data_len -= err* ai->framesize;
				break;
		}
		if(err < 0)
		{
                        err = snd_pcm_prepare(phandle);
			if(err < 0)
			{
				const char *serr = snd_strerror(err);

				_alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
                                 "alsa_blitbuffer: %s\n", serr);

				return;
			}
		}
	}

	return;
}

static int AL2ALSAFMT(ALenum format) {
	switch(format) {
		case AL_FORMAT_STEREO8:     return SND_PCM_FORMAT_U8;
		case AL_FORMAT_MONO8:       return SND_PCM_FORMAT_U8;
		case AL_FORMAT_QUAD8_LOKI:  return SND_PCM_FORMAT_U8;
		case AL_FORMAT_STEREO16:    return SND_PCM_FORMAT_S16;
		case AL_FORMAT_MONO16:      return SND_PCM_FORMAT_S16;
		case AL_FORMAT_QUAD16_LOKI: return SND_PCM_FORMAT_S16;

		default: break;
	}
	return -1;
}



static int FRAMESIZE(snd_pcm_format_t fmt, unsigned int chans) {
	int retval = 0;
	switch(fmt) {
		case SND_PCM_FORMAT_U8:
			retval = 1;
			break;
		case SND_PCM_FORMAT_S16:
			retval = 2;
			break;
		default:
	                return -1;
		        break;
		}

	return(retval*chans);
}

