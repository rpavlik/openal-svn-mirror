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

#ifndef TARGET_CLASSIC
#define OPAQUE_UPP_TYPES 1
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

#include "globaltypes.h"
#include "alMain.h"

#ifdef MAC_OS_X
#include "sm_ca.h"
#else
#include "sm.h"
#endif

ALbuffer gBuffer[AL_MAXBUFFERS + 1];  // global buffers (0 is null buffer)
ALsource gSource[AL_MAXSOURCES + 1];  // global sources (0 not used)
ALlistener gListener; // global listenters
ALuint gBufferSize;
extern ALfloat gDopplerFactor;
extern ALfloat gDopplerVelocity;
extern ALfloat gDistanceScale;
extern ALfloat gPropagationSpeed;
extern ALenum gDistanceModel;

#ifndef MAC_OS_X
SndCallBackUPP gpSMRtn;  // global pointer to Sound Manager callback routine
extern pascal void smService (SndChannelPtr chan, SndCommand* acmd); // external sound manager service routine (sm.c)
#endif

#pragma export on 

// AL_MAIN functions
ALAPI ALvoid ALAPIENTRY alInit(ALint *argc, ALubyte **argv)
{
	int i;
	
	// set internal buffer size
#ifndef MAC_OS_X
	SoundComponentData	outputFormat;
	OSErr error = GetSoundOutputInfo( NULL, siHardwareFormat, &outputFormat );
	if (error != 0) // error -- use default
	{
#endif
		gBufferSize = AL_DEFAULT_INTERNAL_BUFFERS_SIZE;
#ifndef MAC_OS_X
	} else // use 2 x hardware sample count
	{
		gBufferSize = outputFormat.sampleCount * 2;
	}
#endif
	
	// set global scale variables
	gDopplerFactor = 1.0f;
	gDopplerVelocity = 1.0f;
	gDistanceScale = 1.0f;
	gPropagationSpeed = 1.0f;
	
	// set global distance model
	gDistanceModel = AL_INVERSE_DISTANCE;
	
	// clear buffer information
	for (i = 0; i < AL_MAXBUFFERS; i++)
	{
		gBuffer[i].data = NULL;
		gBuffer[i].size = 0;
		gBuffer[i].frequency = 44100;
	}
	
	// clear source/channel information
	for (i= 0; i < AL_MAXSOURCES; i++)
	{
		
		gSource[i].srcBufferNum = AL_MAXBUFFERS + 1; // value > AL_MAXBUFFERS used as signal that no buffer is attached to this source
		gSource[i].readOffset = 0;
                gSource[i].uncompressedReadOffset = 0;
		gSource[i].writeOffset = 0;
		gSource[i].state = AL_INITIAL;
		gSource[i].srcRelative = AL_FALSE;
		gSource[i].looping = AL_FALSE;
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
		gSource[i].ptrQueue = NULL;
                gSource[i].uncompressedData = NULL;
                gSource[i].uncompressedSize = 0;
#ifdef MAC_OS_X
                gSource[i].uncompressedBufferOffset = 0;
                gSource[i].uncompressedSize = 0;
#else
                gSource[i].ptrSndHeader = NULL;
                gSource[i].channelPtr = NULL;
#endif
	}
	
	// clear listener info
	gListener.Position[0] = 0;
	gListener.Position[1] = 0;
	gListener.Position[2] = 0;
	gListener.Velocity[0] = 0;
	gListener.Velocity[1] = 0;
	gListener.Velocity[2] = 0;
	gListener.Forward[0] = 0;
	gListener.Forward[1] = 1;
	gListener.Forward[2] = 0;
	gListener.Up[0] = 0;
	gListener.Up[1] = 0;
	gListener.Up[2] = 1 ;
	gListener.Gain = 1.0f;
	gListener.Environment = 0;

#ifndef MAC_OS_X	
#ifndef TARGET_CLASSIC
	gpSMRtn = NewSndCallBackUPP(smService); // pointer to the Sound Manager callback routine
#else
	gpSMRtn = NewSndCallBackProc(smService); // pointer to the Sound Manager callback routine
#endif
#endif
}

ALAPI ALvoid ALAPIENTRY alExit(ALvoid)
{
	int i;
	
	// dispose of buffers
	for (i = 0; i < AL_MAXBUFFERS; i++)
	{
		if (gBuffer[i].data != NULL)
		{
                        if (gBuffer[i].data != NULL) {
#ifdef MAC_OS_X
                            free(gBuffer[i].data);
#else	
                            DisposePtr((char *) gBuffer[i].data);
#endif
                        }
			gBuffer[i].data = NULL;
			gBuffer[i].size = 0;
		}
	}
	
	// dispose of source/channel information
	for (i= 0; i <= AL_MAXSOURCES; i++)
	{
                if (gSource[i].uncompressedData != NULL) {
#ifdef MAC_OS_X
                    free(gSource[i].uncompressedData);
#else
                    DisposePtr((char *) gSource[i].uncompressedData);
#endif
                }
                gSource[i].uncompressedData = NULL;
                gSource[i].uncompressedSize = 0;
		smSourceKill(i);
	}

#ifndef MAC_OS_X
#ifndef TARGET_CLASSIC
	DisposeSndCallBackUPP(gpSMRtn); // dispose of pointer to Sound Manager callback routine
#else
	DisposeRoutineDescriptor(gpSMRtn); // dispose of pointer to Sound Manager callback routine
#endif
#endif
}

#pragma export off
