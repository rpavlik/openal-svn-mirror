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
#include <stdio.h>
#include "Include/alMain.h"
#include "AL/al.h"
#include "AL/alc.h"
#include "Include/alError.h"
#include "Include/alBuffer.h"

/*
*	AL Buffer Functions
*
*	AL Buffers are shared amoung Contexts, so we store the list of generated Buffers
*	as a global variable in this module.   (A valid context is not required to make
*	AL Buffer function calls
*
*/

/*
* Global Variables
*/

static ALbuffer *	g_pBuffers = NULL;			// Linked List of Buffers
static ALuint		g_uiBufferCount = 0;		// Buffer Count

/*
*	alGenBuffers(ALsizei n, ALuint *puiBuffers)
*
*	Generates n AL Buffers, and stores the Buffers Names in the array pointed to by puiBuffers
*/
ALAPI ALvoid ALAPIENTRY alGenBuffers(ALsizei n,ALuint *puiBuffers)
{
	ALCcontext *Context;
	ALbuffer *ALBuf;
	ALsizei i=0;

	Context = alcGetCurrentContext();
	SuspendContext(Context);

	// Check that we are actually generation some Buffers
	if (n > 0)
	{
		// Check the pointer is valid (and points to enough memory to store Buffer Names)
		if (!IsBadWritePtr((void*)puiBuffers, n * sizeof(ALuint)))
		{
			// Is this the first Buffer created
			if (!g_pBuffers)
			{
				g_pBuffers = malloc(sizeof(ALbuffer));
				if (g_pBuffers)
				{
					memset(g_pBuffers, 0, sizeof(ALbuffer));
					puiBuffers[i]=(ALuint)g_pBuffers;
					g_pBuffers->state=UNUSED;
					g_uiBufferCount++;
					i++;
				}
				ALBuf = g_pBuffers;
			}
			else
			{
				// Find last Buffer in list
				ALBuf = g_pBuffers;
				while (ALBuf->next)
					ALBuf=ALBuf->next;
			}
			
			// Create all the new Buffers
			while ((ALBuf)&&(i<n))
			{
				ALBuf->next = malloc(sizeof(ALbuffer));
				if (ALBuf->next)
				{
					memset(ALBuf->next, 0, sizeof(ALbuffer));
					puiBuffers[i] = (ALuint)ALBuf->next;
					ALBuf->next->previous = ALBuf;
					ALBuf->next->state = UNUSED;
					g_uiBufferCount++;
					i++;
					ALBuf = ALBuf->next;
				}
				else
				{
					// Out of memory
					break;
				}
			}

			// If we didn't create all the Buffers, we must have run out of memory
			if (i != n)
			{
				alSetError(AL_OUT_OF_MEMORY);
			}
		}
		else
		{
			// Pointer does not point to enough memory to write Buffer names
			alSetError(AL_INVALID_VALUE);
		}
	}

	ProcessContext(Context);

	return;
}

/*
*	alDeleteBuffers(ALsizei n, ALuint *puiBuffers)
*
*	Deletes the n AL Buffers pointed to by puiBuffers
*/
ALAPI ALvoid ALAPIENTRY alDeleteBuffers(ALsizei n,ALuint *puiBuffers)
{
	ALCcontext *Context;
	ALbuffer *ALBuf;
	ALsizei i;
	ALboolean bFailed = AL_FALSE;

	Context = alcGetCurrentContext();
	SuspendContext(Context);

	// Check we are actually Deleting some Buffers
	if (n > 0)
	{
		if (n <= g_uiBufferCount)
		{
			// Check that all the buffers are valid and can actually be deleted
			for (i = 0; i < n; i++)
			{
				// Check for valid Buffer ID or NULL buffer
				if ((alIsBuffer(puiBuffers[i])) || (puiBuffers[i] == 0))
				{
					// If not the NULL buffer, check that the reference count is 0
					ALBuf = ((ALbuffer *)puiBuffers[i]);
					if (ALBuf)
					{
						if (ALBuf->refcount != 0)
						{
							// Buffer still in use, cannot be deleted
							alSetError(AL_INVALID_OPERATION);
							bFailed = AL_TRUE;
						}
					}
				}
				else
				{
					// Invalid Buffer
					alSetError(AL_INVALID_NAME);
					bFailed = AL_TRUE;
				}
			}

			// If all the Buffers were valid (and have Reference Counts of 0), then we can delete them
			if (!bFailed)
			{
				for (i = 0; i < n; i++)
				{
					ALBuf=((ALbuffer *)puiBuffers[i]);
					if (ALBuf)
					{
						if (ALBuf->previous)
							ALBuf->previous->next=ALBuf->next;
						else
							g_pBuffers = ALBuf->next;

						if (ALBuf->next)
							ALBuf->next->previous = ALBuf->previous;

						// Release the memory used to store audio data
						if (ALBuf->data)
							free(ALBuf->data);

						// Release buffer structure
						memset(ALBuf, 0, sizeof(ALbuffer));
						g_uiBufferCount--;
						free(ALBuf);
					}
				}
			}
		}
		else
		{
			alSetError(AL_INVALID_NAME);
		}
	}

	ProcessContext(Context);

	return;

}

