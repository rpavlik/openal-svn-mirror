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

#include "globals.h"
#include "alExtension.h"
#include "alc.h"
#include <string.h>

#pragma export on 

ALAPI ALboolean ALAPIENTRY alIsExtensionPresent(ALubyte *extName)
{
	return AL_FALSE;
}

ALAPI ALvoid * ALAPIENTRY alGetProcAddress(ALubyte *fname)
{
	return NULL;
}

ALAPI ALenum ALAPIENTRY alGetEnumValue (ALubyte *ename)
{
	if (strcmp("AL_INVALID", (const char *)ename) == 0) { return AL_INVALID; }
	if (strcmp("ALC_INVALID", (const char *)ename) == 0) { return ALC_INVALID; }
	if (strcmp("AL_NONE", (const char *)ename) == 0) { return AL_NONE; }
	if (strcmp("AL_FALSE", (const char *)ename) == 0) { return AL_FALSE; }
	if (strcmp("ALC_FALSE", (const char *)ename) == 0) { return ALC_FALSE; }
	if (strcmp("AL_TRUE", (const char *)ename) == 0) { return AL_TRUE; }
	if (strcmp("ALC_TRUE", (const char *)ename) == 0) { return ALC_TRUE; }
	if (strcmp("AL_SOURCE_RELATIVE", (const char *)ename) == 0) { return AL_SOURCE_RELATIVE; }
	if (strcmp("AL_CONE_INNER_ANGLE", (const char *)ename) == 0) { return AL_CONE_INNER_ANGLE; }
	if (strcmp("AL_CONE_OUTER_ANGLE", (const char *)ename) == 0) { return AL_CONE_OUTER_ANGLE; }
	if (strcmp("AL_PITCH", (const char *)ename) == 0) { return AL_PITCH; }
	if (strcmp("AL_POSITION", (const char *)ename) == 0) { return AL_POSITION; }
	if (strcmp("AL_DIRECTION", (const char *)ename) == 0) { return AL_DIRECTION; }
	if (strcmp("AL_VELOCITY", (const char *)ename) == 0) { return AL_VELOCITY; }
	if (strcmp("AL_LOOPING", (const char *)ename) == 0) { return AL_LOOPING; }
	if (strcmp("AL_BUFFER", (const char *)ename) == 0) { return AL_BUFFER; }
	if (strcmp("AL_GAIN", (const char *)ename) == 0) { return AL_GAIN; }
	if (strcmp("AL_MIN_GAIN", (const char *)ename) == 0) { return AL_MIN_GAIN; }
	if (strcmp("AL_MAX_GAIN", (const char *)ename) == 0) { return AL_MAX_GAIN; }
	if (strcmp("AL_ORIENTATION", (const char *)ename) == 0) { return AL_ORIENTATION; }
	if (strcmp("AL_REFERENCE_DISTANCE", (const char *)ename) == 0) { return AL_REFERENCE_DISTANCE; }
	if (strcmp("AL_ROLLOFF_FACTOR", (const char *)ename) == 0) { return AL_ROLLOFF_FACTOR; }
	if (strcmp("AL_CONE_OUTER_GAIN", (const char *)ename) == 0) { return AL_CONE_OUTER_GAIN; }
	if (strcmp("AL_MAX_DISTANCE", (const char *)ename) == 0) { return AL_MAX_DISTANCE; }
	if (strcmp("AL_SOURCE_STATE", (const char *)ename) == 0) { return AL_SOURCE_STATE; }
	if (strcmp("AL_INITIAL", (const char *)ename) == 0) { return AL_INITIAL; }
	if (strcmp("AL_PLAYING", (const char *)ename) == 0) { return AL_PLAYING; }
	if (strcmp("AL_PAUSED", (const char *)ename) == 0) { return AL_PAUSED; }
	if (strcmp("AL_STOPPED", (const char *)ename) == 0) { return AL_STOPPED; }
	if (strcmp("AL_BUFFERS_QUEUED", (const char *)ename) == 0) { return AL_BUFFERS_QUEUED; }
	if (strcmp("AL_BUFFERS_PROCESSED", (const char *)ename) == 0) { return AL_BUFFERS_PROCESSED; }
	if (strcmp("AL_FORMAT_MONO8", (const char *)ename) == 0) { return AL_FORMAT_MONO8; }
	if (strcmp("AL_FORMAT_MONO16", (const char *)ename) == 0) { return AL_FORMAT_MONO16; }
	if (strcmp("AL_FORMAT_STEREO8", (const char *)ename) == 0) { return AL_FORMAT_STEREO8; }
	if (strcmp("AL_FORMAT_STEREO16", (const char *)ename) == 0) { return AL_FORMAT_STEREO16; }
	if (strcmp("AL_FREQUENCY", (const char *)ename) == 0) { return AL_FREQUENCY; }
	if (strcmp("AL_SIZE", (const char *)ename) == 0) { return AL_SIZE; }
	if (strcmp("AL_UNUSED", (const char *)ename) == 0) { return AL_UNUSED; }
	if (strcmp("AL_PENDING", (const char *)ename) == 0) { return AL_PENDING; }
	if (strcmp("AL_PROCESSED", (const char *)ename) == 0) { return AL_PROCESSED; }
	if (strcmp("ALC_MAJOR_VERSION", (const char *)ename) == 0) { return ALC_MAJOR_VERSION; }
	if (strcmp("ALC_MINOR_VERSION", (const char *)ename) == 0) { return ALC_MINOR_VERSION; }
	if (strcmp("ALC_ATTRIBUTES_SIZE", (const char *)ename) == 0) { return ALC_ATTRIBUTES_SIZE; }
	if (strcmp("ALC_ALL_ATTRIBUTES", (const char *)ename) == 0) { return ALC_ALL_ATTRIBUTES; }
	if (strcmp("ALC_DEFAULT_DEVICE_SPECIFIER", (const char *)ename) == 0) { return ALC_DEFAULT_DEVICE_SPECIFIER; }
	if (strcmp("ALC_DEVICE_SPECIFIER", (const char *)ename) == 0) { return ALC_DEVICE_SPECIFIER; }
	if (strcmp("ALC_EXTENSIONS", (const char *)ename) == 0) { return ALC_EXTENSIONS; }
	if (strcmp("ALC_FREQUENCY", (const char *)ename) == 0) { return ALC_FREQUENCY; }
	if (strcmp("ALC_REFRESH", (const char *)ename) == 0) { return ALC_REFRESH; }
	if (strcmp("ALC_SYNC", (const char *)ename) == 0) { return ALC_SYNC; }
	if (strcmp("AL_NO_ERROR", (const char *)ename) == 0) { return AL_NO_ERROR; }
	if (strcmp("AL_INVALID_NAME", (const char *)ename) == 0) { return AL_INVALID_NAME; }
	if (strcmp("AL_INVALID_ENUM", (const char *)ename) == 0) { return AL_INVALID_ENUM; }
	if (strcmp("AL_INVALID_VALUE", (const char *)ename) == 0) { return AL_INVALID_VALUE; }
	if (strcmp("AL_INVALID_OPERATION", (const char *)ename) == 0) { return AL_INVALID_OPERATION; }
	if (strcmp("AL_OUT_OF_MEMORY", (const char *)ename) == 0) { return AL_OUT_OF_MEMORY; }
	if (strcmp("ALC_NO_ERROR", (const char *)ename) == 0) { return ALC_NO_ERROR; }
	if (strcmp("ALC_INVALID_DEVICE", (const char *)ename) == 0) { return ALC_INVALID_DEVICE; }
	if (strcmp("ALC_INVALID_CONTEXT", (const char *)ename) == 0) { return ALC_INVALID_CONTEXT; }
	if (strcmp("ALC_INVALID_ENUM", (const char *)ename) == 0) { return ALC_INVALID_ENUM; }
	if (strcmp("ALC_INVALID_VALUE", (const char *)ename) == 0) { return ALC_INVALID_VALUE; }
	if (strcmp("ALC_OUT_OF_MEMORY", (const char *)ename) == 0) { return ALC_OUT_OF_MEMORY; }
	if (strcmp("AL_VENDOR", (const char *)ename) == 0) { return AL_VENDOR; }
	if (strcmp("AL_VERSION", (const char *)ename) == 0) { return AL_VERSION; }
	if (strcmp("AL_RENDERER", (const char *)ename) == 0) { return AL_RENDERER; }
	if (strcmp("AL_EXTENSIONS", (const char *)ename) == 0) { return AL_EXTENSIONS; }
	if (strcmp("AL_DOPPLER_FACTOR", (const char *)ename) == 0) { return AL_DOPPLER_FACTOR; }
	if (strcmp("AL_DOPPLER_VELOCITY", (const char *)ename) == 0) { return AL_DOPPLER_VELOCITY; }
	if (strcmp("AL_DISTANCE_MODEL", (const char *)ename) == 0) { return AL_DISTANCE_MODEL; }
	if (strcmp("AL_INVERSE_DISTANCE", (const char *)ename) == 0) { return AL_INVERSE_DISTANCE; }
	if (strcmp("AL_INVERSE_DISTANCE_CLAMPED", (const char *)ename) == 0) { return AL_INVERSE_DISTANCE_CLAMPED; }
	
	return -1;
}

#pragma export off
