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

    /* If the name starts with a ', then it's a device configuration */
    if(deviceSpecifier && deviceSpecifier[0] == '\'') {
        Rcvar listOfLists;

        /* see if the user defined devices, sampling-rate, or direction */
        devices   = rc_lookup( "devices" );
        direction = rc_lookup( "direction" );
        freq_sym  = rc_lookup( "sampling-rate" );
        speakers  = rc_lookup( "speaker-num" );

        listOfLists = rc_eval( deviceSpecifier );
        rc_foreach( listOfLists, rc_define_list );
        /* Got a config, so remove the name from further processing */
        deviceSpecifier = NULL;

        /* redefine the stuff we saved */
        if( direction != NULL )
            rc_define( "direction", alrc_quote( direction ));

        if( devices != NULL )
            rc_define( "devices", alrc_quote( devices ) );

        if( freq_sym != NULL )
            rc_define( "sampling-rate", alrc_quote( freq_sym ));

        if( speakers != NULL )
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

	/* defaults */
	retval->format = _ALC_EXTERNAL_FMT;
	retval->speed  = _ALC_EXTERNAL_SPEED;
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
	alcBackendOpen_(deviceSpecifier, (openForInput ? ALC_OPEN_INPUT_ : ALC_OPEN_OUTPUT_), &retval->ops, &retval->privateData );
	if( retval->privateData == NULL ) {
		free( retval );
		return NULL;
	}
        /* set specifier */
        retval->specifier = retval->ops->getName(retval->privateData);
	retval->flags |= ( openForInput ? ALCD_READ : ALCD_WRITE );

	num_devices++;

	/* Figure out the size of buffer to request from the device based
	 * on the bytes per sample and speed */
	retval->bufferSizeInBytes  = _alSmallestPowerOfTwo(retval->speed / 15);
	retval->bufferSizeInBytes *= _alGetChannelsFromFormat(retval->format);
	retval->bufferSizeInBytes *= _alGetBitsFromFormat(retval->format) / 8;
	if(_alcDeviceSet(retval) == ALC_FALSE) {
		alcCloseDevice(retval);
		return NULL;
	}

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
 * until _alcDeviceSet is called, none of the changes are important.
 *
 * Returns ALC_FALSE if the setting operation failed.  After a call to this
 * function, the caller should check the members in dev to see what the
 * actual values set are.
 */
ALCboolean _alcDeviceSet(AL_device *dev)
{
    _alDebug(ALD_CONTEXT, __FILE__, __LINE__,
             "requesting device buffer size %d, format 0x%x, speed %d",
             dev->bufferSizeInBytes, dev->format, dev->speed);

    if(!dev->ops->setAttributes(dev->privateData, &dev->bufferSizeInBytes,
                                &dev->format, &dev->speed)) {
        _alDebug(ALD_CONVERT, __FILE__, __LINE__, "alcDeviceSet_ failed");
        return ALC_FALSE;
    }

    _alDebug(ALD_CONTEXT, __FILE__, __LINE__,
             "got device buffer size %d, format 0x%x, speed %d",
             dev->bufferSizeInBytes, dev->format, dev->speed);
    return ALC_TRUE;
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
