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

#include <stdlib.h>
#include "globals.h"
#include "alSource.h"
#include "alSoftware.h"
#include "sm.h"

#ifdef VORBIS_EXTENSION
#include "vorbisrtn.h"
#endif
 
 // Sound Manager functions
void smPlaySegment(unsigned int source)
{
	OSErr Err;
	SndCommand SCommand;
	int iBufferNum;
	ALfloat Pitch, Panning, Volume;
	QueueEntry *pQE;

	iBufferNum = gSource[source].srcBufferNum;
		
	if ((gSource[source].channelPtr != NULL) && (gBuffer[iBufferNum].size > 0)) // have a sound channel and data, so play...
	{	
		
		gSource[source].state = AL_PLAYING;
		
		// reset pitch and volume as needed for 3D environment
		alCalculateSourceParameters(source,&Pitch,&Panning,&Volume);
		smSetSourcePitch(source, Pitch);
		if (gBuffer[iBufferNum].channels == 1) // make sure this is a mono buffer before positioning it
		{
			if (Volume > 1.0f) Volume = 1.0f;
			smSetSourceVolume(source, (kFullVolume * Volume * ((Panning < 0.5) ? (1): (2.0 - Panning * 2.0))), 
			     (kFullVolume * Volume * ((Panning < 0.5) ? Panning * 2.0 : (1))));
		} else
		{
			Volume = (gSource[source].gain*gListener.Gain);
			if (Volume > 1.0f) Volume = 1.0f;
			smSetSourceVolume(source, (kFullVolume * Volume), (kFullVolume * Volume));
		}
		// create sound header
                if (gSource[source].readOffset > gBuffer[iBufferNum].size) { 
                    gSource[source].readOffset = 0;
                    if (gSource[source].pCompHdr != NULL) {
                        DisposePtr(gSource[source].pCompHdr);
                        gSource[source].pCompHdr = NULL;
                    } 
                }
                if (gBuffer[iBufferNum].format == AL_FORMAT_VORBIS_EXT) { // compressed format handling
                    ov_fillBuffer(source, iBufferNum);
                    gSource[source].ptrSndHeader->samplePtr = (char *) gBuffer[iBufferNum].uncompressedData;
                    gSource[source].ptrSndHeader->numFrames = gBuffer[iBufferNum].uncompressedSize;
                } else // uncompressed buffer format
                {
                    gSource[source].ptrSndHeader->samplePtr = (char *) gBuffer[iBufferNum].data + gSource[source].readOffset;
                    if ((gSource[source].readOffset + gBufferSize) <= gBuffer[iBufferNum].size)
                    {
                            gSource[source].ptrSndHeader->numFrames = gBufferSize;
                    } else
                    {
                            gSource[source].ptrSndHeader->numFrames = (gBuffer[iBufferNum].size - gSource[source].readOffset);
                    }
                }
                if (gBuffer[iBufferNum].bits == 16) { // adjust number of frames for 16-bit width
                    gSource[source].ptrSndHeader->numFrames = gSource[source].ptrSndHeader->numFrames / 2; 
                }
                gSource[source].ptrSndHeader->numFrames =gSource[source].ptrSndHeader->numFrames / gBuffer[iBufferNum].channels; // adjust number of frames for number of channels
                gSource[source].ptrSndHeader->sampleSize = gBuffer[iBufferNum].bits;
                gSource[source].ptrSndHeader->numChannels = gBuffer[iBufferNum].channels;
                switch (gBuffer[iBufferNum].frequency)
                {
                        case 11025:	gSource[source].ptrSndHeader->sampleRate = rate11khz;
                                        break;
                        case 22050: gSource[source].ptrSndHeader->sampleRate = rate22khz;
                                        break;
                        case 44100: gSource[source].ptrSndHeader->sampleRate = rate44khz;
                                    break;
                        default: gSource[source].ptrSndHeader->sampleRate = rate44khz;
                }
                gSource[source].ptrSndHeader->loopStart = 0;
                gSource[source].ptrSndHeader->loopEnd = 0;
                gSource[source].ptrSndHeader->encode = extSH;
                gSource[source].ptrSndHeader->baseFrequency = 60;
                gSource[source].ptrSndHeader->markerChunk = NULL;
                gSource[source].ptrSndHeader->instrumentChunks = NULL;
                gSource[source].ptrSndHeader->AESRecording = NULL;
				
		// create buffer command
		SCommand.cmd = bufferCmd;
		SCommand.param1 = NULL;
		SCommand.param2 = (long) gSource[source].ptrSndHeader;
				
		// send buffer command
		Err = SndDoCommand (gSource[source].channelPtr, &SCommand, true);
				
		// create callback command
		SCommand.cmd = callBackCmd;
		SCommand.param1 = NULL;
		SCommand.param2 = source;
				
		// send callback command
		Err = SndDoCommand (gSource[source].channelPtr, &SCommand, true);
	} else // evaluate if should start processing queue...
	{
	    // find first un-processed queue (or don't find any to process)
		pQE = gSource[source].ptrQueue;
		if (pQE != NULL)
		{
			if (pQE->bufferNum == NULL)
			{
				pQE->processed = AL_TRUE;  // if first queue entry is null buffer, mark as processed and move on...
			}
			while (pQE->processed == AL_TRUE)
			{
				pQE = pQE->pNext;
				if (pQE == NULL) break;
				if (pQE->bufferNum == NULL)
				{
					pQE->processed = AL_TRUE; // if there are null buffers in the queue, mark as processed and move to next un-processed non-null buffer...
				}
			}
		}
		
		// if there's a queue to process, do it...
		if (pQE != NULL)
		{
			pQE->processed = AL_TRUE;
			gSource[source].srcBufferNum = pQE->bufferNum;
			smPlaySegment(source);
		} else
		{
			gSource[source].state = AL_STOPPED;
		}
	}
}

