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

#include "Include/alMain.h"
#include "AL/al.h"
#include "AL/alc.h"
#include "Include/alError.h"
#include "Include/alEax.h"
#include "Include/alListener.h"
#include <objbase.h>

typedef enum
{
	NONE	= 0x00,

	// Buffer / Source properties
	EAX2B	= 0x02,

	// Listener / Global properties
	EAX2L	= 0x10000
} EAXGUID;

#define EAXBUFFERGUID	(EAX2B)
#define EAXLISTENERGUID	(EAX2L)

// EAX 2.0 GUIDs
const GUID DSPROPSETID_EAX20_ListenerProperties
				= { 0x306a6a8, 0xb224, 0x11d2, { 0x99, 0xe5, 0x0, 0x0, 0xe8, 0xd8, 0xc7, 0x22 } };

const GUID DSPROPSETID_EAX20_BufferProperties
				= { 0x306a6a7, 0xb224, 0x11d2, {0x99, 0xe5, 0x0, 0x0, 0xe8, 0xd8, 0xc7, 0x22 } };

ALboolean LongInRange(long lValue, long lMin, long lMax);
ALboolean ULongInRange(unsigned long ulValue, unsigned long ulMin, unsigned long ulMax);
ALboolean FloatInRange(float flValue, float flMin, float flMax);

