/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * interface_sound.c
 *
 * This file defines the high-level interface to the sound device.
 * It actually dispatches requests based on the architecture that
 * we're targetting.  See platform.h for more of a clue.
 */
#include "al_siteconfig.h"

#include <AL/al.h>
#include <stdlib.h>
#include <string.h>

#include "al_config.h"
#include "al_debug.h"
#include "al_main.h"
#include "arch/interface/interface_sound.h"

typedef enum {
	LA_NONE,
	LA_NATIVE,  /* native audio for platform */
	LA_ALSA,    /* ALSA backend */
	LA_ARTS,    /* aRts backend */
	LA_ESD,     /* ESD backend */
	LA_SDL,     /* SDL backend */
	LA_NULL,    /* null backend */
	LA_WAVEOUT  /* WAVE backend */
} lin_audio;

/* represents which backend we are using */
static lin_audio hardware_type = LA_NONE;

void *
_alcBackendOpenOutput(void)
{
	Rcvar device_params;
	Rcvar device_list;
	Rcvar device;
	void *retval = NULL;
	char adevname[64]; /* FIXME: magic number */

	device_list = rc_lookup("devices");
	while(device_list != NULL) {
		device      = rc_car( device_list );
		device_list = rc_cdr( device_list );

		switch(rc_type(device)) {
		case ALRC_STRING:
			rc_tostr0(device, adevname, 64);
			break;
		case ALRC_SYMBOL:
			rc_symtostr0(device, adevname, 64);
			break;
		case ALRC_CONSCELL:
			device_params = rc_cdr( device );
			if(device_params == NULL) {
				continue;
			}
			rc_define("device-params", device_params);
			rc_symtostr0(rc_car(device), adevname, 64);
			break;
		default:
			_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
				  "_alcBackendOpenOutput: bad type %s for device",
				  rc_typestr( rc_type( device ) ));
			continue;
		}

		if(strcmp(adevname, "dsp") == 0) {
			_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
				  "_alcBackendOpenOutput: 'dsp' is a deprecated device name. Use 'native' instead.");
			retval = grab_write_native();
			if(retval != NULL) {
				hardware_type = LA_NATIVE;
				return retval;
			}
		}
		if(strcmp(adevname, "native") == 0) {
			retval = grab_write_native();
			if(retval != NULL) {
				hardware_type = LA_NATIVE;
				return retval;
			}
		}

		if(strcmp(adevname, "alsa") == 0) {
			retval = grab_write_alsa();
			if(retval != NULL) {
				hardware_type = LA_ALSA;
				return retval;
			}
		}

		if(strcmp(adevname, "arts") == 0) {
			retval = grab_write_arts();
			if(retval != NULL) {
				hardware_type = LA_ARTS;
				return retval;
			}
		}

		if(strcmp(adevname, "esd") == 0) {
			retval = grab_write_esd();
			if(retval != NULL) {
				hardware_type = LA_ESD;
				return retval;
			}
		}

		if(strcmp(adevname, "sdl") == 0) {
			retval = grab_write_sdl();
			if(retval != NULL) {
				hardware_type = LA_SDL;
				return retval;
			}
		}

		if(strcmp(adevname, "null") == 0) {
			retval = grab_write_null();
			if(retval != NULL) {
				hardware_type = LA_NULL;
				return retval;
			}
		}

		if(strcmp(adevname, "waveout") == 0) {
			retval = grab_write_waveout();
			if(retval != NULL) {
				hardware_type = LA_WAVEOUT;
				return retval;
			}
		}
	}

	/* no device list specified, try native or fail */
	retval = grab_write_native();
	if(retval != NULL) {
		hardware_type = LA_NATIVE;
		return retval;
	}

	return NULL;
}