pascal void smService (SndChannelPtr chan, SndCommand* acmd)
{
	ALuint source;
	QueueEntry *pQE;
	ALboolean bNewQE;
        
	source = (ALuint) acmd->param2;
	
	bNewQE = AL_FALSE;
	
	// evaluate whether or not the buffer has been over-run
        if (gBuffer[gSource[source].srcBufferNum].format != AL_FORMAT_VORBIS_EXT) {
            gSource[source].readOffset += gBufferSize;
        }
	if (gSource[source].readOffset >= gBuffer[gSource[source].srcBufferNum].size)
	{
	    // check if there is a new queue to go to -- if not, then reset queue processed flags, decrement loop counter, and restart
            pQE = gSource[source].ptrQueue;
            if (pQE != NULL) {
                while (pQE->processed == AL_TRUE) {
                    pQE = pQE->pNext;
                    if (pQE == NULL) break;
                }
            }
	    
	    if (pQE != NULL) // process next queued buffer
	    {
	    	pQE->processed = AL_TRUE;
	    	gSource[source].srcBufferNum = pQE->bufferNum;
	    	gSource[source].readOffset = 0;
                if (gSource[source].pCompHdr != NULL) {
                    DisposePtr(gSource[source].pCompHdr);
                    gSource[source].pCompHdr = NULL;
                }
	    	bNewQE = AL_TRUE;
	    } else // completed all buffers, so reset buffer processed flags and decrement loop counter
	    {
	        // reset all processed flags if looping
	        if (gSource[source].looping == AL_TRUE)
	        {
	    		pQE = gSource[source].ptrQueue;
	    		while (pQE != NULL)
	    		{
	    			pQE->processed = AL_FALSE;
	    			pQE = pQE->pNext;
	    		}
	    	}
	    	
	    	pQE = gSource[source].ptrQueue; // if there is a queue, stage first buffer
	    	if (pQE != NULL)
	    	{
	    		gSource[source].srcBufferNum = pQE->bufferNum;
	    		gSource[source].readOffset = 0;
                        if (gSource[source].pCompHdr != NULL) {
                            DisposePtr(gSource[source].pCompHdr);
                            gSource[source].pCompHdr = NULL;
                        }
	    		pQE->processed = AL_TRUE;
	    	}
                
                // if looping is on and have uncompressed all compressed data, then reset source but keep it in a playing state
                if ((gSource[source].looping == AL_TRUE) && (gBuffer[gSource[source].srcBufferNum].uncompressedSize == 0)) {
                    gSource[source].readOffset = 0;
                    if (gSource[source].pCompHdr != NULL) {
                        DisposePtr(gSource[source].pCompHdr);
                        gSource[source].pCompHdr = NULL;
                    } 
                }
	    	
                // if looping is off and have uncompressed all compressed data, then set the source's state to STOPPED
                if ((gSource[source].looping != AL_TRUE) && (gBuffer[gSource[source].srcBufferNum].uncompressedSize == 0)) {
                    gSource[source].state = AL_STOPPED; 
                }
            }
	}
	
	// if now stopped, reset read pointer
	if (gSource[source].state == AL_STOPPED)
	{
		gSource[source].readOffset = 0;
                if (gSource[source].pCompHdr != NULL) {
                    DisposePtr(gSource[source].pCompHdr);
                    gSource[source].pCompHdr = NULL;
                }
	}
	
	// evaluate if more data needs to be played -- if so then call smPlaySegment
	if (
		(gSource[source].state == AL_PLAYING) &&
	    (((gSource[source].readOffset == 0) && (gSource[source].looping == AL_TRUE)) || (gSource[source].readOffset != 0) || (bNewQE == AL_TRUE))
	   )
	{
            smPlaySegment(source);
	}
}

