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

#include "include\alMain.h"
#include "al\alc.h"
#include "include\alError.h"
#include "include\alEax.h"
#include "include\alListener.h"
#include <objbase.h>
 
// EAX 2.0 GUIDs
const GUID DSPROPSETID_EAX20_ListenerProperties
				= { 0x306a6a8, 0xb224, 0x11d2, { 0x99, 0xe5, 0x0, 0x0, 0xe8, 0xd8, 0xc7, 0x22 } };

const GUID DSPROPSETID_EAX20_BufferProperties
				= { 0x306a6a7, 0xb224, 0x11d2, {0x99, 0xe5, 0x0, 0x0, 0xe8, 0xd8, 0xc7, 0x22 } };

/*
	Test for support of appropriate EAX Version
*/
ALboolean CheckEAXSupport(ALubyte *szEAXName)
{
	ALCcontext		*ALContext;
	ALCdevice		*ALCDevice;
	ALsource		*ALSource;
	LPKSPROPERTYSET	lpPropertySet;
	GUID			ListenerGuid, BufferGuid;
	ALuint			ListenerProperty, BufferProperty;
	ALuint			i, ulSupport;
	ALuint			property, size;
	ALint			value = 0xFFFFFFFF;
	ALfloat			fvalue;
	ALboolean		bSourceGenerated = AL_FALSE;
	ALboolean		bEAXSupported = AL_FALSE;
	
	ALContext = alcGetCurrentContext();
	ALCDevice = alcGetContextsDevice(ALContext);
	SuspendContext(ALContext);

	// To test for EAX support we will need a valid Source

	ALSource = ALContext->Source;
	lpPropertySet = NULL;

	// See if one has already been created
	for (i=0;i<ALContext->SourceCount;i++)
	{
		if (alIsSource((ALuint)ALSource))
		{
			if (ALSource->uservalue3)
			{
				lpPropertySet = ALSource->uservalue3;
				break;
			}
		}
		ALSource = ALSource->next;
	}

	// If we didn't find a valid source, create one now
	if (lpPropertySet == NULL)
	{
		alGenSources(1, &((ALuint)(ALSource)));

		if (alGetError() == AL_NO_ERROR)
		{
			bSourceGenerated = AL_TRUE;
			if (ALSource->uservalue3)
			{
				lpPropertySet = ALSource->uservalue3;
			}
		}
	}

	// If Property Set Interface hasn't been obtained, EAX support is not available
	if (lpPropertySet == NULL)
	{
		if (bSourceGenerated)
			alDeleteSources(1, &(ALuint)ALSource);

		ProcessContext(ALContext);
		return AL_FALSE;
	}

	if ( (strcmp(szEAXName, "EAX") == 0) || (strcmp(szEAXName, "EAX2.0") == 0) )
	{
		ListenerGuid = DSPROPSETID_EAX20_ListenerProperties;
		BufferGuid = DSPROPSETID_EAX20_BufferProperties;
		ListenerProperty = 1;	// LISTENER_ALL
		BufferProperty = 1;		// BUFFER_ALL
		property = 2;			// ROOM
		size = sizeof(ALint);
		value = -10000;
	}
	else
	{
		// Unknown EAX Name
		if (bSourceGenerated)
			alDeleteSources(1, &((ALuint)ALSource));

		ProcessContext(ALContext);
		return AL_FALSE;
	}

	if (SUCCEEDED(IKsPropertySet_QuerySupport(lpPropertySet, &ListenerGuid, ListenerProperty, &ulSupport)))
	{
		if ( (ulSupport & KSPROPERTY_SUPPORT_GET) && (ulSupport & KSPROPERTY_SUPPORT_SET))
		{
			if (SUCCEEDED(IKsPropertySet_QuerySupport(lpPropertySet, &BufferGuid, BufferProperty, &ulSupport)))
			{
				if ( (ulSupport & KSPROPERTY_SUPPORT_GET) && (ulSupport & KSPROPERTY_SUPPORT_SET) )
				{
					// Fully supported !
					bEAXSupported = AL_TRUE;
		
					// Set Default Property
					if (value != 0xFFFFFFFF)
						IKsPropertySet_Set(lpPropertySet, &ListenerGuid, property, NULL, 0, &value, sizeof(ALint));
					else
						IKsPropertySet_Set(lpPropertySet, &ListenerGuid, property, NULL, 0, &fvalue, sizeof(ALfloat));
				}
			}
		}
	}

	if (bSourceGenerated)
		alDeleteSources(1, &((ALuint)ALSource));

	ProcessContext(ALContext);
	return bEAXSupported;
}