/*
*	alIsBuffer(ALuint uiBuffer)
*
*	Checks if ulBuffer is a valid Buffer Name
*/
ALAPI ALboolean ALAPIENTRY alIsBuffer(ALuint uiBuffer)
{
	ALCcontext *Context;
	ALboolean result=AL_FALSE;
	ALbuffer *ALBuf;
	unsigned int i;
	
	Context = alcGetCurrentContext();
	SuspendContext(Context);

	// Check through list of generated buffers for uiBuffer
	ALBuf = g_pBuffers;
	for (i = 0; i < g_uiBufferCount; i++)
	{
		if ((ALuint)ALBuf == uiBuffer)
		{
			result = AL_TRUE;
			break;
		}

		ALBuf = ALBuf->next;
	}

	ProcessContext(Context);

	return result;
}

/*
*	alBufferData(ALuint buffer,ALenum format,ALvoid *data,ALsizei size,ALsizei freq)
*
*	Fill buffer with audio data
*/
ALAPI ALvoid ALAPIENTRY alBufferData(ALuint buffer,ALenum format,ALvoid *data,ALsizei size,ALsizei freq)
{
	ALCcontext *Context;
	ALbuffer *ALBuf;
	ALsizei i;
	
	Context = alcGetCurrentContext();
	SuspendContext(Context);

	if (alIsBuffer(buffer))
	{
		ALBuf=((ALbuffer *)buffer);
//		if ((ALBuf->state==UNUSED)&&(data))
		if ((ALBuf->refcount==0)&&(data))
		{
			switch(format)
			{
				case AL_FORMAT_MONO8:
					ALBuf->data=realloc(ALBuf->data,1+size/sizeof(ALubyte)*sizeof(ALshort));
					if (ALBuf->data)
					{
						ALBuf->format=AL_FORMAT_MONO16;
						for (i=0;i<size/sizeof(ALubyte);i++) 
							ALBuf->data[i]=(ALshort)((((ALubyte *)data)[i]-128)<<8);
						ALBuf->size=size/sizeof(ALubyte)*sizeof(ALshort);
						ALBuf->frequency=freq;
					}
					else
						alSetError(AL_OUT_OF_MEMORY);
					break;

				case AL_FORMAT_MONO16:
					ALBuf->data=realloc(ALBuf->data,2+size/sizeof(ALshort)*sizeof(ALshort));
					if (ALBuf->data)
					{
						ALBuf->format=AL_FORMAT_MONO16;
						memcpy(ALBuf->data,data,size/sizeof(ALshort)*sizeof(ALshort));
						ALBuf->size=size/sizeof(ALshort)*sizeof(ALshort);
						ALBuf->frequency=freq;
					}
					else
						alSetError(AL_OUT_OF_MEMORY);
					break;

				case AL_FORMAT_STEREO8:
					ALBuf->data=realloc(ALBuf->data,2+size/sizeof(ALubyte)*sizeof(ALshort));
					if (ALBuf->data)
					{
						ALBuf->format=AL_FORMAT_STEREO16;
						for (i=0;i<size/sizeof(ALubyte);i++) 
							ALBuf->data[i]=(ALshort)((((ALubyte *)data)[i]-128)<<8);
						ALBuf->size=size/sizeof(ALubyte)*sizeof(ALshort);
						ALBuf->frequency=freq;
					}
					else
						alSetError(AL_OUT_OF_MEMORY);
					break;

				case AL_FORMAT_STEREO16:
					ALBuf->data=realloc(ALBuf->data,4+size/sizeof(ALshort)*sizeof(ALshort));
					if (ALBuf->data)
					{
						ALBuf->format=AL_FORMAT_STEREO16;
						memcpy(ALBuf->data,data,size/sizeof(ALshort)*sizeof(ALshort));
						ALBuf->size=size/sizeof(ALshort)*sizeof(ALshort);
						ALBuf->frequency=freq;
					}
					else
						alSetError(AL_OUT_OF_MEMORY);
					break;

				default:
					alSetError(AL_INVALID_VALUE);
					break;
			}
		}
		else
		{
			// Buffer is in use, or data is a NULL pointer
			alSetError(AL_INVALID_VALUE);
		}
	} 
	else
	{
		// Invalid Buffer Name
		alSetError(AL_INVALID_NAME);
	}

	ProcessContext(Context);
}