void *
_alcBackendOpenInput(void)
{
	Rcvar device_params;
	Rcvar device_list;
	Rcvar device;
	void *retval = NULL;
	char adevname[64]; /* FIXME: magic number */

	device_list = rc_lookup("devices");
	while(device_list != NULL) {
		device      = rc_car( device_list );
		device_list = rc_cdr( device_list );

		switch(rc_type(device)) {
		case ALRC_STRING:
			rc_tostr0(device, adevname, 64);
			break;
		case ALRC_SYMBOL:
			rc_symtostr0(device, adevname, 64);
			break;
		case ALRC_CONSCELL:
			device_params = rc_cdr( device );
			if(device_params == NULL) {
				continue;
			}
			rc_define("device-params", device_params);
			rc_symtostr0(rc_car(device), adevname, 64);
			break;
		default:
			_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
				  "_alcBackendOpenInput: bad type %s for device",
				  rc_typestr( rc_type( device ) ));
			continue;
		}

		if(strcmp(adevname, "dsp") == 0) {
			_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
				  "_alcBackendOpenInput: 'dsp' is a deprecated device name. Use 'native' instead.");
			retval = grab_read_native();
			if(retval != NULL) {
				hardware_type = LA_NATIVE;
				return retval;
			}
		}
		if(strcmp(adevname, "native") == 0) {
			retval = grab_read_native();
			if(retval != NULL) {
				hardware_type = LA_NATIVE;
				return retval;
			}
		}

		if(strcmp(adevname, "alsa") == 0) {
			retval = grab_read_alsa();
			if(retval != NULL) {
				hardware_type = LA_ALSA;
				return retval;
			}
		}

		if(strcmp(adevname, "arts") == 0) {
			retval = grab_read_arts();
			if(retval != NULL) {
				hardware_type = LA_ARTS;
				return retval;
			}
		}

		if(strcmp(adevname, "esd") == 0) {
			retval = grab_read_esd();
			if(retval != NULL) {
				hardware_type = LA_ESD;
				return retval;
			}
		}

		if(strcmp(adevname, "sdl") == 0) {
			retval = grab_read_sdl();
			if(retval != NULL) {
				hardware_type = LA_SDL;
				return retval;
			}
		}

		if(strcmp(adevname, "null") == 0) {
			retval = grab_read_null();
			if(retval != NULL) {
				hardware_type = LA_NULL;
				return retval;
			}
		}

		if(strcmp(adevname, "waveout") == 0) {
			retval = grab_read_waveout();
			if(retval != NULL) {
				hardware_type = LA_WAVEOUT;
				return retval;
			}
		}
	}

	/* no device list specified, try native or fail */
	retval = grab_read_native();
	if(retval != NULL) {
		hardware_type = LA_NATIVE;
		return retval;
	}

	return NULL;
}

ALboolean
_alcBackendClose(void *handle)
{
	switch(hardware_type) {
	case LA_NATIVE:
		release_native(handle);
		return AL_TRUE;
	case LA_ALSA:
		release_alsa(handle);
		return AL_TRUE;
	case LA_ARTS:
		release_arts(handle);
		return AL_TRUE;
	case LA_ESD:
		release_esd(handle);
		return AL_TRUE;
	case LA_SDL:
		release_sdl(handle);
		return AL_TRUE;
	case LA_NULL:
		release_null(handle);
		return AL_TRUE;
	case LA_WAVEOUT:
		release_waveout(handle);
		return AL_TRUE;
	default:
		_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
			  "_alcBackendClose: unknown backend %d\n", hardware_type );
		return AL_FALSE;
	}

}

void
_alcBackendPause(void *handle)
{
	switch(hardware_type) {
	case LA_NATIVE:
		pause_nativedevice(handle);
		break;
	case LA_ALSA:
		pause_alsa(handle);
		break;
	case LA_ARTS:
		pause_arts(handle);
		break;
	case LA_ESD:
		pause_esd(handle);
		break;
	case LA_SDL:
		pause_sdl(handle);
		break;
	case LA_NULL:
		pause_null(handle);
		break;
	case LA_WAVEOUT:
		pause_waveout(handle);
		break;
	default:
		_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
			  "_alcBackendPause: unknown backend %d\n", hardware_type );
		break;
	}
}

void
_alcBackendResume(void *handle)
{
	switch(hardware_type) {
	case LA_NATIVE:
		resume_nativedevice(handle);
		break;
	case LA_ALSA:
		resume_alsa(handle);
		break;
	case LA_ARTS:
		resume_arts(handle);
		break;
	case LA_ESD:
		resume_esd(handle);
		break;
	case LA_SDL:
		resume_sdl(handle);
		break;
	case LA_NULL:
		resume_null(handle);
		break;
	case LA_WAVEOUT:
		resume_waveout(handle);
		break;
	default:
		_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
			  "_alcBackendResume: unknown backend %d\n", hardware_type );
		break;
	}
}

ALboolean
_alcBackendSetWrite(void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed)
{
	switch(hardware_type) {
	case LA_NATIVE:
		return set_write_native(handle, bufsiz, fmt, speed);
	case LA_ALSA:
		return set_write_alsa(handle, bufsiz, fmt, speed);
	case LA_ARTS:
		return set_write_arts(handle, bufsiz, fmt, speed);
	case LA_ESD:
		return set_write_esd(handle, bufsiz, fmt, speed);
	case LA_SDL:
		return set_write_sdl(handle, bufsiz, fmt, speed);
	case LA_NULL:
		return set_write_null(handle, bufsiz, fmt, speed);
	case LA_WAVEOUT:
		return set_write_waveout(handle, bufsiz, fmt, speed);
	default:
		_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
			  "_alcBackendSetWrite: unknown backend %d\n", hardware_type );
		return AL_FALSE;
	}
}