/*
	EAXGet(propertySetID, property, source, value, size)

	propertySetID : GUID of EAX Property Set (defined in eax.h files)
	property	  : Property in Property Set to retrieve (enumerations defined in eax.h files)
	source		  : Source to use to retrieve EAX affect (this can be NULL for Listener Property Sets)
	value		  : Pointer to memory location to retrieve value
	size		  : Size of data pointed to by value

	Returns AL_INVALID_NAME if a valid Source name was required but not given
	Returns AL_INVALID_OPERATION if the Source is 2D
	Returns AL_INVALID_VALUE if the GUID is not recognized
*/
ALAPI ALenum ALAPIENTRY EAXGet(const GUID *propertySetID,ALuint property,ALuint source,ALvoid *value,ALuint size)
{
	ALuint i;
	ALsource	*ALSource;
	ALCcontext	*ALContext;
	ALuint		ulBytes;
	ALenum		ALErrorCode = AL_NO_ERROR;
	ALboolean	bGenSource = AL_FALSE;
	ALboolean	bEAX2B = AL_FALSE, bEAX2L = AL_FALSE;

	ALContext = alcGetCurrentContext();
	SuspendContext(ALContext);

	// Determine which EAX Property is being used
	if ( IsEqualGUID(propertySetID, &DSPROPSETID_EAX20_BufferProperties) )
		bEAX2B = AL_TRUE;
	else if ( IsEqualGUID(propertySetID, &DSPROPSETID_EAX20_ListenerProperties) )
		bEAX2L = AL_TRUE;
	else
		ALErrorCode = AL_INVALID_VALUE;

	if (bEAX2B)
	{
		if (alIsSource(source))
		{
			ALSource = (ALsource*)source;
			if (ALSource->uservalue3)
			{
				if (FAILED(IKsPropertySet_Get((LPKSPROPERTYSET)ALSource->uservalue3, propertySetID, property, NULL, 0, value, size, &ulBytes)))
						ALErrorCode = AL_INVALID_OPERATION;
			}
			else
				ALErrorCode = AL_INVALID_OPERATION;
		}
		else
			ALErrorCode = AL_INVALID_NAME;
	}
	else if (bEAX2L)
	{
		// If source is valid use that, otherwise find a source
		if ( alIsSource(source) && ((ALsource*)(source))->uservalue3 )
		{
			ALSource = (ALsource*)source;
		}
		else
		{
			ALSource = ALContext->Source;
			
			// See if one has already been created
			for (i=0;i<ALContext->SourceCount;i++)
			{
				if (ALSource->uservalue3)
					break;
				ALSource = ALSource->next;
			}

			// If an appropriate source wasn't created, generate one now
			if (ALSource == NULL)
			{
				alGenSources(1, &(ALuint)ALSource);
				if (alGetError() == AL_NO_ERROR)
					bGenSource = AL_TRUE;
			}
		}

		if (alIsSource((ALuint)ALSource) && ALSource->uservalue3)
		{
			if (FAILED(IKsPropertySet_Get((LPKSPROPERTYSET)ALSource->uservalue3, propertySetID, property, NULL, 0, value, size, &ulBytes)))
				ALErrorCode = AL_INVALID_OPERATION;
		}
		else
			ALErrorCode = AL_INVALID_OPERATION;

		// If we generated a source to get the EAX Listener property, release it now
		if (bGenSource)
			alDeleteSources(1, &(ALuint)ALSource);
	}

	ProcessContext(ALContext);
	return ALErrorCode;
}


