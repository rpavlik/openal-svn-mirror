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

#ifdef TARGET_CLASSIC
#include <Sound.h>
#else
#include <Carbon/Carbon.h>
#endif

#include "sm.h"
#include "globals.h" 
#include "alError.h"
#include "alSoftware.h"
#include "alSource.h"
#include "alBuffer.h"
 
#pragma export on 

ALAPI ALvoid ALAPIENTRY alGenSources(ALsizei n, ALuint *sources)
{
	int i=0;
	int iCount=0;
	
	if (n > 0)
	{
		// check if have enough sources available
		for (i = 1; i <= AL_MAXSOURCES; i++)
		{
			if (gSource[i].srcBufferNum == AL_MAXBUFFERS + 1) // found un-used source
			{
				iCount++;
			}
		}
	
		if (iCount >= n)
		{
			iCount = 0;
			// allocate sources where possible...
			for (i = 1; i <= AL_MAXSOURCES; i++)
			{
				if (gSource[i].srcBufferNum == AL_MAXBUFFERS + 1) // found un-used source
				{
		    		smSourceInit(i);
					gSource[i].srcBufferNum = 0;  // allocate the source
					gSource[i].state = AL_INITIAL;
					sources[iCount] = i;
					iCount++;
				}
				if (iCount >= n) break;
			}
		} else
		{
			alSetError(AL_INVALID_VALUE);
		}
	}
}

ALAPI ALvoid ALAPIENTRY alDeleteSources (ALsizei n, ALuint *sources)
{
	int i;
	QueueEntry *tempQPtr;
	int iCount=0;
	
	// check if it's even possible to delete the number of sources requested
	for (i = 1; i <= AL_MAXSOURCES; i++)
	{
		if (gSource[i].srcBufferNum != AL_MAXBUFFERS + 1)
		{
			iCount++;
		}
	}
	
	// clear source/channel information
	if (iCount >= n)
	{
		for (i= 0; i < n; i++)
		{
			if (alIsSource(sources[i]) == true)
			{
           	 	smSourceKill(sources[i]);
				gSource[sources[i]].srcBufferNum = AL_MAXBUFFERS + 1; // value > AL_MAXBUFFERS used as signal source is not being used
				gSource[sources[i]].readOffset = 0;
				gSource[sources[i]].writeOffset = 0;
				gSource[sources[i]].state = AL_INITIAL;
				gSource[sources[i]].srcRelative = AL_FALSE;
				gSource[sources[i]].looping = AL_FALSE;
				if (gSource[sources[i]].ptrQueue != NULL)
				{
					tempQPtr = gSource[sources[i]].ptrQueue;
					gSource[sources[i]].ptrQueue = tempQPtr->pNext;
					DisposePtr((char *)tempQPtr);
				}
				gSource[i].channelPtr = NULL;
				gSource[i].pitch = 1.0f;
				gSource[i].gain = 1.0f;
    			gSource[i].maxDistance = 100000000; // ***** should be MAX_FLOAT
    			gSource[i].minGain = 0.0f;
    			gSource[i].maxGain = 1.0f;
    			gSource[i].rolloffFactor = 1.0f;
    			gSource[i].referenceDistance = 1.0f;
				gSource[i].Position[0] = 0;
				gSource[i].Position[1] = 0;
				gSource[i].Position[2] = 0;
				gSource[i].Velocity[0] = 0;
				gSource[i].Velocity[1] = 0;
				gSource[i].Velocity[2] = 0;	
				gSource[i].ptrSndHeader = NULL;
			
				sources[i] = 0;
			}
		}
	} else
	{
		alSetError(AL_INVALID_VALUE);
	}
}

ALAPI ALboolean ALAPIENTRY alIsSource(ALuint source)
{
	if (source > AL_MAXSOURCES) return AL_FALSE; // can't be a source in this case...
	
	if (gSource[source].srcBufferNum <= AL_MAXBUFFERS)
	{
	 	return AL_TRUE;
	}
	
	return AL_FALSE;
}

