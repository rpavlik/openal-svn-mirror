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
#include "alError.h"
#include "alBuffer.h"

#ifdef MAC_OS_X
#include <stdlib.h>
#include <string.h>
#endif

#pragma export on 
 
// AL_BUFFER functions
ALAPI ALvoid ALAPIENTRY alGenBuffers(ALsizei n, ALuint *buffers)
{
	int i;
	int iCount=0;
	
	// check if have enough buffers available
	for (i = 1; i <= AL_MAXBUFFERS; i++)
	{
		if (gBuffer[i].data == NULL)
		{
			iCount++;
		}
	}
	
	if (iCount >= n)
	{
		iCount = 0;
		for (i = 1; i <= AL_MAXBUFFERS; i++)
		{
			if (gBuffer[i].data == NULL) // found un-used internal buffer to use
			{
#ifdef MAC_OS_X
                                gBuffer[i].data = (void *) malloc(1024);  // allocate the buffer
                                memset(gBuffer[i].data, 0, 1024);
#else
				gBuffer[i].data = (void *) NewPtrClear(1024);  // allocate the buffer
#endif
				gBuffer[i].size = 1024;
				gBuffer[i].bits = 8;
				gBuffer[i].channels = 1;
				buffers[iCount] = i;
				iCount++;
			}
			if (iCount >= n) break;
		}
	} else
	{
		alSetError(AL_INVALID_VALUE);
	}
}

ALAPI ALvoid ALAPIENTRY alDeleteBuffers(ALsizei n, ALuint *buffers)
{
	int i=0,j=0;
	int iCount=0;
	ALboolean bAttached = AL_FALSE;
	
	// check if it's even possible to delete the number of buffers requested
	for (i = 1; i <= AL_MAXBUFFERS; i++)
	{
		if (gBuffer[i].data != NULL)
		{
			iCount++;
		}
	}
	
	if (iCount >= n)
	{ 
		// make sure none of the buffers are attached to a source
		for (i = 0; i < n; i++)
		{
			for (j = 1; j <= AL_MAXSOURCES; j++)
			{
				if (gSource[j].srcBufferNum == buffers[i])
				{
					bAttached = AL_TRUE;
					break;
				}
			}
			if (bAttached == AL_TRUE) 
			{
				break;
			}
		}
	
		// passed all tests, so do the deletion...
		if (bAttached == AL_FALSE)
		{
			for (i = 0; i < n; i++)
			{
				if ((alIsBuffer(buffers[i]) == AL_TRUE) && (buffers[i] != 0))
				{
                                        if (gBuffer[buffers[i]].data != 0) {
#ifdef MAC_OS_X
                                            free(gBuffer[buffers[i]].data);
#else
                                            DisposePtr((char *)gBuffer[buffers[i]].data); // get rid of memory used by buffer
#endif
                                            gBuffer[buffers[i]].data = NULL;
                                        }
	 				gBuffer[buffers[i]].data = NULL;
	 				gBuffer[buffers[i]].size = 0;
	 				gBuffer[buffers[i]].bits = 8;
	 				gBuffer[buffers[i]].channels = 1;
	 				buffers[i] = 0;
	 			}
			}
		}
	}
	
	// set error code if appropriate
	if ((iCount < n) || (bAttached == AL_TRUE))
	{
		alSetError(AL_INVALID_VALUE);
	}
}

ALAPI ALboolean ALAPIENTRY alIsBuffer(ALuint buffer)
{
	if (buffer > AL_MAXBUFFERS) return AL_FALSE; // can't be a buffer in this case...
	if (gBuffer[buffer].size == 0) return AL_FALSE;  // otherwise should have some memory allocated to it...
	
	return AL_TRUE;
}

ALAPI ALvoid ALAPIENTRY alBufferData(ALuint buffer,ALenum format,ALvoid *data,ALsizei size,ALsizei freq)
{
	if (alIsBuffer(buffer) == AL_TRUE)
	{
#ifdef MAC_OS_X
                if (gBuffer[buffer].data != NULL) {
                    free(gBuffer[buffer].data);
                    gBuffer[buffer].data = NULL;
                }
                
                if (gBuffer[buffer].data == NULL) {
                    gBuffer[buffer].data = (void *)malloc(size);
                    memset(gBuffer[buffer].data, 0, size);
                }
#else
		DisposePtr((char *) gBuffer[buffer].data);	
		gBuffer[buffer].data = (void *) NewPtrClear(size); // size is bytes for this example
#endif
                
		if (gBuffer[buffer].data != NULL)
		{
#ifdef MAC_OS_X
                        memcpy(gBuffer[buffer].data, data, size);
#else
			BlockMove((char *) data, gBuffer[buffer].data, size);
#endif
			gBuffer[buffer].size = size;
			gBuffer[buffer].frequency = freq;
                        gBuffer[buffer].format = format;
			switch (format)
			{
		    	case AL_FORMAT_STEREO8:
		    		gBuffer[buffer].channels = 2;
		    		gBuffer[buffer].bits = 8;
		    		break;
				case AL_FORMAT_MONO8 : 
					gBuffer[buffer].channels = 1;
		    		gBuffer[buffer].bits = 8;
		    		break;
		    	case AL_FORMAT_STEREO16:
		    		gBuffer[buffer].channels = 2;
		    		gBuffer[buffer].bits = 16;
		    		break;
                        case AL_FORMAT_MONO16: 
		    		gBuffer[buffer].channels = 1;
		    		gBuffer[buffer].bits = 16;
		    		break;
		    	default: 
		    		gBuffer[buffer].channels = 1;
		    		gBuffer[buffer].bits = 8;
			}
		}
	} else 
	{
		alSetError(AL_INVALID_NAME);
	}
}

ALAPI ALvoid ALAPIENTRY alGetBufferf (ALuint buffer, ALenum pname, ALfloat *value)
{
	if (alIsBuffer(buffer))
	{
		switch(pname)
		{
			case AL_FREQUENCY:
				*value = (float) gBuffer[buffer].frequency;
				break;
			default:
				alSetError(AL_INVALID_ENUM);
		}
	}
}

ALAPI ALvoid ALAPIENTRY alGetBufferi(ALuint buffer, ALenum pname, ALint *value)
{
	if (alIsBuffer(buffer))
	{
		switch(pname)
		{
			case AL_FREQUENCY:
				*value=gBuffer[buffer].frequency;
				break;
			case AL_BITS:
				*value=gBuffer[buffer].bits;
				break;
			case AL_CHANNELS:
				*value=gBuffer[buffer].channels;
				break;
			case AL_SIZE:
				*value=gBuffer[buffer].size;
				break;
			default:
				alSetError(AL_INVALID_ENUM);
		}
	}
}

#pragma export off