/*
	Test for support of appropriate EAX Version
*/
ALboolean CheckEAXSupport(const ALubyte *szEAXName)
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
		alGenSources(1, ((ALuint *)(&ALSource)));

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
			alDeleteSources(1, (ALuint *)&ALSource);

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
			alDeleteSources(1, ((ALuint *)&ALSource));

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
		alDeleteSources(1, ((ALuint *)&ALSource));

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
	EAXGUID		eaxGuid = NONE;

	ALContext = alcGetCurrentContext();
	SuspendContext(ALContext);

	// Determine which EAX Property is being used
	if ( IsEqualGUID(propertySetID, &DSPROPSETID_EAX20_BufferProperties) )
		eaxGuid = EAX2B;
	else if ( IsEqualGUID(propertySetID, &DSPROPSETID_EAX20_ListenerProperties) )
		eaxGuid = EAX2L;
	else
		ALErrorCode = AL_INVALID_VALUE;

	if (eaxGuid & EAXBUFFERGUID)
	{
		if (alIsSource(source))
		{
			ALSource = (ALsource*)source;
			if (ALSource->uservalue3)
			{
				// EAX 2.0 Buffer call
				if (eaxGuid == EAX2B)
				{
					if (FAILED(IKsPropertySet_Get((LPKSPROPERTYSET)ALSource->uservalue3, propertySetID, property, NULL, 0,
							value, size, &ulBytes)))
						ALErrorCode = AL_INVALID_OPERATION;
				}
			}
			else
				ALErrorCode = AL_INVALID_OPERATION;
		}
		else
			ALErrorCode = AL_INVALID_NAME;
	}
	else if (eaxGuid & EAXLISTENERGUID)
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
				alGenSources(1, (ALuint *)&ALSource);
				if (alGetError() == AL_NO_ERROR)
					bGenSource = AL_TRUE;
			}
		}

		if (alIsSource((ALuint)ALSource) && ALSource->uservalue3)
		{
			// EAX 2.0 Listener call
			if (eaxGuid == EAX2L)
			{
				if (FAILED(IKsPropertySet_Get((LPKSPROPERTYSET)ALSource->uservalue3, propertySetID, property, NULL, 0,
						value, size, &ulBytes)))
					ALErrorCode = AL_INVALID_OPERATION;
			}
		}
		else
			ALErrorCode = AL_INVALID_OPERATION;

		// If we generated a source to get the EAX Listener property, release it now
		if (bGenSource)
			alDeleteSources(1, (ALuint *)&ALSource);
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
ALAPI ALenum ALAPIENTRY EAXSet(const GUID *propertySetID,ALuint property,ALuint source,ALvoid *pValue,ALuint size)
{
	ALsource	*ALSource;
	ALCcontext	*ALCContext;
	ALuint		i, ulBytesReturned;
	ALenum		ALErrorCode = AL_NO_ERROR;
	EAXGUID		eaxGuid;
	ALboolean	bGenSource = AL_FALSE;
	ALboolean	bSetValue = AL_FALSE;
	ALboolean	bGetValue = AL_FALSE;
	
	ALCContext=alcGetCurrentContext();
	SuspendContext(ALCContext);

	// Determine which EAX Property is being set
	if ( IsEqualGUID(propertySetID, &DSPROPSETID_EAX20_BufferProperties) )
		eaxGuid = EAX2B;
	else if ( IsEqualGUID(propertySetID, &DSPROPSETID_EAX20_ListenerProperties) )
		eaxGuid = EAX2L;
	else
		ALErrorCode = AL_INVALID_VALUE;

	// Process EAX Buffer related calls
	if (eaxGuid & EAXBUFFERGUID)
	{
		if (alIsSource(source))
		{
			ALSource = (ALsource*)source;
			if (ALSource->uservalue3)
			{
				// EAX 2.0 Buffer call
				if (eaxGuid == EAX2B)
				{
					switch(property & ~DSPROPERTY_EAXBUFFER_DEFERRED)
					{
					case DSPROPERTY_EAXBUFFER_NONE:
						bSetValue = AL_TRUE;
						break;

					case DSPROPERTY_EAXBUFFER_ALLPARAMETERS:
						if ((pValue) && (size >= sizeof(EAXBUFFERPROPERTIES)))
						{
							if (memcmp((void*)(&ALSource->EAX20BP), pValue, sizeof(EAXBUFFERPROPERTIES)))
							{
								if ( (LongInRange(((LPEAXBUFFERPROPERTIES)pValue)->lDirect, EAXBUFFER_MINDIRECT, EAXBUFFER_MAXDIRECT)) &&
									 (LongInRange(((LPEAXBUFFERPROPERTIES)pValue)->lDirectHF, EAXBUFFER_MINDIRECTHF, EAXBUFFER_MAXDIRECTHF)) &&
									 (LongInRange(((LPEAXBUFFERPROPERTIES)pValue)->lRoom, EAXBUFFER_MINROOM, EAXBUFFER_MAXROOM)) &&
									 (LongInRange(((LPEAXBUFFERPROPERTIES)pValue)->lRoomHF, EAXBUFFER_MINROOMHF, EAXBUFFER_MAXROOMHF)) &&
									 (LongInRange(((LPEAXBUFFERPROPERTIES)pValue)->lObstruction, EAXBUFFER_MINOBSTRUCTION, EAXBUFFER_MAXOBSTRUCTION)) &&
									 (FloatInRange(((LPEAXBUFFERPROPERTIES)pValue)->flObstructionLFRatio, EAXBUFFER_MINOBSTRUCTIONLFRATIO, EAXBUFFER_MAXOBSTRUCTIONLFRATIO)) &&
									 (LongInRange(((LPEAXBUFFERPROPERTIES)pValue)->lOcclusion, EAXBUFFER_MINOCCLUSION, EAXBUFFER_MAXOCCLUSION)) &&
									 (FloatInRange(((LPEAXBUFFERPROPERTIES)pValue)->flOcclusionLFRatio, EAXBUFFER_MINOCCLUSIONLFRATIO, EAXBUFFER_MAXOCCLUSIONLFRATIO)) &&
									 (FloatInRange(((LPEAXBUFFERPROPERTIES)pValue)->flOcclusionRoomRatio, EAXBUFFER_MINOCCLUSIONROOMRATIO, EAXBUFFER_MAXOCCLUSIONROOMRATIO)) &&
									 (LongInRange(((LPEAXBUFFERPROPERTIES)pValue)->lOutsideVolumeHF, EAXBUFFER_MINOUTSIDEVOLUMEHF, EAXBUFFER_MAXOUTSIDEVOLUMEHF)) &&
									 (FloatInRange(((LPEAXBUFFERPROPERTIES)pValue)->flRoomRolloffFactor, EAXBUFFER_MINROOMROLLOFFFACTOR, EAXBUFFER_MAXROOMROLLOFFFACTOR)) &&
									 (FloatInRange(((LPEAXBUFFERPROPERTIES)pValue)->flAirAbsorptionFactor, EAXBUFFER_MINAIRABSORPTIONFACTOR, EAXBUFFER_MAXAIRABSORPTIONFACTOR)) &&
									 (ULongInRange(((LPEAXBUFFERPROPERTIES)pValue)->dwFlags, 0, ~EAXBUFFERFLAGS_RESERVED)) )
								{
									memcpy((void*)(&ALSource->EAX20BP), pValue, sizeof(EAXBUFFERPROPERTIES));
									bSetValue = AL_TRUE;
								}
								else
								{
									ALErrorCode = AL_INVALID_OPERATION;
								}
							}
						}
						else
						{
							ALErrorCode = AL_INVALID_OPERATION;
						}
						break;

					case DSPROPERTY_EAXBUFFER_DIRECT:
						if ((pValue) && (size >= sizeof(long)) && (LongInRange(*((long*)pValue), EAXBUFFER_MINDIRECT, EAXBUFFER_MAXDIRECT)))
						{
							bSetValue = (ALSource->EAX20BP.lDirect != (*(long*)pValue));
							ALSource->EAX20BP.lDirect = (*(long*)pValue);
						}
						else
						{
							ALErrorCode = AL_INVALID_OPERATION;
						}
						break;

					case DSPROPERTY_EAXBUFFER_DIRECTHF:
						if ((pValue) && (size >= sizeof(long)) && (LongInRange(*((long *)pValue), EAXBUFFER_MINDIRECTHF, EAXBUFFER_MAXDIRECTHF)))
						{
							bSetValue = (ALSource->EAX20BP.lDirectHF != (*(long*)pValue));
							ALSource->EAX20BP.lDirectHF = (*(long*)pValue);
						}
						else
						{
							ALErrorCode = AL_INVALID_OPERATION;
						}
						break;

					case DSPROPERTY_EAXBUFFER_ROOM:
						if ((pValue) && (size >= sizeof(long)) && (LongInRange(*((long *)pValue), EAXBUFFER_MINROOM, EAXBUFFER_MAXROOM)))
						{
							bSetValue = (ALSource->EAX20BP.lRoom != (*(long*)pValue));
							ALSource->EAX20BP.lRoom = (*(long*)pValue);
						}
						else
						{
							ALErrorCode = AL_INVALID_OPERATION;
						}
						break;

					case DSPROPERTY_EAXBUFFER_ROOMHF:
						if ((pValue) && (size >= sizeof(long)) && (LongInRange(*((long *)pValue), EAXBUFFER_MINROOMHF, EAXBUFFER_MAXROOMHF)))
						{
							bSetValue = (ALSource->EAX20BP.lRoomHF != (*(long*)pValue));
							ALSource->EAX20BP.lRoomHF = (*(long*)pValue);
						}
						else
						{
							ALErrorCode = AL_INVALID_OPERATION;
						}
						break;

					case DSPROPERTY_EAXBUFFER_OBSTRUCTION:
						if ((pValue) && (size >= sizeof(long)) && (LongInRange(*((long *)pValue), EAXBUFFER_MINOBSTRUCTION, EAXBUFFER_MAXOBSTRUCTION)))
						{
							bSetValue = (ALSource->EAX20BP.lObstruction != (*(long*)pValue));
							ALSource->EAX20BP.lObstruction = (*(long*)pValue);
						}
						else
						{
							ALErrorCode = AL_INVALID_OPERATION;
						}
						break;

					case DSPROPERTY_EAXBUFFER_OBSTRUCTIONLFRATIO:
						if ((pValue) && (size >= sizeof(float)) && (FloatInRange(*((float *)pValue), EAXBUFFER_MINOBSTRUCTIONLFRATIO, EAXBUFFER_MAXOBSTRUCTIONLFRATIO)))
						{
							bSetValue = (ALSource->EAX20BP.flObstructionLFRatio != (*(float*)pValue));
							ALSource->EAX20BP.flObstructionLFRatio = (*(float*)pValue);
						}
						else
						{
							ALErrorCode = AL_INVALID_OPERATION;
						}
						break;

					case DSPROPERTY_EAXBUFFER_OCCLUSION:
						if ((pValue) && (size >= sizeof(long)) && (LongInRange(*((long *)pValue), EAXBUFFER_MINOCCLUSION, EAXBUFFER_MAXOCCLUSION)))
						{
							bSetValue = (ALSource->EAX20BP.lOcclusion != (*(long*)pValue));
							ALSource->EAX20BP.lOcclusion = (*(long*)pValue);
						}
						else
						{
							ALErrorCode = AL_INVALID_OPERATION;
						}
						break;

					case DSPROPERTY_EAXBUFFER_OCCLUSIONLFRATIO:
						if ((pValue) && (size >= sizeof(float)) && (FloatInRange(*((float *)pValue), EAXBUFFER_MINOCCLUSIONLFRATIO, EAXBUFFER_MAXOCCLUSIONLFRATIO)))
						{
							bSetValue = (ALSource->EAX20BP.flOcclusionLFRatio != (*(float*)pValue));
							ALSource->EAX20BP.flOcclusionLFRatio = (*(float*)pValue);
						}
						else
						{
							ALErrorCode = AL_INVALID_OPERATION;
						}
						break;

					case DSPROPERTY_EAXBUFFER_OCCLUSIONROOMRATIO:
						if ((pValue) && (size >= sizeof(float)) && (FloatInRange(*((float *)pValue), EAXBUFFER_MINOCCLUSIONROOMRATIO, EAXBUFFER_MAXOCCLUSIONROOMRATIO)))
						{
							bSetValue = (ALSource->EAX20BP.flOcclusionRoomRatio != (*(float*)pValue));
							ALSource->EAX20BP.flOcclusionRoomRatio = (*(float*)pValue);
						}
						else
						{
							ALErrorCode = AL_INVALID_OPERATION;
						}
						break;

					case DSPROPERTY_EAXBUFFER_OUTSIDEVOLUMEHF:
						if ((pValue) && (size >= sizeof(long)) && (LongInRange(*((long *)pValue), EAXBUFFER_MINOUTSIDEVOLUMEHF, EAXBUFFER_MAXOUTSIDEVOLUMEHF)))
						{
							bSetValue = (ALSource->EAX20BP.lOutsideVolumeHF != (*(long*)pValue));
							ALSource->EAX20BP.lOutsideVolumeHF = (*(long*)pValue);
						}
						else
						{
							ALErrorCode = AL_INVALID_OPERATION;
						}
						break;

					case DSPROPERTY_EAXBUFFER_ROOMROLLOFFFACTOR:
						if ((pValue) && (size >= sizeof(float)) && (FloatInRange(*((float *)pValue), EAXBUFFER_MINROOMROLLOFFFACTOR, EAXBUFFER_MAXROOMROLLOFFFACTOR)))
						{
							bSetValue = (ALSource->EAX20BP.flRoomRolloffFactor != (*(float*)pValue));
							ALSource->EAX20BP.flRoomRolloffFactor = (*(float*)pValue);
						}
						else
						{
							ALErrorCode = AL_INVALID_OPERATION;
						}
						break;

					case DSPROPERTY_EAXBUFFER_AIRABSORPTIONFACTOR:
						if ((pValue) && (size >= sizeof(float)) && (FloatInRange(*((float *)pValue), EAXBUFFER_MINAIRABSORPTIONFACTOR, EAXBUFFER_MAXAIRABSORPTIONFACTOR)))
						{
							bSetValue = (ALSource->EAX20BP.flAirAbsorptionFactor != (*(float*)pValue));
							ALSource->EAX20BP.flAirAbsorptionFactor = (*(float*)pValue);
						}
						else
						{
							ALErrorCode = AL_INVALID_OPERATION;
						}
						break;

					case DSPROPERTY_EAXBUFFER_FLAGS:
						if ((pValue) && (size >= sizeof(unsigned long)) && (ULongInRange(*((unsigned long *)pValue), 0, ~EAXBUFFERFLAGS_RESERVED)))
						{
							bSetValue = (ALSource->EAX20BP.dwFlags != (*(unsigned long*)pValue));
							ALSource->EAX20BP.dwFlags = (*(unsigned long*)pValue);
						}
						else
						{
							ALErrorCode = AL_INVALID_OPERATION;
						}
						break;

					default:
						ALErrorCode = AL_INVALID_OPERATION;
					}

					if (bSetValue)
					{
						if (FAILED(IKsPropertySet_Set((LPKSPROPERTYSET)ALSource->uservalue3, propertySetID, property, NULL, 0, pValue, size)))
							ALErrorCode = AL_INVALID_OPERATION;
					}
				}
			}
			else
				ALErrorCode = AL_INVALID_OPERATION;
		}
		else
			ALErrorCode = AL_INVALID_NAME;
	}
	else if (eaxGuid & EAXLISTENERGUID)
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
				alGenSources(1, (ALuint *)&ALSource);
				if (alGetError() == AL_NO_ERROR)
					bGenSource = AL_TRUE;
			}
		}

		if (alIsSource((ALuint)ALSource) && ALSource->uservalue3)
		{
			// EAX 2.0 Listener call
			if (eaxGuid == EAX2L)
			{
				switch(property & ~DSPROPERTY_EAXLISTENER_DEFERRED)
				{
				case DSPROPERTY_EAXLISTENER_NONE:
					bSetValue = AL_TRUE;
					break;

				case DSPROPERTY_EAXLISTENER_ALLPARAMETERS:
					if ((pValue) && (size >= sizeof(EAXLISTENERPROPERTIES)))
					{
						if (memcmp((void*)(&ALCContext->Listener.EAX20LP), pValue, sizeof(EAXLISTENERPROPERTIES)))
						{
							if ( (LongInRange(((LPEAXLISTENERPROPERTIES)pValue)->lRoom, EAXLISTENER_MINROOM, EAXLISTENER_MAXROOM)) &&
								 (LongInRange(((LPEAXLISTENERPROPERTIES)pValue)->lRoomHF, EAXLISTENER_MINROOMHF, EAXLISTENER_MAXROOMHF)) &&
								 (FloatInRange(((LPEAXLISTENERPROPERTIES)pValue)->flRoomRolloffFactor, EAXLISTENER_MINROOMROLLOFFFACTOR, EAXLISTENER_MAXROOMROLLOFFFACTOR)) &&
								 (FloatInRange(((LPEAXLISTENERPROPERTIES)pValue)->flDecayTime, EAXLISTENER_MINDECAYTIME, EAXLISTENER_MAXDECAYTIME)) &&
								 (FloatInRange(((LPEAXLISTENERPROPERTIES)pValue)->flDecayHFRatio, EAXLISTENER_MINDECAYHFRATIO, EAXLISTENER_MAXDECAYHFRATIO)) &&
								 (LongInRange(((LPEAXLISTENERPROPERTIES)pValue)->lReflections, EAXLISTENER_MINREFLECTIONS, EAXLISTENER_MAXREFLECTIONS)) &&
								 (FloatInRange(((LPEAXLISTENERPROPERTIES)pValue)->flReflectionsDelay, EAXLISTENER_MINREFLECTIONSDELAY, EAXLISTENER_MAXREFLECTIONSDELAY)) &&
								 (LongInRange(((LPEAXLISTENERPROPERTIES)pValue)->lReverb, EAXLISTENER_MINREVERB, EAXLISTENER_MAXREVERB)) &&
								 (FloatInRange(((LPEAXLISTENERPROPERTIES)pValue)->flReverbDelay, EAXLISTENER_MINREVERBDELAY, EAXLISTENER_MAXREVERBDELAY)) &&
								 (ULongInRange(((LPEAXLISTENERPROPERTIES)pValue)->dwEnvironment, 0, EAX_ENVIRONMENT_COUNT)) &&
								 (FloatInRange(((LPEAXLISTENERPROPERTIES)pValue)->flEnvironmentSize, EAXLISTENER_MINENVIRONMENTSIZE, EAXLISTENER_MAXENVIRONMENTSIZE)) &&
								 (FloatInRange(((LPEAXLISTENERPROPERTIES)pValue)->flEnvironmentDiffusion, EAXLISTENER_MINENVIRONMENTDIFFUSION, EAXLISTENER_MAXENVIRONMENTDIFFUSION)) &&
								 (FloatInRange(((LPEAXLISTENERPROPERTIES)pValue)->flAirAbsorptionHF, EAXLISTENER_MINAIRABSORPTIONHF, EAXLISTENER_MAXAIRABSORPTIONHF)) &&
								 (ULongInRange(((LPEAXLISTENERPROPERTIES)pValue)->dwFlags, 0, ~EAXLISTENERFLAGS_RESERVED)) )
							{
								memcpy((void*)(&ALCContext->Listener.EAX20LP), pValue, sizeof(EAXLISTENERPROPERTIES));
								bSetValue = AL_TRUE;
							}
							else
							{
								ALErrorCode = AL_INVALID_OPERATION;
							}
						}
					}
					else
					{
						ALErrorCode = AL_INVALID_OPERATION;
					}
					break;

				case DSPROPERTY_EAXLISTENER_ROOM:
					if ((pValue) && (size >= sizeof(long)) && (LongInRange(*(long*)pValue, EAXLISTENER_MINROOM, EAXLISTENER_MAXROOM)))
					{
						bSetValue = (ALCContext->Listener.EAX20LP.lRoom != (*(long*)pValue));
						ALCContext->Listener.EAX20LP.lRoom = (*(long*)pValue);
					}
					else
					{
						ALErrorCode = AL_INVALID_OPERATION;
					}
					break;

				case DSPROPERTY_EAXLISTENER_ROOMHF:
					if ((pValue) && (size >= sizeof(long)) && (LongInRange(*(long*)pValue, EAXLISTENER_MINROOMHF, EAXLISTENER_MAXROOMHF)))
					{
						bSetValue = (ALCContext->Listener.EAX20LP.lRoomHF != (*(long*)pValue));
						ALCContext->Listener.EAX20LP.lRoomHF = (*(long*)pValue);
					}
					else
					{
						ALErrorCode = AL_INVALID_OPERATION;
					}
					break;

				case DSPROPERTY_EAXLISTENER_ROOMROLLOFFFACTOR:
					if ((pValue) && (size >= sizeof(float)) && (FloatInRange(*(float*)pValue, EAXLISTENER_MINROOMROLLOFFFACTOR, EAXLISTENER_MAXROOMROLLOFFFACTOR)))
					{
						bSetValue = (ALCContext->Listener.EAX20LP.flRoomRolloffFactor != (*(float*)pValue));
						ALCContext->Listener.EAX20LP.flRoomRolloffFactor = (*(float*)pValue);
					}
					else
					{
						ALErrorCode = AL_INVALID_OPERATION;
					}
					break;

				case DSPROPERTY_EAXLISTENER_DECAYTIME:
					if ((pValue) && (size >= sizeof(float)) && (FloatInRange(*(float*)pValue, EAXLISTENER_MINDECAYTIME, EAXLISTENER_MAXDECAYTIME)))
					{
						bSetValue = (ALCContext->Listener.EAX20LP.flDecayTime != (*(float*)pValue));
						ALCContext->Listener.EAX20LP.flDecayTime = (*(float*)pValue);
					}
					else
					{
						ALErrorCode = AL_INVALID_OPERATION;
					}
					break;

				case DSPROPERTY_EAXLISTENER_DECAYHFRATIO:
					if ((pValue) && (size >= sizeof(float)) && (FloatInRange(*(float*)pValue, EAXLISTENER_MINDECAYHFRATIO, EAXLISTENER_MAXDECAYHFRATIO)))
					{
						bSetValue = (ALCContext->Listener.EAX20LP.flDecayHFRatio != (*(float*)pValue));
						ALCContext->Listener.EAX20LP.flDecayHFRatio = (*(float*)pValue);
					}
					else
					{
						ALErrorCode = AL_INVALID_OPERATION;
					}
					break;

				case DSPROPERTY_EAXLISTENER_REFLECTIONS:
					if ((pValue) && (size >= sizeof(long)) && (LongInRange(*(long*)pValue, EAXLISTENER_MINREFLECTIONS, EAXLISTENER_MAXREFLECTIONS)))
					{
						bSetValue = (ALCContext->Listener.EAX20LP.lReflections != (*(long*)pValue));
						ALCContext->Listener.EAX20LP.lReflections = (*(long*)pValue);
					}
					else
					{
						ALErrorCode = AL_INVALID_OPERATION;
					}
					break;

				case DSPROPERTY_EAXLISTENER_REFLECTIONSDELAY:
					if ((pValue) && (size >= sizeof(float)) && (FloatInRange(*(float*)pValue, EAXLISTENER_MINREFLECTIONSDELAY, EAXLISTENER_MAXREFLECTIONSDELAY)))
					{
						bSetValue = (ALCContext->Listener.EAX20LP.flReflectionsDelay != (*(float*)pValue));
						ALCContext->Listener.EAX20LP.flReflectionsDelay = (*(float*)pValue);
					}
					else
					{
						ALErrorCode = AL_INVALID_OPERATION;
					}
					break;

				case DSPROPERTY_EAXLISTENER_REVERB:
					if ((pValue) && (size >= sizeof(long)) && (LongInRange(*(long*)pValue, EAXLISTENER_MINREVERB, EAXLISTENER_MAXREVERB)))
					{
						bSetValue = (ALCContext->Listener.EAX20LP.lReverb != (*(long*)pValue));
						ALCContext->Listener.EAX20LP.lReverb = (*(long*)pValue);
					}
					else
					{
						ALErrorCode = AL_INVALID_OPERATION;
					}
					break;

				case DSPROPERTY_EAXLISTENER_REVERBDELAY:
					if ((pValue) && (size >= sizeof(float)) && (FloatInRange(*(float*)pValue, EAXLISTENER_MINREVERBDELAY, EAXLISTENER_MAXREVERBDELAY)))
					{
						bSetValue = (ALCContext->Listener.EAX20LP.flReverbDelay != (*(float*)pValue));
						ALCContext->Listener.EAX20LP.flReverbDelay = (*(float*)pValue);
					}
					else
					{
						ALErrorCode = AL_INVALID_OPERATION;
					}
					break;

				case DSPROPERTY_EAXLISTENER_ENVIRONMENT:
					if ((pValue) && (size >= sizeof(unsigned long)) && (ULongInRange(*(unsigned long*)pValue, 0, EAX_ENVIRONMENT_COUNT)))
					{
						bGetValue = bSetValue = (ALCContext->Listener.EAX20LP.dwEnvironment != (*(unsigned long*)pValue));
						ALCContext->Listener.EAX20LP.dwEnvironment = (*(unsigned long*)pValue);
					}
					else
					{
						ALErrorCode = AL_INVALID_OPERATION;
					}
					break;

				case DSPROPERTY_EAXLISTENER_ENVIRONMENTSIZE:
					if ((pValue) && (size >= sizeof(float)) && (FloatInRange(*(float*)pValue, EAXLISTENER_MINENVIRONMENTSIZE, EAXLISTENER_MAXENVIRONMENTSIZE)))
					{
						bGetValue = bSetValue = (ALCContext->Listener.EAX20LP.flEnvironmentSize != (*(float*)pValue));
						ALCContext->Listener.EAX20LP.flEnvironmentSize = (*(float*)pValue);
					}
					else
					{
						ALErrorCode = AL_INVALID_OPERATION;
					}
					break;

				case DSPROPERTY_EAXLISTENER_ENVIRONMENTDIFFUSION:
					if ((pValue) && (size >= sizeof(float)) && (FloatInRange(*(float*)pValue, EAXLISTENER_MINENVIRONMENTDIFFUSION, EAXLISTENER_MAXENVIRONMENTDIFFUSION)))
					{
						bSetValue = (ALCContext->Listener.EAX20LP.flEnvironmentDiffusion != (*(float*)pValue));
						ALCContext->Listener.EAX20LP.flEnvironmentDiffusion = (*(float*)pValue);
					}
					else
					{
						ALErrorCode = AL_INVALID_OPERATION;
					}
					break;

				case DSPROPERTY_EAXLISTENER_AIRABSORPTIONHF:
					if ((pValue) && (size >= sizeof(float)) && (FloatInRange(*(float*)pValue, EAXLISTENER_MINAIRABSORPTIONHF, EAXLISTENER_MAXAIRABSORPTIONHF)))
					{
						bSetValue = (ALCContext->Listener.EAX20LP.flAirAbsorptionHF != (*(float*)pValue));
						ALCContext->Listener.EAX20LP.flAirAbsorptionHF = (*(float*)pValue);
					}
					else
					{
						ALErrorCode = AL_INVALID_OPERATION;
					}
					break;

				case DSPROPERTY_EAXLISTENER_FLAGS:
					if ((pValue) && (size >= sizeof(unsigned long)) && (ULongInRange(*(unsigned long*)pValue, 0, ~EAXLISTENERFLAGS_RESERVED)))
					{
						bSetValue = (ALCContext->Listener.EAX20LP.dwFlags != (*(unsigned long*)pValue));
						ALCContext->Listener.EAX20LP.dwFlags = (*(unsigned long*)pValue);
					}
					else
					{
						ALErrorCode = AL_INVALID_OPERATION;
					}
					break;

				default:
					ALErrorCode = AL_INVALID_OPERATION;
					break;
				}

				if (bSetValue)
				{
					if (FAILED(IKsPropertySet_Set((LPKSPROPERTYSET)ALSource->uservalue3, propertySetID, property, NULL, 0, pValue, size)))
						ALErrorCode = AL_INVALID_OPERATION;

					if (bGetValue)
						IKsPropertySet_Get((LPKSPROPERTYSET)ALSource->uservalue3, propertySetID, DSPROPERTY_EAXLISTENER_ALLPARAMETERS, NULL, 0,
							&ALCContext->Listener.EAX20LP, sizeof(ALCContext->Listener.EAX20LP), &ulBytesReturned);
				}
			}
		}
		else
			ALErrorCode = AL_INVALID_OPERATION;
	}

	// If we generated a source to set the EAX Listener property, release it now
	if (bGenSource)
		alDeleteSources(1, (ALuint *)&ALSource);
	
	ProcessContext(ALCContext);
	return ALErrorCode;
}

ALboolean LongInRange(long lValue, long lMin, long lMax)
{
	return ((lValue >= lMin) && (lValue <= lMax));
}

ALboolean ULongInRange(unsigned long ulValue, unsigned long ulMin, unsigned long ulMax)
{
	return ((ulValue >= ulMin) && (ulValue <= ulMax));
}

ALboolean FloatInRange(float flValue, float flMin, float flMax)
{
	return ((flValue >= flMin) && (flValue <= flMax));
}