ALAPI ALvoid ALAPIENTRY alSourcef (ALuint source, ALenum pname, ALfloat value)
{
	if (alIsSource(source))
	{
		switch(pname) 
		{
			case AL_PITCH:
				if (value>=0.0f)
				{	
					gSource[source].pitch = value;
				}
				else
					alSetError(AL_INVALID_VALUE);
				break;
			case AL_GAIN:
				gSource[source].gain = value;	
				break;
			case AL_MAX_DISTANCE:
				gSource[source].maxDistance = value;
				break;
			case AL_MIN_GAIN:
				if ((value >= 0.0f) && (value <= 1.0f))
				{
					gSource[source].minGain = value;
				} else
				{
					alSetError(AL_INVALID_VALUE);
				}
				break;
			case AL_MAX_GAIN:
				if ((value >= 0.0f) && (value <= 1.0f))
				{
					gSource[source].maxGain = value;
				} else
				{
					alSetError(AL_INVALID_VALUE);
				}
				break;
			case AL_ROLLOFF_FACTOR:
				if (value > 0.0f)
				{
					if (value <= 1.0f) // clamp to 1.0f because implementation breaks above 1.0f
					{
						gSource[source].rolloffFactor = value;
					}
				} else
				{
					alSetError(AL_INVALID_VALUE);
				}
				break;
			case AL_REFERENCE_DISTANCE:
				if (value > 0.0f)
				{
					gSource[source].referenceDistance = value;
				} else
				{
					alSetError(AL_INVALID_VALUE);
				}
				break;
			default:
				alSetError(AL_INVALID_OPERATION);
				break;
		}
	}
}

ALAPI ALvoid ALAPIENTRY alSourcefv (ALuint source, ALenum pname, ALfloat *values)
{
	switch(pname) 
	{
		case AL_POSITION:
			gSource[source].Position[0]=values[0];
			gSource[source].Position[1]=values[1];
			gSource[source].Position[2]=values[2];
			break;
		case AL_DIRECTION:
		    alSetError(AL_INVALID_ENUM); // cone functions not implemented yet
		    break;
		case AL_VELOCITY:
			gSource[source].Velocity[0]=values[0];
			gSource[source].Velocity[1]=values[1];
			gSource[source].Velocity[2]=values[2];
			break;
		default:
			alSetError(AL_INVALID_ENUM);
			break;
	}
}

ALAPI ALvoid ALAPIENTRY alSource3f (ALuint source, ALenum pname, ALfloat v1, ALfloat v2, ALfloat v3)
{
	switch(pname) 
	{
		case AL_POSITION:
			gSource[source].Position[0]=v1;
			gSource[source].Position[1]=v2;
			gSource[source].Position[2]=v3;
			break;
		case AL_VELOCITY:
			gSource[source].Velocity[0]=v1;
			gSource[source].Velocity[1]=v2;
			gSource[source].Velocity[2]=v3;
			break;
		default:
			alSetError(AL_INVALID_ENUM);
			break;
	}
}

ALAPI ALvoid ALAPIENTRY alSourcei (ALuint source, ALenum pname, ALint value)
{
	ALenum error;
	ALuint buffer;
	
	if (alIsSource(source))
	{
		switch(pname) 
		{
			case AL_LOOPING:
				gSource[source].looping = value;
				break;
			case AL_BUFFER:
				if ((gSource[source].state == AL_STOPPED) || (gSource[source].state == AL_INITIAL))
				{
					if ((alIsBuffer(value)) || (value == NULL))
					{
						gSource[source].srcBufferNum = value;
				
						// reset queue
						alGetError(); // clear error code
						error = AL_NO_ERROR;
						while (error == AL_NO_ERROR)
						{
							alSourceUnqueueBuffers(source, 1, &buffer);
							error = alGetError();
						}
					
					} else
					{
						alSetError(AL_INVALID_VALUE);
					}
				} else
				{
					alSetError(AL_INVALID_OPERATION);
				}
				break;
			case AL_SOURCE_RELATIVE:
				if ((value==AL_FALSE)||(value==AL_TRUE))
				{
					gSource[source].srcRelative = value;
				} else
				{
					alSetError(AL_INVALID_VALUE);
				}
				break;
			default:
				alSetError(AL_INVALID_ENUM);
				break;
	
		}
	}
}

ALAPI ALvoid ALAPIENTRY alGetSourcef (ALuint source, ALenum pname, ALfloat *value)
{
	if (alIsSource(source))
	{
		switch(pname) 
		{
			case AL_PITCH:
				*value = gSource[source].pitch;
				break;
			case AL_GAIN:
				*value = gSource[source].gain;
				break;
			case AL_MAX_DISTANCE:
				*value = gSource[source].maxDistance;
				break;
			case AL_MIN_GAIN:
				*value = gSource[source].minGain;
				break;
			case AL_MAX_GAIN:
				*value = gSource[source].maxGain;
				break;
			case AL_ROLLOFF_FACTOR:
				*value = gSource[source].rolloffFactor;
				break;
			case AL_REFERENCE_DISTANCE:
				*value = gSource[source].referenceDistance;
				break;
			default:
				alSetError(AL_INVALID_ENUM);
				break;
		}
	}	
}

