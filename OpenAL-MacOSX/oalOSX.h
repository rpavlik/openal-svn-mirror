/**********************************************************************************************************************************
*
*   OpenAL cross platform audio library
*   Copyright (c) 2004, Apple Computer, Inc. All rights reserved.
*
*   Redistribution and use in source and binary forms, with or without modification, are permitted provided 
*   that the following conditions are met:
*
*   1.  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. 
*   2.  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
*       disclaimer in the documentation and/or other materials provided with the distribution. 
*   3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of its contributors may be used to endorse or promote 
*       products derived from this software without specific prior written permission. 
*
*   THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
*   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS 
*   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
*   TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
*   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE 
*   USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
**********************************************************************************************************************************/

#ifndef __OAL_OSX__
#define __OAL_OSX__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Carbon/Carbon.h>
#include <AudioToolbox/AudioToolbox.h>
#include "CAStreamBasicDescription.h"
#include "oalImp.h"
#include "MacOSX_OALExtensions.h"

#define maxLen 256


bool		IsFormatSupported(UInt32	inFormatID);
OSStatus	FillInASBD(CAStreamBasicDescription &inASBD, UInt32	inFormatID, UInt32 inSampleRate);
bool		IsValidRenderQuality (UInt32 inRenderQuality);
UInt32		GetDesiredRenderChannelsFor3DMixer(UInt32	inDeviceChannels);
void		GetDefaultDeviceName(ALCchar*		outDeviceName, bool	isInput);
uintptr_t	GetNewPtrToken (void);
ALuint		GetNewToken (void);
UInt32		CalculateNeededMixerBusses(const ALCint *attrList, UInt32 inDefaultBusCount);
const char* GetFormatString(UInt32 inToken);
const char* GetALAttributeString(UInt32 inToken);
const char* GetALCAttributeString(UInt32 inToken);
void		WaitOneRenderCycle();
void		ReconfigureContextsOfThisDevice(uintptr_t inDeviceToken);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// OALRingBuffer:
// This class implements an audio ring buffer. Multi-channel data can be either 
// interleaved or deinterleaved.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

enum {
		kOALRingBufferError_WayBehind		= -2,	// both fetch times are earlier than buffer start time
		kOALRingBufferError_SlightlyBehind	= -1,	// fetch start time is earlier than buffer start time (fetch end time OK)
		kOALRingBufferError_OK				= 0,
		kOALRingBufferError_SlightlyAhead	= 1,	// fetch end time is later than buffer end time (fetch start time OK)
		kOALRingBufferError_WayAhead		= 2,	// both fetch times are later than buffer end time
		kOALRingBufferError_TooMuch			= 3,	// fetch start time is earlier than buffer start time and fetch end time is later than buffer end time
		kOALRingBufferError_CPUOverload		= 4		// the reader is unable to get enough CPU cycles to capture a consistent snapshot of the time bounds
};

const UInt32 kTimeBoundsQueueSize = 32;
const UInt32 kTimeBoundsQueueMask = kTimeBoundsQueueSize - 1;

class OALRingBuffer {
public:
	typedef SInt64 SampleTime;

	OALRingBuffer();
	~OALRingBuffer();
	
	void		Allocate(int nChannels, UInt32 bytesPerFrame, UInt32 capacityFrames);
					// capacityFrames will be rounded up to a power of 2
	void		Deallocate();
	
	OSStatus	Store(const AudioBufferList *abl, UInt32 nFrames, SInt64 frameNumber);
					// Copy nFrames of data into the ring buffer at the specified sample time.
					// The sample time should normally increase sequentially, though gaps
					// are filled with zeroes. A sufficiently large gap effectively empties
					// the buffer before storing the new data. 
							
					// If frameNumber is less than the previous frame number, the behavior is undefined.
							
					// Return false for failure (buffer not large enough).
				
	OSStatus	Fetch(AudioBufferList *abl, UInt32 nFrames, SInt64 frameNumber);
								// will alter mNumDataBytes of the buffers
	
	OSStatus	GetTimeBounds(SInt64 &startTime, SInt64 &endTime);
	
protected:

	int			FrameOffset(SInt64 frameNumber) { return (frameNumber & mCapacityFramesMask) * mBytesPerFrame; }

	OSStatus	CheckTimeBounds(SInt64 startRead, SInt64 endRead);
	
	// these should only be called from Store.
	SInt64				StartTime() const { return mTimeBoundsQueue[mTimeBoundsQueuePtr & kTimeBoundsQueueMask].mStartTime; }
	SInt64				EndTime()   const { return mTimeBoundsQueue[mTimeBoundsQueuePtr & kTimeBoundsQueueMask].mEndTime; }
	void					SetTimeBounds(SInt64 startTime, SInt64 endTime);
	
protected:
	Byte **		mBuffers;				// allocated in one chunk of memory
	int			mNumberChannels;
	UInt32		mBytesPerFrame;			// within one deinterleaved channel
	UInt32		mCapacityFrames;		// per channel, must be a power of 2
	UInt32		mCapacityFramesMask;
	UInt32		mCapacityBytes;			// per channel
	
	// range of valid sample time in the buffer
	typedef struct {
		volatile SInt64		mStartTime;
		volatile SInt64		mEndTime;
		volatile UInt32		mUpdateCounter;
	} TimeBounds;
	
	OALRingBuffer::TimeBounds mTimeBoundsQueue[kTimeBoundsQueueSize];
	UInt32 mTimeBoundsQueuePtr;
};


#endif
