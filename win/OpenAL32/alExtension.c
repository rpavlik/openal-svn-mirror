/**
 * OpenAL cross platform audio library
 * Copyright (C) 1999-2000 by authors.
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA  02111-1307, USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */

#include <stdlib.h>
#include <string.h>
#include "Include/alExtension.h"
#include "Include/alEax.h"

static ALextension extension[]=  {	
	{ "EAX",					  (ALvoid *) NULL				},
	{ "EAX2.0",					  (ALvoid *) NULL				},
	{ NULL,						  (ALvoid *) NULL				} };

static ALfunction  function[]=   {	
	{ "EAXGet",						(ALvoid *) EAXGet					},
	{ "EAXSet",						(ALvoid *) EAXSet					},
	{ NULL,							(ALvoid *) NULL						} };

static ALenums	   enumeration[]={
	// Types
	{ "AL_INVALID",						AL_INVALID							},
	{ "ALC_INVALID",					ALC_INVALID							},
	{ "AL_NONE",						AL_NONE								},
	{ "AL_FALSE",						AL_FALSE							},
	{ "ALC_FALSE",						ALC_FALSE							},
	{ "AL_TRUE",						AL_TRUE								},
	{ "ALC_TRUE",						ALC_TRUE							},
	
	// Source and Listener Properties
	{ "AL_SOURCE_RELATIVE",				AL_SOURCE_RELATIVE					},
	{ "AL_CONE_INNER_ANGLE",			AL_CONE_INNER_ANGLE					},
	{ "AL_CONE_OUTER_ANGLE",			AL_CONE_OUTER_ANGLE					},
	{ "AL_PITCH",						AL_PITCH							},
	{ "AL_POSITION",					AL_POSITION							},
	{ "AL_DIRECTION",					AL_DIRECTION						},
	{ "AL_VELOCITY",					AL_VELOCITY							},
	{ "AL_LOOPING",						AL_LOOPING							},
	{ "AL_BUFFER",						AL_BUFFER							},
	{ "AL_GAIN",						AL_GAIN								},
	{ "AL_MIN_GAIN",					AL_MIN_GAIN							},
	{ "AL_MAX_GAIN",					AL_MAX_GAIN							},
	{ "AL_ORIENTATION",					AL_ORIENTATION						},
	{ "AL_REFERENCE_DISTANCE",			AL_REFERENCE_DISTANCE				},
	{ "AL_ROLLOFF_FACTOR",				AL_ROLLOFF_FACTOR					},
	{ "AL_CONE_OUTER_GAIN",				AL_CONE_OUTER_GAIN					},
	{ "AL_MAX_DISTANCE",				AL_MAX_DISTANCE						},

	// Source State information
	{ "AL_SOURCE_STATE",				AL_SOURCE_STATE						},
	{ "AL_INITIAL",						AL_INITIAL							},
	{ "AL_PLAYING",						AL_PLAYING							},
	{ "AL_PAUSED",						AL_PAUSED							},
	{ "AL_STOPPED",						AL_STOPPED							},

	// Queue information
	{ "AL_BUFFERS_QUEUED",				AL_BUFFERS_QUEUED					},
	{ "AL_BUFFERS_PROCESSED",			AL_BUFFERS_PROCESSED				},
	
	// Buffer Formats
	{ "AL_FORMAT_MONO8",				AL_FORMAT_MONO8						},
	{ "AL_FORMAT_MONO16",				AL_FORMAT_MONO16					},
	{ "AL_FORMAT_STEREO8",				AL_FORMAT_STEREO8					},
	{ "AL_FORMAT_STEREO16",				AL_FORMAT_STEREO16					},

	// Buffer attributes
	{ "AL_FREQUENCY",					AL_FREQUENCY						},
	{ "AL_BITS",						AL_BITS								},
	{ "AL_CHANNELS",					AL_CHANNELS							},
	{ "AL_SIZE",						AL_SIZE								},
	{ "AL_DATA",						AL_DATA								},
	
	// Buffer States (not supported yet)
	{ "AL_UNUSED",						AL_UNUSED							},
	{ "AL_PENDING",						AL_PENDING							},
	{ "AL_PROCESSED",					AL_PROCESSED						},

	// ALC Properties
	{ "ALC_MAJOR_VERSION",				ALC_MAJOR_VERSION					},
	{ "ALC_MINOR_VERSION",				ALC_MINOR_VERSION					},
	{ "ALC_ATTRIBUTES_SIZE",			ALC_ATTRIBUTES_SIZE					},
	{ "ALC_ALL_ATTRIBUTES",				ALC_ALL_ATTRIBUTES					},
	{ "ALC_DEFAULT_DEVICE_SPECIFIER",	ALC_DEFAULT_DEVICE_SPECIFIER		},
	{ "ALC_DEVICE_SPECIFIER",			ALC_DEVICE_SPECIFIER				},
	{ "ALC_EXTENSIONS",					ALC_EXTENSIONS						},
	{ "ALC_FREQUENCY",					ALC_FREQUENCY						},
	{ "ALC_REFRESH",					ALC_REFRESH							},
	{ "ALC_SYNC",						ALC_SYNC							},

	// AL Error Messages
	{ "AL_NO_ERROR",					AL_NO_ERROR							},
	{ "AL_INVALID_NAME",				AL_INVALID_NAME						},
	{ "AL_INVALID_ENUM",				AL_INVALID_ENUM						},
	{ "AL_INVALID_VALUE",				AL_INVALID_VALUE					},
	{ "AL_INVALID_OPERATION",			AL_INVALID_OPERATION				},
	{ "AL_OUT_OF_MEMORY",				AL_OUT_OF_MEMORY					},
	
	// ALC Error Message
	{ "ALC_NO_ERROR",					ALC_NO_ERROR						},
	{ "ALC_INVALID_DEVICE",				ALC_INVALID_DEVICE					},
	{ "ALC_INVALID_CONTEXT",			ALC_INVALID_CONTEXT					},
	{ "ALC_INVALID_ENUM",				ALC_INVALID_ENUM					},
	{ "ALC_INVALID_VALUE",				ALC_INVALID_VALUE					},
	{ "ALC_OUT_OF_MEMORY",				ALC_OUT_OF_MEMORY					},

	// Context strings
	{ "AL_VENDOR",						AL_VENDOR							},
	{ "AL_VERSION",						AL_VERSION							},
	{ "AL_RENDERER",					AL_RENDERER							},
	{ "AL_EXTENSIONS",					AL_EXTENSIONS						},
	
	// Global states
	{ "AL_DOPPLER_FACTOR",				AL_DOPPLER_FACTOR					},
	{ "AL_DOPPLER_VELOCITY",			AL_DOPPLER_VELOCITY					},
	{ "AL_DISTANCE_MODEL",				AL_DISTANCE_MODEL					},
	
	// Distance Models
	{ "AL_INVERSE_DISTANCE",			AL_INVERSE_DISTANCE					},
	{ "AL_INVERSE_DISTANCE_CLAMPED",	AL_INVERSE_DISTANCE_CLAMPED			},

	// Default
	{ NULL,								(ALenum  ) 0 						} };


ALAPI ALboolean ALAPIENTRY alIsExtensionPresent(ALubyte *extName)
{
	ALboolean bSupported = AL_FALSE;
	ALsizei i=0;

	// Check for EAX Support
	if (!strcmp(extName, "EAX") || !strcmp(extName, "EAX2.0"))
	{
		bSupported = CheckEAXSupport(extName);
	}

	return bSupported;
}


ALAPI ALvoid * ALAPIENTRY alGetProcAddress(ALubyte *funcName)
{
	ALsizei i=0;
	ALvoid *pAddress;

	while ((function[i].funcName)&&(strcmp((char *)function[i].funcName,(char *)funcName)))
		i++;
	pAddress = function[i].address;

	return pAddress;
}


ALAPI ALenum ALAPIENTRY alGetEnumValue(ALubyte *enumName)
{
	ALsizei i=0;
	ALenum	val;

	while ((enumeration[i].enumName)&&(strcmp((char *)enumeration[i].enumName,(char *)enumName)))
		i++;
	val = enumeration[i].value;

	return val;
}
