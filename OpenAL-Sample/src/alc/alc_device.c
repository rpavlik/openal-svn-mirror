/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * alc_device.c
 *
 */
#include "al_siteconfig.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <stdlib.h>
#include <string.h>

#include "config/al_config.h"
#include "al_main.h"
#include "al_mixer.h"
#include "al_debug.h"

#include "alc/alc_device.h"
#include "alc/alc_error.h"

#include "backends/alc_backend.h"

static int num_devices = 0;

static const struct {
    ALCchar *description;
    ALCchar *drvname;
} DeviceList[] = {
#define DEFAULT_NAME "Default\0"
    { DEFAULT_NAME, "" },

#ifdef USE_BACKEND_ALSA
#define ALSA_NAME "Advanced Linux Sound Architecture (ALSA)\0"
    { ALSA_NAME, "alsa" },
#else
#define ALSA_NAME
#endif

#ifdef USE_BACKEND_OSS
#define OSS_NAME "Open Sound System (OSS)\0"
    { OSS_NAME, "oss" },
#else
#define OSS_NAME
#endif

#ifdef USE_BACKEND_DMEDIA
#define DMEDIA_NAME "DMedia\0"
    { DMEDIA_NAME, "dmedia" },
#else
#define DMEDIA_NAME
#endif

#ifdef USE_BACKEND_NATIVE_DARWIN
#define DARWIN_NAME "Darwin Native\0"
    { DARWIN_NAME, "darwin" },
#else
#define DARWIN_NAME
#endif

#ifdef USE_BACKEND_NATIVE_SOLARIS
#define SOLARIS_NAME "Solaris Native\0"
    { SOLARIS_NAME, "solaris" },
#else
#define SOLARIS_NAME
#endif

#ifdef USE_BACKEND_NATIVE_WINDOWS
#define WINDOWS_NAME "Windows Native\0"
    { WINDOWS_NAME, "windows" },
#else
#define WINDOWS_NAME
#endif

#ifdef USE_BACKEND_ESD
#define ESD_NAME "Enlightened Sound Daemon (ESD)\0"
    { ESD_NAME, "esd" },
#else
#define ESD_NAME
#endif

#ifdef USE_BACKEND_ARTS
#define ARTS_NAME "Analog Realtime Synthesizer (aRts)\0"
    { ARTS_NAME, "arts" },
#else
#define ARTS_NAME
#endif

#ifdef USE_BACKEND_SDL
#define SDL_NAME "Simple DirectMedia Layer (SDL)\0"
    { SDL_NAME, "sdl" },
#else
#define SDL_NAME
#endif

#ifdef USE_BACKEND_WAVEOUT
#define WAVE_NAME "Wave Writer\0"
    { WAVE_NAME, "wave" },
#else
#define WAVE_NAME
#endif

#ifdef USE_BACKEND_NULL
#define NULL_NAME "Null Output\0"
    { NULL_NAME, "null" },
#else
#define NULL_NAME
#endif

    { NULL, NULL }
};

const ALCchar *_alcDeviceNames =
    DEFAULT_NAME
    ALSA_NAME
    OSS_NAME
    DMEDIA_NAME
    DARWIN_NAME
    SOLARIS_NAME
    WINDOWS_NAME
    ESD_NAME
    ARTS_NAME
    SDL_NAME
    WAVE_NAME
    NULL_NAME
"\0";


static __inline void eval_config(const ALCchar *cfg)
{
	Rcvar listOfLists = rc_eval( cfg );
	/* define each attribute pair */
	rc_foreach( listOfLists, rc_define_list );
}

static int set_config_from_name(const ALCchar *name)
{
	int i;

	/* First name in the list is the default, which behaves like NULL and
	 * uses no special settings */
	if(name == NULL || strcmp(name, DeviceList[0].description) == 0)
		return 0;

	/* If the name starts with a ', then it's a device configuration */
	if(name[0] == '\'')
	{
		eval_config(name);
		return 0;
	}

	for(i = 1;DeviceList[i].description != NULL;i++)
	{
		if(strcmp(name, DeviceList[i].description) == 0)
		{
			ALCchar *str = malloc(strlen("'((devices '(") +
			                      strlen(DeviceList[i].drvname) +
			                      strlen("))") + 1);
			if(!str)
				return 1;

			strcpy(str, "'((devices '(");
			strcat(str, DeviceList[i].drvname);
			strcat(str, "))");

			eval_config(str);

			free(str);
			return 0;
		}
	}

	return 1;
}

/*
 * Opens a device, using the alrc expression deviceSpecifier to specify
 * attributes for the device.  Returns the device if successful, NULL
 * otherwise.
 */
