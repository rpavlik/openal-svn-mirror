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

#if defined(__APPLE__) & defined(__MACH__) // check for OS X
#define MAC_OS_X
#endif

#ifdef MAC_OS_X
#include <stdlib.h>
#else
#ifdef TARGET_CLASSIC 
#include <Sound.h>
#else
#include <Carbon/Carbon.h>
#endif
#endif

#include "al.h"
#include "alc.h"

// two legacy functions in AL Mac
// AL_MAIN functions
ALAPI ALvoid ALAPIENTRY alInit(ALint *argc, ALubyte **argv);
ALAPI ALvoid ALAPIENTRY alExit(ALvoid);

// ALC-global variables
ALCenum LastError=ALC_NO_ERROR;
ALCdevice *pOpenDevice = 0;
void *pContext = 0;

#pragma export on

ALCAPI void * ALCAPIENTRY alcCreateContext( ALCdevice *dev,  ALint* attrlist )
{
	// if have device pointer and there isn't a context yet, then allow creation of new one
	if ((dev != 0) && (pContext == 0))
	{
		pContext = (void *) dev;
		alInit(NULL, 0);  // call alInit -- legacy call
		return pContext;
	} else
	{
		return 0 ;
	}
}

ALCAPI ALCubyte*  ALCAPIENTRY alcGetString(ALCdevice *device,ALCenum param)
{
	return NULL;
}

ALCAPI ALCvoid    ALCAPIENTRY alcGetIntegerv(ALCdevice *device,ALCenum param,ALCsizei size,ALCint *data)
{
}

ALCAPI ALCboolean ALCAPIENTRY alcMakeContextCurrent(ALCcontext *context)
{
	return AL_TRUE;
}

ALCAPI void ALCAPIENTRY alcProcessContext( ALvoid *alcHandle )
{
}

ALCAPI void ALCAPIENTRY alcSuspendContext( ALvoid *alcHandle )
{
}

ALCAPI ALCvoid ALCAPIENTRY alcDestroyContext(ALCcontext *context)
{
	if (context == pContext)
	{
		alExit();
		pContext = 0;
	} else
	{
		LastError = ALC_INVALID_CONTEXT;
	}
}

ALCAPI ALCenum ALCAPIENTRY alcGetError(ALCdevice *device)
{
	ALCenum errorCode;

	errorCode=LastError;
	LastError=AL_NO_ERROR;
	return errorCode;
}

ALCAPI void * ALCAPIENTRY alcGetCurrentContext( ALvoid )
{
	return pContext;
}

ALCAPI ALCdevice* ALCAPIENTRY alcGetContextsDevice(ALCcontext *context)
{
	if ((context == pContext) && (context != 0))
	{
		return pOpenDevice;
	} else
	{
		LastError = ALC_INVALID_CONTEXT;
		return 0;
	}
}

ALCAPI ALCdevice* ALCAPIENTRY alcOpenDevice(ALCubyte *deviceName)
{
	// allow opening of one device
	if (pOpenDevice == 0)
	{
		pOpenDevice = &pOpenDevice; // put in a valid memory address
		
#ifdef MAC_OS_X
        no_smInit();
#endif

		return pOpenDevice;
	} else
	{
		return 0;
	}
}

ALCAPI void ALCAPIENTRY alcCloseDevice( ALCdevice *dev )
{
#ifdef MAC_OS_X
	no_smTerminate();
#endif

	pOpenDevice = 0;
}

ALCAPI ALboolean ALCAPIENTRY alcIsExtensionPresent(ALCdevice *device, ALubyte *extName)
{
	return AL_FALSE;
}

ALCAPI ALvoid  * ALCAPIENTRY alcGetProcAddress(ALCdevice *device, ALubyte *funcName)
{
	return 0;
}

ALCAPI ALenum ALCAPIENTRY alcGetEnumValue(ALCdevice *device, ALubyte *enumName)
{
	return AL_NO_ERROR;
}

#pragma export off