/*
*	alGetBufferf(ALuint buffer,ALenum pname,ALfloat *value)
*
*	Query buffer for floating point attributes (current none defined)
*/
ALAPI ALvoid ALAPIENTRY alGetBufferf(ALuint buffer,ALenum pname,ALfloat *value)
{
	ALCcontext *Context;
	ALbuffer *ALBuf;

	Context = alcGetCurrentContext();
	SuspendContext(Context);

	if (value)
	{
		if (alIsBuffer(buffer))
		{
			ALBuf=((ALbuffer *)buffer);
			switch(pname)
			{
				default:
					alSetError(AL_INVALID_ENUM);
					break;
			}
		}
		else
		{
			// Invalid Buffer Name
			alSetError(AL_INVALID_NAME);
		}
	}
	else
	{
		// value is a NULL pointer
		alSetError(AL_INVALID_VALUE);
	}

	ProcessContext(Context);
}

/*
*	alGetBufferi(ALuint buffer,ALenum pname,ALint *value)
*
*	Query buffer for integer attributes
*/
ALAPI ALvoid ALAPIENTRY alGetBufferi(ALuint buffer,ALenum pname,ALint *value)
{
	ALCcontext *Context;
	ALbuffer *ALBuf;

	Context = alcGetCurrentContext();
	SuspendContext(Context);

	if (value)
	{
		if (alIsBuffer(buffer))
		{
			ALBuf=((ALbuffer *)buffer);
			switch(pname)
			{
				case AL_FREQUENCY:
					*value=ALBuf->frequency;
					break;

				case AL_BITS:
					*value=(((ALBuf->format==AL_FORMAT_MONO8)||(ALBuf->format==AL_FORMAT_STEREO8))?8:16);
					break;

				case AL_CHANNELS:
					*value=(((ALBuf->format==AL_FORMAT_MONO8)||(ALBuf->format==AL_FORMAT_MONO16))?1:2);
					break;

				case AL_SIZE:
					*value=ALBuf->size;
					break;

				default:
					alSetError(AL_INVALID_ENUM);
					break;
			}
		} 
		else
		{
			// Invalid Buffer Name
			alSetError(AL_INVALID_NAME);
		}
	}
	else
	{
		// value is a NULL pointer
		alSetError(AL_INVALID_VALUE);
	}

	ProcessContext(Context);
}


/*
*	ReleaseALBuffers()
*
*	INTERNAL FN : Called by DLLMain on exit to destroy any buffers that still exist
*/
ALvoid ReleaseALBuffers(ALvoid)
{
	ALbuffer *ALBuffer;
	ALbuffer *ALBufferTemp;
	unsigned int i;
#ifdef _DEBUG
	char szString[256];
#endif

#ifdef _DEBUG
	if (g_uiBufferCount > 0)
	{
		// In Debug Mode only - write out number of AL Buffers not destroyed
		sprintf(szString, "OpenAL32 : DllMain() %d Buffer(s) NOT deleted\n", g_uiBufferCount);
		OutputDebugString(szString);
	}
#endif

	ALBuffer = g_pBuffers;
	for (i = 0; i < g_uiBufferCount; i++)
	{
		// Release sample data
		if (ALBuffer->data)
			free(ALBuffer->data);
		
		// Release Buffer structure
		ALBufferTemp = ALBuffer;
		ALBuffer = ALBuffer->next;
		memset(ALBufferTemp, 0, sizeof(ALbuffer));
		free(ALBufferTemp);
	}
}