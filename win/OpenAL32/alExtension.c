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
#include "alExtension.h"
#include "alEax.h"
#include "alError.h"
#include "alMain.h"
#include "AL/al.h"

static ALfunction  function[]=   {	
	{ "EAXGet",						(ALvoid *) EAXGet					},
	{ "EAXSet",						(ALvoid *) EAXSet					},
	{ NULL,							(ALvoid *) NULL						} };

static ALenums enumeration[]={
	// Types
	{ (ALchar *)"AL_INVALID",                       AL_INVALID                          },
	{ (ALchar *)"ALC_INVALID",						ALC_INVALID							},
	{ (ALchar *)"AL_NONE",							AL_NONE								},
	{ (ALchar *)"AL_FALSE",							AL_FALSE							},
	{ (ALchar *)"ALC_FALSE",						ALC_FALSE							},
	{ (ALchar *)"AL_TRUE",							AL_TRUE								},
	{ (ALchar *)"ALC_TRUE",							ALC_TRUE							},
	
	// Source and Listener Properties
	{ (ALchar *)"AL_SOURCE_RELATIVE",				AL_SOURCE_RELATIVE					},
	{ (ALchar *)"AL_CONE_INNER_ANGLE",				AL_CONE_INNER_ANGLE					},
	{ (ALchar *)"AL_CONE_OUTER_ANGLE",				AL_CONE_OUTER_ANGLE					},
	{ (ALchar *)"AL_PITCH",							AL_PITCH							},
	{ (ALchar *)"AL_POSITION",						AL_POSITION							},
	{ (ALchar *)"AL_DIRECTION",						AL_DIRECTION						},
	{ (ALchar *)"AL_VELOCITY",						AL_VELOCITY							},
	{ (ALchar *)"AL_LOOPING",						AL_LOOPING							},
	{ (ALchar *)"AL_BUFFER",						AL_BUFFER							},
	{ (ALchar *)"AL_GAIN",							AL_GAIN								},
	{ (ALchar *)"AL_MIN_GAIN",						AL_MIN_GAIN							},
	{ (ALchar *)"AL_MAX_GAIN",						AL_MAX_GAIN							},
	{ (ALchar *)"AL_ORIENTATION",					AL_ORIENTATION						},
	{ (ALchar *)"AL_REFERENCE_DISTANCE",			AL_REFERENCE_DISTANCE				},
	{ (ALchar *)"AL_ROLLOFF_FACTOR",				AL_ROLLOFF_FACTOR					},
	{ (ALchar *)"AL_CONE_OUTER_GAIN",				AL_CONE_OUTER_GAIN					},
	{ (ALchar *)"AL_MAX_DISTANCE",					AL_MAX_DISTANCE						},
	{ (ALchar *)"AL_SEC_OFFSET",                    AL_SEC_OFFSET                       },
	{ (ALchar *)"AL_SAMPLE_OFFSET",                 AL_SAMPLE_OFFSET                    },
	{ (ALchar *)"AL_BYTE_OFFSET",                   AL_BYTE_OFFSET                      },
	{ (ALchar *)"AL_SOURCE_TYPE",                   AL_SOURCE_TYPE                      },
	{ (ALchar *)"AL_STATIC",                        AL_STATIC                           },
	{ (ALchar *)"AL_STREAMING",                     AL_STREAMING                        },
	{ (ALchar *)"AL_UNDETERMINED",                  AL_UNDETERMINED                     },

	// Source State information
	{ (ALchar *)"AL_SOURCE_STATE",					AL_SOURCE_STATE						},
	{ (ALchar *)"AL_INITIAL",						AL_INITIAL							},
	{ (ALchar *)"AL_PLAYING",						AL_PLAYING							},
	{ (ALchar *)"AL_PAUSED",						AL_PAUSED							},
	{ (ALchar *)"AL_STOPPED",						AL_STOPPED							},

	// Queue information
	{ (ALchar *)"AL_BUFFERS_QUEUED",				AL_BUFFERS_QUEUED					},
	{ (ALchar *)"AL_BUFFERS_PROCESSED",				AL_BUFFERS_PROCESSED				},
	
	// Buffer Formats
	{ (ALchar *)"AL_FORMAT_MONO8",					AL_FORMAT_MONO8						},
	{ (ALchar *)"AL_FORMAT_MONO16",					AL_FORMAT_MONO16					},
	{ (ALchar *)"AL_FORMAT_STEREO8",				AL_FORMAT_STEREO8					},
	{ (ALchar *)"AL_FORMAT_STEREO16",				AL_FORMAT_STEREO16					},

	// Buffer attributes
	{ (ALchar *)"AL_FREQUENCY",						AL_FREQUENCY						},
	{ (ALchar *)"AL_SIZE",							AL_SIZE								},

	// Buffer States (not supported yet)
	{ (ALchar *)"AL_UNUSED",						AL_UNUSED							},
	{ (ALchar *)"AL_PENDING",						AL_PENDING							},
	{ (ALchar *)"AL_PROCESSED",						AL_PROCESSED						},

	// ALC Properties
	{ (ALchar *)"ALC_MAJOR_VERSION",				ALC_MAJOR_VERSION					},
	{ (ALchar *)"ALC_MINOR_VERSION",				ALC_MINOR_VERSION					},
	{ (ALchar *)"ALC_ATTRIBUTES_SIZE",				ALC_ATTRIBUTES_SIZE					},
	{ (ALchar *)"ALC_ALL_ATTRIBUTES",				ALC_ALL_ATTRIBUTES					},
	{ (ALchar *)"ALC_DEFAULT_DEVICE_SPECIFIER",		ALC_DEFAULT_DEVICE_SPECIFIER		},
	{ (ALchar *)"ALC_DEVICE_SPECIFIER",				ALC_DEVICE_SPECIFIER				},
	{ (ALchar *)"ALC_EXTENSIONS",					ALC_EXTENSIONS						},
	{ (ALchar *)"ALC_FREQUENCY",					ALC_FREQUENCY						},
	{ (ALchar *)"ALC_REFRESH",						ALC_REFRESH							},
	{ (ALchar *)"ALC_SYNC",							ALC_SYNC							},
	{ (ALchar *)"ALC_MONO_SOURCES",                 ALC_MONO_SOURCES                    },
	{ (ALchar *)"ALC_STEREO_SOURCES",               ALC_STEREO_SOURCES                  },
	{ (ALchar *)"ALC_CAPTURE_DEVICE_SPECIFIER",     ALC_CAPTURE_DEVICE_SPECIFIER        },
	{ (ALchar *)"ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER", ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER },
    { (ALchar *)"ALC_CAPTURE_SAMPLES",              ALC_CAPTURE_SAMPLES                 },

	// AL Error Messages
	{ (ALchar *)"AL_NO_ERROR",						AL_NO_ERROR							},
	{ (ALchar *)"AL_INVALID_NAME",					AL_INVALID_NAME						},
	{ (ALchar *)"AL_INVALID_ENUM",					AL_INVALID_ENUM						},
	{ (ALchar *)"AL_INVALID_VALUE",					AL_INVALID_VALUE					},
	{ (ALchar *)"AL_INVALID_OPERATION",				AL_INVALID_OPERATION				},
	{ (ALchar *)"AL_OUT_OF_MEMORY",					AL_OUT_OF_MEMORY					},
	
	// ALC Error Message
	{ (ALchar *)"ALC_NO_ERROR",						ALC_NO_ERROR						},
	{ (ALchar *)"ALC_INVALID_DEVICE",				ALC_INVALID_DEVICE					},
	{ (ALchar *)"ALC_INVALID_CONTEXT",				ALC_INVALID_CONTEXT					},
	{ (ALchar *)"ALC_INVALID_ENUM",					ALC_INVALID_ENUM					},
	{ (ALchar *)"ALC_INVALID_VALUE",				ALC_INVALID_VALUE					},
	{ (ALchar *)"ALC_OUT_OF_MEMORY",				ALC_OUT_OF_MEMORY					},

	// Context strings
	{ (ALchar *)"AL_VENDOR",						AL_VENDOR							},
	{ (ALchar *)"AL_VERSION",						AL_VERSION							},
	{ (ALchar *)"AL_RENDERER",						AL_RENDERER							},
	{ (ALchar *)"AL_EXTENSIONS",					AL_EXTENSIONS						},
	
	// Global states
	{ (ALchar *)"AL_DOPPLER_FACTOR",				AL_DOPPLER_FACTOR					},
	{ (ALchar *)"AL_DOPPLER_VELOCITY",				AL_DOPPLER_VELOCITY					},
	{ (ALchar *)"AL_DISTANCE_MODEL",				AL_DISTANCE_MODEL					},
	{ (ALchar *)"AL_SPEED_OF_SOUND",                AL_SPEED_OF_SOUND                   },
	
	// Distance Models
	{ (ALchar *)"AL_INVERSE_DISTANCE",				AL_INVERSE_DISTANCE					},
	{ (ALchar *)"AL_INVERSE_DISTANCE_CLAMPED",		AL_INVERSE_DISTANCE_CLAMPED			},
	{ (ALchar *)"AL_LINEAR_DISTANCE",				AL_LINEAR_DISTANCE					},
	{ (ALchar *)"AL_LINEAR_DISTANCE_CLAMPED",		AL_LINEAR_DISTANCE_CLAMPED			},
	{ (ALchar *)"AL_EXPONENT_DISTANCE",				AL_EXPONENT_DISTANCE			    },
	{ (ALchar *)"AL_EXPONENT_DISTANCE_CLAMPED",		AL_EXPONENT_DISTANCE_CLAMPED	    },

	// Default
	{ (ALchar *)NULL,								(ALenum  ) 0 						} };