/*
	EAXSet(propertySetID, property, source, value, size)

	propertySetID : GUID of EAX Property Set (defined in eax.h files)
	property	  : Property in Property Set to affect (enumerations defined in eax.h files)
	source		  : Source to apply EAX affects to (this can be NULL for Listener Property Sets)
	value		  : Pointer to value to set
	size		  : Size of data pointed to by value

	Returns AL_INVALID_NAME if a valid Source name was required but not given
	Returns AL_INVALID_OPERATION if the Source is 2D or the EAX call fails
	Returns AL_INVALID_VALUE if the GUID is not recognized
*/
ALAPI ALenum ALAPIENTRY EAXSet(const GUID *propertySetID,ALuint property,ALuint source,ALvoid *value,ALuint size)
{
	ALsource	*ALSource;
	ALCcontext	*ALCContext;
	ALuint		i;
	ALenum		ALErrorCode = AL_NO_ERROR;
	ALboolean	bGenSource = AL_FALSE;
	ALboolean	bEAX2B = AL_FALSE, bEAX2L = AL_FALSE;	

	ALCContext=alcGetCurrentContext();
	SuspendContext(ALCContext);

	// Determine which EAX Property is being set
	if ( IsEqualGUID(propertySetID, &DSPROPSETID_EAX20_BufferProperties) )
		bEAX2B = AL_TRUE;
	else if ( IsEqualGUID(propertySetID, &DSPROPSETID_EAX20_ListenerProperties) )
		bEAX2L = AL_TRUE;
	else
		ALErrorCode = AL_INVALID_VALUE;

	// Process EAX Buffer related calls
	if (bEAX2B)
	{
		if (alIsSource(source))
		{
			ALSource = (ALsource*)source;
			if (ALSource->uservalue3)
			{
				if (FAILED(IKsPropertySet_Set((LPKSPROPERTYSET)ALSource->uservalue3, propertySetID, property, NULL, 0, value, size)))
					ALErrorCode = AL_INVALID_OPERATION;
			}
			else
				ALErrorCode = AL_INVALID_OPERATION;
		}
		else
			ALErrorCode = AL_INVALID_NAME;
	}
	else if (bEAX2L)
	{
		// If source is valid use that, otherwise find a source
		if (alIsSource(source) && ((ALsource*)(source))->uservalue3)
		{
			ALSource = (ALsource*)source;
		}
		else
		{
			ALSource = ALCContext->Source;

			// See if one has already been created
			for (i=0;i<ALCContext->SourceCount;i++)
			{
				if (alIsSource((ALuint)ALSource))
				{
					if (ALSource->uservalue3)
						break;
				}
				ALSource = ALSource->next;
			}

			// If an appropriate source wasn't created, generate one now
			if (ALSource == NULL)
			{
				alGenSources(1, &(ALuint)ALSource);
				if (alGetError() == AL_NO_ERROR)
					bGenSource = AL_TRUE;
			}
		}

		// If we found a valid Source, then apply EAX affect using EAXUnified if necessary
		if (alIsSource((ALuint)ALSource) && ALSource->uservalue3)
		{
			if (FAILED(IKsPropertySet_Set((LPKSPROPERTYSET)ALSource->uservalue3, propertySetID, property, NULL, 0, value, size)))
				ALErrorCode = AL_INVALID_OPERATION;
		}
		else
			ALErrorCode = AL_INVALID_OPERATION;
	}

	// If we generated a source to set the EAX Listener property, release it now
	if (bGenSource)
		alDeleteSources(1, &(ALuint)ALSource);
	
	ProcessContext(ALCContext);
	return ALErrorCode;
}