ALboolean
_alcBackendSetRead(void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed)
{
	switch(hardware_type) {
	case LA_NATIVE:
		return set_read_native(handle, bufsiz, fmt, speed);
	case LA_ALSA:
		return set_read_alsa(handle, bufsiz, fmt, speed);
	case LA_ARTS:
		return set_read_arts(handle, bufsiz, fmt, speed);
	case LA_ESD:
		return set_read_esd(handle, bufsiz, fmt, speed);
	case LA_SDL:
		return set_read_sdl(handle, bufsiz, fmt, speed);
	case LA_NULL:
		return set_read_null(handle, bufsiz, fmt, speed);
	case LA_WAVEOUT:
		return set_read_waveout(handle, bufsiz, fmt, speed);
	default:
		_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
			  "_alcBackendSetRead: unknown backend %d\n", hardware_type );
		return AL_FALSE;
	}
}

void
_alcBackendWrite(void *handle, void *dataptr, int bytes_to_write)
{
	switch(hardware_type) {
	case LA_NATIVE:
		native_blitbuffer(handle, dataptr, bytes_to_write);
		break;
	case LA_ALSA:
		alsa_blitbuffer(handle, dataptr, bytes_to_write);
		break;
	case LA_ARTS:
		arts_blitbuffer(handle, dataptr, bytes_to_write);
		break;
	case LA_ESD:
		esd_blitbuffer(handle, dataptr, bytes_to_write);
		break;
	case LA_SDL:
		sdl_blitbuffer(handle, dataptr, bytes_to_write);
		break;
	case LA_NULL:
		null_blitbuffer(handle, dataptr, bytes_to_write);
		break;
	case LA_WAVEOUT:
		waveout_blitbuffer(handle, dataptr, bytes_to_write);
		break;
	default:
		_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
			  "_alcBackendWrite: unknown backend %d\n", hardware_type );
		break;
	}
}


ALsizei
_alcBackendRead(void *handle, void *capture_buffer, int bufsiz)
{
	switch(hardware_type) {
	case LA_NATIVE:
		return capture_nativedevice(handle, capture_buffer, bufsiz);
	case LA_ALSA:
		return capture_alsa(handle, capture_buffer, bufsiz);
	case LA_ARTS:
		return capture_arts(handle, capture_buffer, bufsiz);
	case LA_ESD:
		return capture_esd(handle, capture_buffer, bufsiz);
	case LA_SDL:
		return capture_sdl(handle, capture_buffer, bufsiz);
	case LA_NULL:
		return capture_null(handle, capture_buffer, bufsiz);
	case LA_WAVEOUT:
		return capture_waveout(handle, capture_buffer, bufsiz);
	default:
		_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
			  "_alcBackendRead: unknown backend %d\n", hardware_type );
		return 0;
	}
}

ALfloat
_alcBackendGetAudioChannel(void *handle, ALuint channel)
{
	switch(hardware_type) {
	case LA_NATIVE:
		return get_nativechannel(handle, channel);
	case LA_ALSA:
		return get_alsachannel(handle, channel);
	case LA_ARTS:
		return get_artschannel(handle, channel);
	case LA_ESD:
		return get_esdchannel(handle, channel);
	case LA_SDL:
		return get_sdlchannel(handle, channel);
	case LA_NULL:
		return get_nullchannel(handle, channel);
	case LA_WAVEOUT:
		return get_waveoutchannel(handle, channel);
	default:
		_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
			  "_alcBackendGetAudioChannel: unknown backend %d\n", hardware_type );
		return 0;
	}
}

void
_alcBackendSetAudioChannel(void *handle, ALuint channel, ALfloat volume)
{
	switch(hardware_type) {
	case LA_NATIVE:
		set_nativechannel(handle, channel, volume);
		break;
	case LA_ALSA:
		set_alsachannel(handle, channel, volume);
		break;
	case LA_ARTS:
		set_artschannel(handle, channel, volume);
		break;
	case LA_ESD:
		set_esdchannel(handle, channel, volume);
		break;
	case LA_SDL:
		set_sdlchannel(handle, channel, volume);
		break;
	case LA_NULL:
		set_nullchannel(handle, channel, volume);
		break;
	case LA_WAVEOUT:
		set_waveoutchannel(handle, channel, volume);
		break;
	default:
		_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
			  "_alcBackendSetAudioChannel: unknown backend %d\n", hardware_type );
		break;
	}
}
