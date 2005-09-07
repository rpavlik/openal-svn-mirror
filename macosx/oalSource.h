/**********************************************************************************************************************************
*
*   OpenAL cross platform audio library
*   Copyright © 2004, Apple Computer, Inc. All rights reserved.
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

#ifndef __OAL_SOURCE__
#define __OAL_SOURCE__

#include "oalImp.h"
#include "oalDevice.h"
#include "oalBuffer.h"
#include "oalContext.h"
#include "altypes.h"

#include <Carbon/Carbon.h>
#include <AudioToolbox/AudioConverter.h>
#include <list>
#include "CAStreamBasicDescription.h"
#include "CAGuard.h"
#include "CAMutex.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// buffer info state constants
enum {
		kPendingProcessing = 0,
		kInProgress = 1,
		kProcessed = 2
};

enum {
		kResetQueueOff			= 0,
		kResetQueueRampDown		= 1,
		kResetQueueJoinLists	= 2
};

enum {
		kRampDown		= -1,
		kNoRamping		= 0,
		kRampUp			= 1
};

#define	OALSourceError_CallConverterAgain	'agan'
#define	kSourceNeedsBus                     -1

// do not change kDistanceScalar from 10.0 - it is used to compensate for a reverb related problem in the 3DMixer
#define kDistanceScalar                     10.0

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Source - Buffer Queue
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark _____BufferQueue_____
// This struct is used by OAL source to store info about the buffers in it's queue
struct	BufferInfo {
							UInt32		mBufferToken;					// the buffer name that the user was returned when the buffer was created
							OALBuffer	*mBuffer;						// buffer object
							UInt32		mOffset;						// current read position offset of this data
							UInt32		mProcessedState;				// mark as true when data of this buffer is finished playing and looping is off
							UInt32		mACToken;						// use this AC from the ACMap for converting the data to the mixer format, when
																		// set to zero, then NO AC is needed, the data has already been converted
};

class BufferQueue : std::list<BufferInfo> {
public:
    
    BufferInfo*     Get(short		index)  {
        iterator	it = begin();        
        std::advance(it, index);
        if (it != end())
            return(&(*it));
        return (NULL);
    }

	void 	AppendBuffer(UInt32	inBufferToken, OALBuffer	*inBuffer, UInt32	inACToken) {
				
		BufferInfo	newBuffer;
		
		inBuffer->mAttachedCount++;
		
		newBuffer.mBufferToken = inBufferToken;
		newBuffer.mBuffer = inBuffer;
		newBuffer.mOffset = 0;
		newBuffer.mProcessedState = kPendingProcessing;
		newBuffer.mACToken = inACToken;

		push_back(value_type (newBuffer));
	}

	bool IsBufferProcessed(UInt32	inIndex) {
		iterator	it = begin();
		bool		returnValue = false;
		
        std::advance(it, inIndex);		
		if (it != end())
			returnValue = (it->mProcessedState == kProcessed) ? true : false;
		return (returnValue);
	}

	void SetBufferAsProcessed(UInt32	inIndex) {
		iterator	it = begin();
        std::advance(it, inIndex);		
		if (it != end())
			it->mProcessedState = kProcessed;
	}

	UInt32	GetBufferTokenByIndex(UInt32 inIndex) {
		iterator	it = begin();
        std::advance(it, inIndex);		
		if (it != end())
			return (it->mBufferToken);
		return (0);
	}

	// return the bufferToken for the buffer removed from the queue
	UInt32 	RemoveQueueEntryByIndex(UInt32	inIndex) {
        iterator	it = begin();
        UInt32		outBufferToken = 0;

        std::advance(it, inIndex);				
        if (it != end())
		{
			outBufferToken = it->mBufferToken;
			it->mBuffer->mAttachedCount--;
			erase(it);
		}
		
		return (outBufferToken);
	}

	// mark all the buffers in the queue as unprocessed and offset 0
	void 	ResetBuffers() {
        iterator	it = begin();
        while (it != end())
		{
			it->mProcessedState = kPendingProcessing;
			it->mOffset = 0;
			it++;
		}
	}

	// count the buffers that have been played
	UInt32 	ProcessedBufferCount() {
        iterator	it = begin();
		UInt32		count = 0;
        
        while (it != end())
		{
			if (it->mProcessedState == kProcessed)
			{
				count++;
			}
			it++;
		}        
        return (count);
	}
	    
    UInt32 Size () const { return size(); }
    bool Empty () const { return empty(); }
};
typedef	BufferQueue BufferQueue;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ACMap - map the AudioConverters for the sources queue
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark _____ACMap_____
// This struct is used by OAL source to store info about the ACs used by it's queue
struct	ACInfo {
					AudioConverterRef				mConverter;
					CAStreamBasicDescription		mInputFormat; 
};

class ACMap : std::multimap<UInt32, ACInfo, std::less<UInt32> > {
public:
    
    // add a new context to the map
    void Add (const	UInt32	inACToken, ACInfo *inACInfo)  {
		iterator it = upper_bound(inACToken);
		insert(it, value_type (inACToken, *inACInfo));
	}

    AudioConverterRef Get(UInt32	inACToken) {
        iterator	it = find(inACToken);
        iterator	theEnd = end();

        if (it != theEnd)
            return ((*it).second.mConverter);
		
		return (NULL);
    }
	
	void GetACForFormat (CAStreamBasicDescription	&inFormat, UInt32	&outToken) {
        iterator	it = begin();
        
		outToken = 0; // 0 means there is none yet
        while (it != end())
		{
			if(	(*it).second.mInputFormat == inFormat)
			{
				outToken = (*it).first;
				it = end();
			}
			else
				it++;
		}
		return;
	}
    
    void Remove (const	UInt32	inACToken) {
        iterator 	it = find(inACToken);
        if (it !=  end()) {
			AudioConverterDispose((*it).second.mConverter);
            erase(it);
		}
    }

    // the map should be disposed after making this call
    void RemoveAllConverters () {
        iterator 	it = begin();		
        iterator	theEnd = end();
       
		while (it !=  theEnd) 
		{
			AudioConverterDispose((*it).second.mConverter);
			it++;
		}
    }

    UInt32 Size () const { return size(); }
    bool Empty () const { return empty(); }
};
typedef	ACMap ACMap;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// OALSources
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define kBufferQueueMutex   "BUFFER_Q"
#define kStateMutex         "SOURCE_STATE"

#pragma mark _____OALSource_____
class OALSource
{
#pragma mark __________ PRIVATE __________
	private:

		UInt32						mSelfToken;					// the token returned to the caller upon alGenSources()
		OALContext                  *mOwningContext;
        OALDevice					*mOwningDevice;             // the OALDevice object for the owning OpenAL device
		
		bool						mCalculateDistance;		
		bool						mResetBusFormat;
		bool						mResetPitch;

		BufferQueue					*mBufferQueueActive;        // map of buffers for queueing
		BufferQueue					*mBufferQueueInactive;      // map of buffers already queued
        CAMutex                     mBufferQueueMutex;
		UInt32						mCurrentBufferIndex;		// token for curent buffer being played
		bool						mQueueIsProcessed;			// state of the entire buffer queue
		
		int							mRenderThreadID;
		CAGuard						mPlayGuard;
		AURenderCallbackStruct 		mPlayCallback;
		int							mCurrentPlayBus;			// the mixer bus currently used by this source
		
		ACMap						*mACMap;

		bool						mOutputSilence;
		Float32						mPosition[3];
		Float32						mVelocity[3];
		Float32						mDirection[3];
		UInt32						mLooping;
		UInt32						mSourceRelative;
		Float32						mConeInnerAngle;
		Float32						mConeOuterAngle;
		Float32						mConeOuterGain;
         
         // support for pre 2.0 3DMixer
        float                       mReadIndex;                    
        float                       mCachedInputL1;                 
        float                       mCachedInputL2;               
        float                       mCachedInputR1;                
        float                       mCachedInputR2;                
        UInt32                      mTempSourceStorageBufferSize;   
        AudioBufferList             *mTempSourceStorage;

		UInt32						mState;						// playback state: Playing, Stopped, Paused, Initial
		float						mGain;
        Float32						mPitch;
        Float32						mDopplerScaler;
        Float32						mRollOffFactor;
		Float32						mReferenceDistance;
		Float32						mMaxDistance;
		Float32						mMinDistance;
		Float32						mMinGain;
		Float32						mMaxGain;
		
		SInt32						mRampState;
		bool						mStopInPostRender;
		bool						mPauseInPostRender;
		UInt32						mResetQInPostRender;		// state for resetting the buffer queue while a source is playing
        		
	void        JoinBufferLists();
	void        LoopToBeginning();
	void        ChangeChannelSettings();
	void		InitSource();
	void		SetState(UInt32		inState);
	OSStatus 	DoPreRender ();
	OSStatus 	DoRender (AudioBufferList 	*ioData);
	OSStatus 	DoSRCRender (AudioBufferList 	*ioData);           // support for pre 2.0 3DMixer
	OSStatus 	DoPostRender ();
	void 		CalculateDistanceAndAzimuth(Float32 *outDistance, Float32 *outAzimuth, Float32 *outElevation, Float32	*outDopplerShift);
	bool        CallingInRenderThread () const { return (int(pthread_self()) == mRenderThreadID); }
	void        UpdateBusGain ();
	void        UpdateBusFormat ();
	void        Reset();

	void		UpdateQueue ();
	void		RampDown (AudioBufferList 			*ioData);
	void		RampUp (AudioBufferList 			*ioData);
	void		ClearActiveQueue();
	void		SkipALNONEBuffers();
	void		AddNotifyAndRenderProcs();
	void		ReleaseNotifyAndRenderProcs();
	void		DisconnectFromBus();

	OSStatus	MuteCurrentPlayBus () const
	{
		return AudioUnitSetParameter (	mOwningDevice->GetMixerUnit(), k3DMixerParam_Gain, kAudioUnitScope_Input,
                                        mCurrentPlayBus, 0.0, 0);
	}

	static	OSStatus	SourceNotificationProc (void                        *inRefCon, 
												AudioUnitRenderActionFlags 	*inActionFlags,
												const AudioTimeStamp 		*inTimeStamp, 
												UInt32 						inBusNumber,
												UInt32 						inNumberFrames, 
												AudioBufferList 			*ioData);

	static	OSStatus	SourceInputProc (	void                        *inRefCon, 
											AudioUnitRenderActionFlags 	*inActionFlags,
											const AudioTimeStamp 		*inTimeStamp, 
											UInt32 						inBusNumber,
											UInt32 						inNumberFrames, 
											AudioBufferList 			*ioData);


	static OSStatus     ACComplexInputDataProc	(	AudioConverterRef				inAudioConverter,
                                                    UInt32							*ioNumberDataPackets,
                                                    AudioBufferList					*ioData,
                                                    AudioStreamPacketDescription	**outDataPacketDescription,
                                                    void*							inUserData);

#pragma mark __________ PUBLIC __________
	public:
	OALSource(const UInt32 	 	inSelfToken, OALContext	*inOwningContext);
	~OALSource();
	
	// set info methods - these may be called from either the render thread or the API caller
	void	SetPitch (Float32	inPitch);
	void	SetGain (Float32	inGain);
	void	SetMinGain (Float32	inMinGain);
	void	SetMaxGain (Float32	inMaxGain);
	void	SetReferenceDistance (Float32	inReferenceDistance);
	void	SetMaxDistance (Float32	inMaxDistance);
	void	SetRollOffFactor (Float32	inRollOffFactor);
	void	SetLooping (UInt32	inLooping);
	void	SetPosition (Float32 inX, Float32 inY, Float32 inZ);
	void	SetVelocity (Float32 inX, Float32 inY, Float32 inZ);
	void	SetDirection (Float32 inX, Float32 inY, Float32 inZ);
	void	SetSourceRelative (UInt32	inSourceRelative);
	void	SetChannelParameters ();
	void	SetConeInnerAngle (Float32	inConeInnerAngle);
	void	SetConeOuterAngle (Float32	inConeOuterAngle);
	void	SetConeOuterGain (Float32	inConeOuterGain);

	// get info methods
	Float32	GetPitch ();
	Float32	GetDopplerScaler ();
	Float32	GetGain ();
	Float32	GetMinGain ();
	Float32	GetMaxGain ();
	Float32	GetReferenceDistance ();
	Float32	GetMaxDistance ();
	Float32	GetRollOffFactor ();
	UInt32	GetLooping ();
	void	GetPosition (Float32 &inX, Float32 &inY, Float32 &inZ);
	void	GetVelocity (Float32 &inX, Float32 &inY, Float32 &inZ);
	void	GetDirection (Float32 &inX, Float32 &inY, Float32 &inZ);
	UInt32	GetSourceRelative ();
	Float32	GetConeInnerAngle ();
	Float32	GetConeOuterAngle ();
	Float32	GetConeOuterGain ();
	UInt32	GetState();
    UInt32	GetToken();

    // buffer queue
	UInt32	GetQLength();
	UInt32	GetBuffersProcessed();
	void	SetBuffer (UInt32	inBuffer, OALBuffer	*inBuffer);
	UInt32	GetBuffer ();
	void	AppendBufferToQueue(UInt32	inBufferToken, OALBuffer	*inBuffer);
	void	RemoveBuffersFromQueue(UInt32	inCount, UInt32	*outBufferTokens);
	
	// playback methods
	void	Play();
	void	Pause();
	void	Resume();
	void	Stop();
	void	RestartQueue();			// to be called from alSourcePlay while the source is in the AL_PLAYING state
};	

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark _____OALSourceMap_____
class OALSourceMap : std::multimap<UInt32, OALSource*, std::less<UInt32> > {
public:
    
    // add a new context to the map
    void Add (const	UInt32	inSourceToken, OALSource **inSource)  {
		iterator it = upper_bound(inSourceToken);
		insert(it, value_type (inSourceToken, *inSource));
	}

    OALSource* GetSourceByIndex(UInt32	inIndex) {
        iterator	it = begin();

		for (UInt32 i = 0; i < inIndex; i++)
        {
            if (it != end())
                it++;
            else
                i = inIndex;
        }
        
        if (it != end())
            return ((*it).second);		
		return (NULL);
    }

    OALSource* Get(UInt32	inSourceToken) {
        iterator	it = find(inSourceToken);
        if (it != end())
            return ((*it).second);
		return (NULL);
    }

    void MarkAllSourcesForRecalculation() {
        iterator	it = begin();
        while (it != end())
		{
			(*it).second->SetChannelParameters();
			it++;
		}
		return;
    }

    void SetDopplerForAllSources() {
        MarkAllSourcesForRecalculation();
		return;
    }
    
    void StopAllSources() {
        iterator	it = begin();
        while (it != end())
		{
			(*it).second->Stop();
			it++;
		}
		return;
    }
    
    void Remove (const	UInt32	inSourceToken) {
        iterator 	it = find(inSourceToken);
        if (it != end())
            erase(it);
    }

    UInt32 Size () const { return size(); }
    bool Empty () const { return empty(); }
};


#endif