void smSetSourceVolume (int source, int rVol, int lVol)
{
	SndCommand VolCommand;
	OSErr Err;
	
	// create volume command
	VolCommand.cmd = volumeCmd;
	VolCommand.param1 = NULL;
	VolCommand.param2 = (rVol << 16) + lVol;
				
	// send volume command
	Err = SndDoImmediate (gSource[source].channelPtr, &VolCommand);
}

void smSetSourcePitch(int source, float value)
{
	SndCommand SoundCommand;
	OSErr Err;
	long Rate;
	
	SoundCommand.cmd = rateMultiplierCmd;
	SoundCommand.param1 = NULL;
	SoundCommand.param2 = 0x00010000 * value;
				
	Err = SndDoImmediate (gSource[source].channelPtr, &SoundCommand);	
}

void smSourceInit(unsigned int source)
{
 	OSErr Err;
 	
    if (gSource[source].channelPtr != NULL)
    {
    	smSourceKill(source);
    } else
    {
		Err = SndNewChannel(&gSource[source].channelPtr, sampledSynth, initMono, gpSMRtn); // create sound channel
		gSource[source].readOffset = 0;
                if (gSource[source].pCompHdr != NULL) {
                    DisposePtr(gSource[source].pCompHdr);
                    gSource[source].pCompHdr = NULL;
                }
		if (gSource[source].ptrSndHeader == NULL)
		{
			gSource[source].ptrSndHeader = malloc(sizeof(ExtSoundHeader));
		}
	}
}

void smSourceFlushAndQuiet(unsigned int source)
{
	OSErr Err;
	SndCommand SCommand;
	
	// flush
	SCommand.cmd = flushCmd;
	SCommand.param1 = 0;
	SCommand.param2 = 0;
	
	Err = SndDoImmediate(gSource[source].channelPtr, &SCommand);
	
	// quiet
	SCommand.cmd = quietCmd;
	SCommand.param1 = 0;
	SCommand.param2 = 0;
	
	Err = SndDoImmediate(gSource[source].channelPtr, &SCommand);
}

void smSourceKill(unsigned int source)
{
	OSErr Err;
	
	smSourceFlushAndQuiet(source);
	
	// kill off source's channel and reset data
	if (gSource[source].channelPtr != NULL)
	{
		SndDisposeChannel(gSource[source].channelPtr, true);
		gSource[source].channelPtr = NULL;
		gSource[source].srcBufferNum = AL_MAXBUFFERS + 1;
		if (gSource[source].ptrSndHeader != NULL)
		{
			free(gSource[source].ptrSndHeader);
			gSource[source].ptrSndHeader = NULL;
		}
	}
}

