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
#include "include\alMain.h"
#include "al\alc.h"
#include "include\alError.h"
#include "include\alBuffer.h"

static ALbuffer *Buffer=NULL;
static ALuint BufferCount=0;

ALAPI ALvoid ALAPIENTRY alGenBuffers(ALsizei n,ALuint *buffers)
{
	ALCcontext *Context;
	ALbuffer *ALBuf;
	ALsizei i=0;
	
	// Request to generate 0 Buffers is a valid NOP
	if (n == 0)
		return;

	Context = alcGetCurrentContext();
	SuspendContext(Context);

	// Check that enough memory has been allocted in the 'buffers' array for n buffers
	if (IsBadWritePtr((void*)buffers, n * sizeof(ALuint)))
	{
		alSetError(AL_INVALID_VALUE);
		ProcessContext(Context);
		return;
	}

	if (!Buffer)
	{
		Buffer=malloc(sizeof(ALbuffer));
		if (Buffer)
		{
			memset(Buffer,0,sizeof(ALbuffer));
			buffers[i]=(ALuint)Buffer;
			Buffer->state=UNUSED;
			BufferCount++;
			i++;
		}
		ALBuf=Buffer;
	}
	else
	{
		ALBuf=Buffer;
		while (ALBuf->next)
			ALBuf=ALBuf->next;
	}
	
	while ((ALBuf)&&(i<n))
	{
		ALBuf->next=malloc(sizeof(ALbuffer));
		if (ALBuf->next)
		{
			memset(ALBuf->next,0,sizeof(ALbuffer));
			buffers[i]=(ALuint)ALBuf->next;
			ALBuf->next->previous=ALBuf;
			ALBuf->next->state=UNUSED;
			BufferCount++;
			i++;
			ALBuf=ALBuf->next;
		}
		else
		{
			// Out of memory
			break;
		}
	}

	if (i!=n)
		alSetError(AL_OUT_OF_MEMORY);

	ProcessContext(Context);
}

ALAPI ALvoid ALAPIENTRY alDeleteBuffers(ALsizei n,ALuint *buffers)
{
	ALCcontext *Context;
	ALbuffer *ALBuf;
	ALsizei i;

	// NOP
	if (n == 0)
		return;

	Context = alcGetCurrentContext();
	SuspendContext(Context);

	if (n > BufferCount)
	{
		alSetError(AL_INVALID_NAME);
		ProcessContext(Context);
		return;
	}

	// Check that all the buffers are valid and can actually be deleted
	for (i=0;i<n;i++)
	{
		// Check for valid Buffer ID or NULL buffer
		if ((!alIsBuffer(buffers[i]))&&(buffers[i]!=0))
		{
			alSetError(AL_INVALID_NAME);
			ProcessContext(Context);
			return;
		}
		else
		{
			// If not the NULL buffer, check that the reference count is 0
			ALBuf=((ALbuffer *)buffers[i]);
			if (ALBuf)
			{
				if (ALBuf->refcount != 0)
				{
					// Buffer still in use, cannot be deleted
					alSetError(AL_INVALID_OPERATION);
					ProcessContext(Context);
					return;
				}
			}
		}
	}

	for (i=0;i<n;i++)
	{
		ALBuf=((ALbuffer *)buffers[i]);
		if (ALBuf)
		{
			if (ALBuf->previous)
				ALBuf->previous->next=ALBuf->next;
			else
				Buffer=ALBuf->next;
			if (ALBuf->next)
				ALBuf->next->previous=ALBuf->previous;

			// Release the memory used to store audio data
			if (ALBuf->data)
				free(ALBuf->data);

			// Release buffer structure
			memset(ALBuf,0,sizeof(ALbuffer));
			BufferCount--;
			free(ALBuf);
		}
	}

	ProcessContext(Context);
}

ALAPI ALboolean ALAPIENTRY alIsBuffer(ALuint buffer)
{
	ALCcontext *Context;
	ALboolean result=AL_FALSE;
	ALbuffer *ALBuf;
	unsigned int i;
	
	Context = alcGetCurrentContext();
	SuspendContext(Context);

	ALBuf = Buffer;
	for (i = 0; i < BufferCount; i++)
	{
		if ((ALuint)ALBuf == buffer)
		{
			result = AL_TRUE;
			break;
		}

		ALBuf = ALBuf->next;
	}

	ProcessContext(Context);
	return result;
}


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
		if ((ALBuf->state==UNUSED)&&(data))
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
		else alSetError(AL_INVALID_VALUE);
	} 
	else alSetError(AL_INVALID_OPERATION);

	ProcessContext(Context);
}


ALAPI ALvoid ALAPIENTRY alGetBufferf(ALuint buffer,ALenum pname,ALfloat *value)
{
	ALCcontext *Context;
	ALbuffer *ALBuf;

	Context = alcGetCurrentContext();
	SuspendContext(Context);

	if (!value)
	{
		alSetError(AL_INVALID_VALUE);
		ProcessContext(Context);
		return;
	}

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
		alSetError(AL_INVALID_NAME);

	ProcessContext(Context);
}


ALAPI ALvoid ALAPIENTRY alGetBufferi(ALuint buffer,ALenum pname,ALint *value)
{
	ALCcontext *Context;
	ALbuffer *ALBuf;

	Context = alcGetCurrentContext();
	SuspendContext(Context);

	if (!value)
	{
		alSetError(AL_INVALID_VALUE);
		ProcessContext(Context);
		return;
	}
		
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
		alSetError(AL_INVALID_NAME);

	ProcessContext(Context);
}


/*
	ReleaseALBuffers()

	Called by DLLMain on exit to destroy any buffers that still exist
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
	if (BufferCount > 0)
	{
		sprintf(szString, "OpenAL32 : DllMain() %d Buffer(s) NOT deleted\n", BufferCount);
		OutputDebugString(szString);
	}
#endif

	ALBuffer = Buffer;
	for (i = 0; i < BufferCount; i++)
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