ALAPI ALboolean ALAPIENTRY alIsExtensionPresent(const ALchar *extName)
{
	ALboolean bSupported = AL_FALSE;
	ALsizei i=0;

	if (extName)
	{
		if (!stricmp(extName, "EAX") || !stricmp(extName, "EAX2.0"))
		{
			bSupported = CheckEAXSupport(extName);
		}
		else if (!stricmp(extName, "AL_EXT_OFFSET") || !stricmp(extName, "AL_EXT_LINEAR_DISTANCE") ||
			     !stricmp(extName, "AL_EXT_EXPONENT_DISTANCE"))
		{
			bSupported = AL_TRUE;
		}
	}
	else
	{
		alSetError(AL_INVALID_VALUE);
	}

	return bSupported;
}


ALAPI ALvoid * ALAPIENTRY alGetProcAddress(const ALchar *funcName)
{
	ALsizei i=0;
	ALvoid *pAddress;

	while ((function[i].funcName)&&(strcmp((char *)function[i].funcName,(char *)funcName)))
		i++;
	pAddress = function[i].address;

	return pAddress;
}


ALAPI ALenum ALAPIENTRY alGetEnumValue(const ALchar *enumName)
{
	ALsizei i=0;
	ALenum	val;

	while ((enumeration[i].enumName)&&(strcmp(enumeration[i].enumName,enumName)))
		i++;
	val = enumeration[i].value;

	return val;
}