ALAPI ALvoid ALAPIENTRY alGetSource3f (ALuint source, ALenum pname, ALfloat *v1, ALfloat *v2, ALfloat *v3)
{
	switch(pname) 
	{
		case AL_POSITION:
			*v1 = gSource[source].Position[0];
			*v2 = gSource[source].Position[1];
			*v3 = gSource[source].Position[2];
			break;
		case AL_DIRECTION:
		    alSetError(AL_INVALID_ENUM); // cone functions not implemented yet
		    break;
		case AL_VELOCITY:
			*v1 = gSource[source].Velocity[0];
			*v2 = gSource[source].Velocity[1];
			*v3 = gSource[source].Velocity[2];
			break;
		default:
			alSetError(AL_INVALID_ENUM);
			break;
	}
}

ALAPI ALvoid ALAPIENTRY alGetSourcefv (ALuint source, ALenum pname, ALfloat *values)
{
	if (alIsSource(source))
	{
		switch(pname) 
		{
			case AL_POSITION:
				values[0] = gSource[source].Position[0];
				values[1] = gSource[source].Position[1];
				values[2] = gSource[source].Position[2];
				break;
			case AL_VELOCITY:
				values[0] = gSource[source].Velocity[0];
				values[1] = gSource[source].Velocity[1];
				values[2] = gSource[source].Velocity[2];
				break;
			case AL_DIRECTION:
				alSetError(AL_INVALID_ENUM);
				break;
			default:
				alSetError(AL_INVALID_ENUM);
				break;
		}
	}
}

ALAPI ALvoid ALAPIENTRY alGetSourcei (ALuint source, ALenum pname, ALint *value)
{
    QueueEntry *pQE;
    ALint i;
    
	if (alIsSource(source))
	{
		switch(pname) 
		{
			case AL_CONE_INNER_ANGLE:
			case AL_CONE_OUTER_ANGLE:
				alSetError(AL_INVALID_ENUM); // not implemented yet
				break;
			case AL_LOOPING:
				*value=gSource[source].looping;
				break;
			case AL_BUFFER:
				*value=gSource[source].srcBufferNum;
				break;
                        case AL_SOURCE_RELATIVE:
                                *value=gSource[source].srcRelative;
				break;
			case AL_SOURCE_STATE:
			    *value=gSource[source].state;
			    break;
			case AL_BUFFERS_QUEUED:
			    pQE = gSource[source].ptrQueue;
			    i = 0;
			    while (pQE != NULL)
			    {
			    	i++;
			    	pQE = (QueueEntry *)pQE->pNext;
			    }
			    *value = i;
			    break;
			case AL_BUFFERS_PROCESSED:
			    i = 0;
			    
			    if (gSource[source].looping == AL_FALSE)
			    {
			    	pQE = gSource[source].ptrQueue;
			    	while (pQE != NULL)
			    	{
			    		if (pQE->processed == AL_TRUE)
			    		{
			    			i++;
			    		}
			    		pQE = (QueueEntry *)pQE->pNext;
			    	}
			    	if ((gSource[source].state == AL_PLAYING) && (i > 0))
			    	{
			    		i--;
			    	}
			    }
			    		    
			    *value = i;
			    break;
			default:
				alSetError(AL_INVALID_ENUM);
				break;
		}
	}
}

ALAPI ALvoid ALAPIENTRY alSourcePlay(ALuint source)
{
	QueueEntry *pQE;
	
    if (gSource[source].state != AL_PAUSED)
    {
        alSourceStop(source);    
    }	
    
	gSource[source].state = AL_PLAYING;
	smPlaySegment(source);
}

ALAPI ALvoid ALAPIENTRY alSourcePause (ALuint source)
{
	gSource[source].state = AL_PAUSED;
}

ALAPI ALvoid ALAPIENTRY alSourceStop (ALuint source)
{
	QueueEntry *pQE;
	
	smSourceFlushAndQuiet(source);
	
	gSource[source].state = AL_STOPPED;
	gSource[source].readOffset = 0;
	
	pQE = gSource[source].ptrQueue; // reset all processed flags
	while (pQE != NULL)
	{
	    gSource[source].srcBufferNum = 0; // will play the null-buffer, then process queue...
	 	pQE->processed = AL_FALSE;
	 	pQE = pQE->pNext;
	}
}

ALAPI ALvoid ALAPIENTRY alSourceRewind (ALuint source)
{
    alSourceStop(source);
    gSource[source].state = AL_INITIAL;
    gSource[source].readOffset = 0; 
}

ALAPI ALvoid ALAPIENTRY alSourcePlayv(ALsizei n, ALuint *ID)
{
	if (n > 0)
	{
		while (n--)
		{
			alSourcePlay(ID[n]);
		}
	}
}

