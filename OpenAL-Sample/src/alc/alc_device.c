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
	Rcvar foo = NULL;
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
	}

	/* see if the user defined devices, sampling-rate, or direction */
	devices   = rc_lookup( "devices" );
	direction = rc_lookup( "direction" );
	freq_sym  = rc_lookup( "sampling-rate" );
	speakers  = rc_lookup( "speaker-num" );

	/* get the attributes requested in the args */
	if( deviceSpecifier ) {
		foo = rc_eval( (const char *) deviceSpecifier );
	}

	/* define each attribute pair */
	rc_foreach( foo, rc_define_list );

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
	retval->bufsiz = _ALC_DEF_BUFSIZ;
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
		/* ToDo: Use return value */
		dev->ops->close(dev->privateData);
	}

	free( dev->specifier );
	free( dev );

	num_devices--;

	return ALC_TRUE;
}

void
alcDevicePause_( AL_device *dev  )
{
	dev->ops->pause( dev->privateData );
}

void
alcDeviceResume_( AL_device *dev  )
{
	dev->ops->resume( dev->privateData );
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
alcDeviceSet_( AL_device *dev )
{
	if( dev->ops->setAttributes(dev->privateData, &dev->bufsiz, &dev->format, &dev->speed) != AL_TRUE ) {
		_alDebug(ALD_CONTEXT, __FILE__, __LINE__, "alcDeviceSet_ failed.");
		_alcSetError( ALC_INVALID_DEVICE );
	}
	_alDebug( ALD_CONVERT, __FILE__, __LINE__,
		  "after set_audiodevice, f|s|b 0x%x|%d|%d",
		  dev->format,
		  dev->speed,
		  dev->bufsiz );
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