ALCdevice *alcOpenDevice( const ALchar *deviceSpecifier ) {
	ALCdevice *retval;
	char dirstr[65];
	Rcvar direction = NULL;
	Rcvar freq_sym = NULL;
	Rcvar speakers = NULL;
	Rcvar devices = NULL;
	int openForInput;

	if( num_devices == 0 ) {
		/* first initialization */
		if( _alParseConfig() == AL_FALSE ) {
			_alDebug(ALD_CONFIG, __FILE__, __LINE__,
				"Couldn't parse config file.");
		}
		if( _alInit() == AL_FALSE ) {
			_alDebug(ALD_CONFIG, __FILE__, __LINE__,
				"Couldn't initialize OpenAL.");
		}
	}

	/* see if the user defined devices, sampling-rate, or direction */
	devices   = rc_lookup( "devices" );
	direction = rc_lookup( "direction" );
	freq_sym  = rc_lookup( "sampling-rate" );
	speakers  = rc_lookup( "speaker-num" );

	if(set_config_from_name(deviceSpecifier))
		return NULL;

	/* redefine the stuff we saved */
	if( direction != NULL ) {
		rc_define( "direction", alrc_quote( direction ));
	}

	if( devices != NULL ) {
		rc_define( "devices", alrc_quote( devices ) );
	}

	if( freq_sym != NULL ) {
		rc_define( "sampling-rate", alrc_quote( freq_sym ));
	}

	if( speakers != NULL ) {
		rc_define( "speaker-num", alrc_quote( speakers ));
	}

	direction = rc_lookup( "direction" );
	devices   = rc_lookup( "devices" );
	freq_sym  = rc_lookup( "sampling-rate" );
	speakers  = rc_lookup( "speaker-num" );

	memset( dirstr, 0, sizeof(dirstr) );
	
	if( direction != NULL ) {
		switch( rc_type( direction ) ) {
			case ALRC_STRING:
				rc_tostr0(direction, dirstr, 64);
				break;
			case ALRC_SYMBOL:
				rc_symtostr0(direction, dirstr, 64);
				break;
			default:
				break;
		}
	}

	retval = malloc( sizeof *retval );
	if( retval == NULL) {
		/* FIXME: set AL_OUT_OF_MEMORY here? */

		return NULL;
	}

	/* copy specifier */
	if(deviceSpecifier)
	{
		size_t len;
		len = strlen((const char *) deviceSpecifier);
		retval->specifier = malloc(len+1);
		if(retval->specifier == NULL)
		{
			free(retval);
			return NULL;
		}

		memcpy(retval->specifier, deviceSpecifier, len);
		retval->specifier[len] = '\0';
	}
	else
	{
		/* JIV FIXME: maybe set to default string? */
		retval->specifier = malloc(1);
		retval->specifier[0] = '\0';
	}

	/* defaults */
	retval->format = _ALC_EXTERNAL_FMT;
	retval->speed  = _ALC_EXTERNAL_SPEED;
	retval->bufferSizeInBytes = ALC_DEFAULT_DEVICE_BUFFER_SIZE_IN_BYTES;
	retval->flags  = ALCD_NONE;

	if( freq_sym != NULL ) {
		switch(rc_type( freq_sym )) {
			case ALRC_INTEGER:
			case ALRC_FLOAT:
				retval->speed = rc_toint( freq_sym );
				break;
			default:
				_alDebug(ALD_CONVERT, __FILE__, __LINE__,
					"invalid type for sampling-rate");
				break;
		}
	}

	if( speakers != NULL ) {
		switch(rc_type( speakers )) {
			case ALRC_INTEGER:
			case ALRC_FLOAT:
				{
					ALint s = rc_toint( speakers );
					if (s >= 0) {
						ALenum fmt = _al_formatscale( retval->format, (ALuint)s );
						if ( fmt >= 0 ) {
							retval->format = fmt;
						}
					}
				}
				break;
			default:
				break;
		}
	}

	openForInput = (strncmp( dirstr, "read", 64 ) == 0);
	alcBackendOpen_( (openForInput ? ALC_OPEN_INPUT_ : ALC_OPEN_OUTPUT_), &retval->ops, &retval->privateData );
	if( retval->privateData == NULL ) {
		free( retval );
		_alcSetError(ALC_INVALID_DEVICE);
		return NULL;
	}
	retval->flags |= ( openForInput ? ALCD_READ : ALCD_WRITE );

	num_devices++;

	return retval;
}

/*
 * Closes the device referred to by dev.
 */
ALCboolean
alcCloseDevice( ALCdevice *dev )
{
	/* ToDo: Is this test really necessary? */
	if ( dev->ops != NULL) {
		_alLockMixerPause();
		/* ToDo: Use return value */
		dev->ops->close(dev->privateData);
		_alUnlockMixerPause();
	}

	free( dev->specifier );
	free( dev );

	num_devices--;

	if( num_devices == 0 ) {
		_alExit ();
	}

	return ALC_TRUE;
}

/*
 * Sets the attributes for the device from the settings in the device. The
 * library is free to change the parameters associated with the device, but
 * until alcDeviceSet_ is called, none of the changes are important.
 *
 * Sets ALC_INVALID_DEVICE if the setting operation failed.  After a call to
 * this function, the caller should check the members in dev is see what the
 * actual values set where.
 */
void
alcDeviceSet_ (AL_device * dev)
{
  _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
            "requesting device buffer size %d, format 0x%x, speed %d",
            dev->bufferSizeInBytes, dev->format, dev->speed);

  if (!dev->ops->setAttributes (dev->privateData, &dev->bufferSizeInBytes,
                                &dev->format, &dev->speed))
    {
      _alDebug (ALD_CONVERT, __FILE__, __LINE__, "alcDeviceSet_ failed");
      _alcSetError (ALC_INVALID_DEVICE);
    }
  _alDebug (ALD_CONTEXT, __FILE__, __LINE__,
            "got device buffer size %d, format 0x%x, speed %d",
            dev->bufferSizeInBytes, dev->format, dev->speed);
}

void
alcDeviceWrite_( AL_device *dev, ALvoid *dataptr, ALuint bytes_to_write )
{
	dev->ops->write( dev->privateData, dataptr, bytes_to_write );
}

ALsizei
alcDeviceRead_( AL_device *dev, ALvoid *dataptr, ALuint bytes_to_read )
{
	return dev->ops->read(dev->privateData, dataptr, bytes_to_read);
}

ALfloat
alcDeviceGetAudioChannel_( AL_device *dev, ALuint channel)
{
	return dev->ops->getAudioChannel( dev->privateData, channel);
}

void
alcDeviceSetAudioChannel_( AL_device *dev, ALuint channel, ALfloat volume)
{
	dev->ops->setAudioChannel(dev->privateData, channel, volume);
}