ALAPI ALvoid ALAPIENTRY alSourcePausev(ALsizei n, ALuint *ID)
{
	if (n > 0)
	{
		while (n--)
		{
			alSourcePause(ID[n]);
		}
	}
}

ALAPI ALvoid ALAPIENTRY alSourceStopv(ALsizei n, ALuint *ID)
{
	if (n > 0)
	{
		while (n--)
		{
			alSourceStop(ID[n]);
		}
	}
}

ALAPI ALvoid ALAPIENTRY alSourceRewindv (ALsizei n, ALuint *ID)
{
    if (n > 0)
    {
        while (n--)
        {
            alSourceRewind(ID[n]);
        }
    }
}

ALAPI ALvoid ALAPIENTRY alQueuei (ALuint source, ALenum param, ALint value)
{
	QueueEntry *ptrQE, *tempPtr;
	
	if (alIsSource(source))
	{
		switch(param) 
		{
			case AL_BUFFER:
				ptrQE = (void *)NewPtrClear(sizeof(QueueEntry));
				ptrQE->bufferNum = value;
				ptrQE->processed = AL_FALSE;
				ptrQE->pitch = gSource[source].pitch;
				ptrQE->gain = gSource[source].gain;
				ptrQE->loopDirection = AL_FALSE; // ***** need to implement real loop directions
				
				tempPtr = gSource[source].ptrQueue;
				if (tempPtr != NULL)
				{
					while (tempPtr->pNext != NULL)
					{
						tempPtr = tempPtr->pNext;
					}
					tempPtr->pNext = ptrQE;
				} else
				{
					gSource[source].ptrQueue = ptrQE;
				}
				break;
			default:
				alSetError(AL_INVALID_ENUM);
				break;
	
		}
	} else
	{
		alSetError(AL_INVALID_NAME);
	}
}

ALAPI ALvoid ALAPIENTRY alQueuef (ALuint source, ALenum param, ALfloat value)
{
	if (alIsSource(source))
	{
		switch(param) 
		{
			case AL_PITCH:
				if ((value>=0.5f)&&(value<=2.0f))
				{	
					// ***** gSource[source].pitch = value;
				}
				else
					alSetError(AL_INVALID_VALUE);
				break;
			case AL_GAIN:
				if (value <= 1.0f)
				{
					// ***** smSetSourceVolume (source, (value * kFullVolume), (value * kFullVolume));
				}	
				break;
			default:
				alSetError(AL_INVALID_OPERATION);
				break;
		}
	} else
	{
		alSetError(AL_INVALID_NAME);
	}
}

ALAPI ALvoid ALAPIENTRY alSourceQueueBuffers (ALuint source, ALsizei n, ALuint *buffers)
{
    int i;
    
	if (alIsSource(source))
	{
		for (i = 0; i < n; i++)
		{
			if ((alIsBuffer(buffers[i])) || (buffers[i] == NULL))
			{
				alQueuei(source, AL_BUFFER, buffers[i]);
			}
		}
	}
}

ALAPI ALvoid ALAPIENTRY alSourceUnqueueBuffers (ALuint source, ALsizei n, ALuint *buffers)
{
 	ALsizei i, count;
 	ALint srcState;
	QueueEntry *tempQEPtr;
	
	if (alIsSource(source))
	{
		alGetSourcei(source, AL_SOURCE_STATE, &srcState);
		
		count = 0;
		
		// test to see if all queue entries can be deleted
		tempQEPtr = gSource[source].ptrQueue;
		for (i = 0; i < n; i++)
		{		
			// allow buffer to be deleted if it is processed and not being played
			if (tempQEPtr != NULL)
			{
				if (((tempQEPtr->processed == AL_TRUE) && (tempQEPtr->bufferNum != gSource[source].srcBufferNum)) || (srcState != AL_PLAYING))
				{
					count++;	
				}
				tempQEPtr = tempQEPtr->pNext;
			}
		}
		
		// have n deletable entries, so do it...
		if (count == n)
		{
			for (i = 0; i < n; i++)
			{		
				tempQEPtr = gSource[source].ptrQueue;
				buffers[i] = tempQEPtr->bufferNum;
				gSource[source].ptrQueue = tempQEPtr->pNext;
				DisposePtr((char *)tempQEPtr);
			}
			if (srcState != AL_PLAYING)
			{
				alSourceStop(source); // if not playing, then reset source
				gSource[source].srcBufferNum = 0; // don't leave behind an already-played buffer ptr
			}
		} else
		{
			alSetError(AL_INVALID_VALUE);
		}
	} else
	{
		alSetError(AL_INVALID_NAME);
	}			
}

#pragma export off

