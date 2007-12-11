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

#ifndef __OAL_SOURCE__
#define __OAL_SOURCE__

#include "oalImp.h"
#include "oalDevice.h"
#include "oalContext.h"
#include "al.h"

#include <Carbon/Carbon.h>
#include <AudioToolbox/AudioConverter.h>
#include <list>
#include "CAStreamBasicDescription.h"
#include "CAGuard.h"
#include "CAMutex.h"
#include "oalAtomicStack.h"
	
class OALBuffer;        // forward declaration

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// buffer info state constants
enum {
		kSecondsOffset		= 1,
		kSampleOffset		= 2,
		kByteOffset		= 3
};	

enum {
		kPendingProcessing = 0,
		kInProgress = 1,
		kProcessed = 2
};

enum {
		kRampingComplete	= -2,
		kRampDown			= -1,
		kNoRamping			= 0,
		kRampUp				= 1
};

enum {
		kMQ_NoMessage					= 0,
		kMQ_Stop						= 1,
		kMQ_Rewind						= 2,
		kMQ_SetBuffer					= 3,
		kMQ_Play						= 4,
		kMQ_Pause						= 5,
		kMQ_Resume						= 6,
		kMQ_SetFramePosition			= 7,
		kMQ_ClearBuffersFromQueue		= 8,
		kMQ_DeconstructionStop			= 9
		
};

#define	OALSourceError_CallConverterAgain	'agan'
#define	kSourceNeedsBus                     -1

// do not change kDistanceScalar from 10.0 - it is used to compensate for a reverb related problem in the 3DMixer
#define kDistanceScalar                     10.0
#define kTransitionState					'tste'

class PlaybackMessage {
public:
	PlaybackMessage(UInt32 inMessage, OALBuffer* inDeferredAppendBuffer, UInt32	inBuffersToUnqueue) :
			mMessageID(inMessage),
			mAppendBuffer(inDeferredAppendBuffer),
			mBuffersToUnqueueInPostRender(inBuffersToUnqueue)
		{ };
		
	~PlaybackMessage(){};

	PlaybackMessage*			mNext;
	UInt32						mMessageID;
	OALBuffer*					mAppendBuffer;
	UInt32						mBuffersToUnqueueInPostRender;
			
	PlaybackMessage *		get_next() { return mNext; }
	void					set_next(PlaybackMessage *next) { mNext = next; }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark _____BufferQueue_____
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// This struct is used by OAL source to store info about the buffers in it's queue
struct	BufferInfo {
							ALuint		mBufferToken;					// the buffer name that the user was returned when the buffer was created
							OALBuffer	*mBuffer;						// buffer object
							UInt32		mOffset;						// current read position offset of this data
							UInt32		mProcessedState;				// mark as true when data of this buffer is finished playing and looping is off
							ALuint		mACToken;						// use this AC from the ACMap for converting the data to the mixer format, when
																		// set to zero, then NO AC is needed, the data has already been converted
};

class BufferQueue : std::list<BufferInfo> {
public:
    
	void 	AppendBuffer(OALSource*	thisSource, ALuint	inBufferToken, OALBuffer	*inBuffer, ALuint	inACToken);
	ALuint 	RemoveQueueEntryByIndex(OALSource*	thisSource, UInt32	inIndex, bool inReleaseIt);
	UInt32 	GetQueueSizeInFrames() ;
	UInt32  GetBufferFrameCount(UInt32	inBufferIndex);
	void 	SetFirstBufferOffset(UInt32	inFrameOffset) ;
	ALuint 	GetBufferTokenByIndex(UInt32	inBufferIndex); 
	UInt32  GetCurrentFrame(UInt32	inBufferIndex);

    BufferInfo*     Get(short		inBufferIndex)  {
        iterator	it = begin();        
        std::advance(it, inBufferIndex);
        if (it != end())
            return(&(*it));
        return (NULL);
    }

	void SetBufferAsProcessed(UInt32	inBufferIndex) {
		iterator	it = begin();
        std::advance(it, inBufferIndex);		
		if (it != end())
			it->mProcessedState = kProcessed;
	}
	
	// mark all the buffers in the queue as unprocessed and offset 0
	void 	ResetBuffers() {
        iterator	it = begin();
        while (it != end())
		{
			it->mProcessedState = kPendingProcessing;
			it->mOffset = 0;
			++it;
		}
	}
	    
    UInt32 Size () const { return size(); }
    bool Empty () const { return empty(); }
	
private:
	UInt32  GetPacketSize() ;
	UInt32	FrameOffsetToPacketOffset(UInt32	inFrameOffset);


};
typedef	BufferQueue BufferQueue;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark _____ACMap_____
// ACMap - map the AudioConverters for the sources queue
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// This struct is used by OAL source to store info about the ACs used by it's queue
struct	ACInfo {
					AudioConverterRef				mConverter;
					CAStreamBasicDescription		mInputFormat; 
};

class ACMap : std::multimap<ALuint, ACInfo, std::less<ALuint> > {
public:
    
    // add a new context to the map
    void Add (const	ALuint	inACToken, ACInfo *inACInfo)  {
		iterator it = upper_bound(inACToken);
		insert(it, value_type (inACToken, *inACInfo));
	}

    AudioConverterRef Get(ALuint	inACToken) {
        const_iterator	it = find(inACToken);
        iterator	theEnd = end();

        if (it != theEnd)
            return ((*it).second.mConverter);
		
		return (NULL);
    }
	
	void GetACForFormat (CAStreamBasicDescription*	inFormat, ALuint	&outToken) {
        iterator	it = begin();
        
		outToken = 0; // 0 means there is none yet
        while (it != end()) {
			if(	(*it).second.mInputFormat == *inFormat) {
				outToken = (*it).first;
				it = end();
			}
			else
				++it;
		}
		return;
	}
    
    void Remove (const	ALuint	inACToken) {
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
        
		while (it !=  theEnd) {
			AudioConverterDispose((*it).second.mConverter);
			++it;
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
#pragma mark _____OALSource_____
class OALSource
{
#pragma mark __________ PRIVATE __________
	private:

		ALuint						mSelfToken;					// the token returned to the caller upon alGenSources()
		bool						mSafeForDeletion;
		OALContext                  *mOwningContext;
		bool						mIsSuspended;
		bool						mCalculateDistance;		
		bool						mResetBusFormat;
		bool						mResetPitch;

		BufferQueue					*mBufferQueueActive;        // map of buffers for queueing
		BufferQueue					*mBufferQueueInactive;      // map of buffers already queued
		ALuint						mCurrentBufferIndex;		// index of the current buffer being played
		bool						mQueueIsProcessed;			// state of the entire buffer queue
		
		pthread_t					mRenderThreadID;
		CAGuard						mPlayGuard;
		AURenderCallbackStruct 		mPlayCallback;
		int							mCurrentPlayBus;			// the mixer bus currently used by this source
		
		ACMap						*mACMap;

		bool						mOutputSilence;
		Float32						mPosition[3];
		Float32						mVelocity[3];
		Float32						mConeDirection[3];
		UInt32						mLooping;
		UInt32						mSourceRelative;
		UInt32						mSourceType;
		Float32						mConeInnerAngle;
		Float32						mConeOuterAngle;
		Float32						mConeOuterGain;

		// Gain Scalers
		Float32						mConeGainScaler;
		Float32						mAttenuationGainScaler;
         
         // support for pre 2.0 3DMixer
        float                       mReadIndex;                    
        float                       mCachedInputL1;                 
        float                       mCachedInputL2;               
        float                       mCachedInputR1;                
        float                       mCachedInputR2;                
        UInt32                      mTempSourceStorageBufferSize;   
        AudioBufferList             *mTempSourceStorage;

		UInt32						mState;						// playback state: Playing, Stopped, Paused, Initial, Transitioning (to stop)
		float						mGain;
        Float32						mPitch;
        Float32						mDopplerScaler;
        Float32						mRollOffFactor;
		Float32						mReferenceDistance;
		Float32						mMaxDistance;
		Float32						mMinGain;
		Float32						mMaxGain;
		
		SInt32						mRampState;
		UInt32						mBufferCountToUnqueueInPostRender;
		bool						mTransitioningToFlushQ;
		
		UInt32						mPlaybackHeadPosition;		// stored for a deferred repositioning of playbackHead as a frame index
		
		Float32						mASAReverbSendLevel;
		Float32						mASAOcclusion;
		Float32						mASAObstruction;

		// Audio Units and properties for RogerBeep and Distortion
		AUNode						mRogerBeepNode;
		AudioUnit					mRogerBeepAU;
		Boolean						mASARogerBeepEnable;
		Boolean						mASARogerBeepOn;
		Float32						mASARogerBeepGain;
		UInt32						mASARogerBeepSensitivity;
		UInt32						mASARogerBeepType;
		char*						mASARogerBeepPreset;

		AUNode						mDistortionNode;
		AudioUnit					mDistortionAU;		
		Boolean						mASADistortionEnable;
		Boolean						mASADistortionOn;
		Float32						mASADistortionMix;
		SInt32						mASADistortionType;
		char*						mASADistortionPreset;
				
		AudioUnit					mRenderUnit;
		UInt32						mRenderElement;
		
	typedef TAtomicStack<PlaybackMessage>	PlaybackMessageList;
	
		PlaybackMessageList			mMessageQueue;

        		
	void        JoinBufferLists();
	void        LoopToBeginning();
	void        ChangeChannelSettings();
	void		InitSource();
	void		SetState(UInt32		inState);
	OSStatus 	DoPreRender ();
	OSStatus 	DoRender (AudioBufferList 	*ioData);
	OSStatus 	DoSRCRender (AudioBufferList 	*ioData);           // support for pre 2.0 3DMixer
	OSStatus 	DoPostRender ();
	bool		ConeAttenuation();
	void 		CalculateDistanceAndAzimuth(Float32 *outDistance, Float32 *outAzimuth, Float32 *outElevation, Float32	*outDopplerShift);
	bool        CallingInRenderThread () const { return (pthread_self() == mRenderThreadID); }
	void        UpdateBusGain ();
	void        UpdateBusFormat ();
	void        UpdateBusReverb ();
	void        UpdateBusOcclusion ();
	void        UpdateBusObstruction ();

	void		UpdateQueue ();
	void		RampDown (AudioBufferList 			*ioData);
	void		RampUp (AudioBufferList 			*ioData);
	void		ClearActiveQueue();
	void		SkipALNONEBuffers();
	void		AddNotifyAndRenderProcs();
	void		ReleaseNotifyAndRenderProcs();
	void		DisconnectFromBus();
	void		SetupMixerBus();
	void		SetupDistortionAU();
	void		SetupRogerBeepAU();
	
	bool		PrepBufferQueueForPlayback();

	UInt32		SecondsToFrames(Float32	inSeconds);
	UInt32		BytesToFrames(Float32	inBytes);
	void		AppendBufferToQueue(ALuint	inBufferToken, OALBuffer	*inBuffer);
	UInt32		FramesToSeconds(UInt32	inFrames);
	UInt32		FramesToBytes(UInt32	inFrames);
	
	void		PostRenderSetBuffer(ALuint inBufferToken, OALBuffer	*inBuffer);
	void		PostRenderRemoveBuffersFromQueue(UInt32	inBuffersToUnqueue);
	void		FlushBufferQueue();
	void		StopFromDestructor();

	void		AdvanceQueueToFrameIndex(UInt32	inFrameOffset);
	OSStatus	SetDistanceParams(bool	inChangeReferenceDistance, bool inChangeMaxDistance);
	Float32		GetMaxAttenuation(Float32	inRefDistance, Float32 inMaxDistance, Float32 inRolloff);
	BufferInfo* NextPlayableBufferInActiveQ();

/*
	OSStatus	MuteCurrentPlayBus () const
	{
		return AudioUnitSetParameter (	mOwningContext->GetMixerUnit(), k3DMixerParam_Gain, kAudioUnitScope_Input, mCurrentPlayBus, 0.0, 0);
	}
*/

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
	OALSource(const ALuint 	 	inSelfToken, OALContext	*inOwningContext);
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
	void	SetQueueOffset(UInt32	inOffsetType, Float32	inSecondOffset);
	void	SetUpDeconstruction();
		
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
	UInt32	GetSourceType ();
	Float32	GetConeInnerAngle ();
	Float32	GetConeOuterAngle ();
	Float32	GetConeOuterGain ();
	UInt32	GetState();
    ALuint	GetToken();
	UInt32	GetQueueOffset(UInt32	inOffsetType);

    // buffer queue
	UInt32	GetQLength();
	UInt32	GetBuffersProcessed();
	void	SetBuffer (ALuint	inBuffer, OALBuffer	*inBuffer);
	ALuint	GetBuffer ();
	void	AddToQueue(ALuint	inBufferToken, OALBuffer	*inBuffer);
	void	RemoveBuffersFromQueue(UInt32	inCount, ALuint	*outBufferTokens);
	bool	IsSourceTransitioningToFlushQ();
	bool	IsBufferInActiveQueue(ALuint inBufferToken);
	bool    IsSafeForDeletion ()  { return (mSafeForDeletion); }
	
	// playback methods
	void	Play();
	void	Pause();
	void	Resume();
	void	Rewind();
	void	Stop();

	void	Suspend();
	void	Unsuspend();
	
	// ASA methods
	void			SetReverbSendLevel(Float32 inReverbLevel);
	void			SetOcclusion(Float32 inOcclusion);
	void			SetObstruction(Float32 inObstruction);
	
	void			SetRogerBeepEnable(Boolean inEnable);
	void			SetRogerBeepOn(Boolean inOn);
	void			SetRogerBeepGain(Float32 inGain);
	void			SetRogerBeepSensitivity(UInt32 inSensitivity);
	void			SetRogerBeepType(UInt32 inType);
	void			SetRogerBeepPreset(FSRef* inRef);
	
	void			SetDistortionEnable(Boolean inEnable);
	void			SetDistortionOn(Boolean inOn);
	void			SetDistortionMix(Float32 inMix);
	void			SetDistortionType(SInt32 inType);
	void			SetDistortionPreset(FSRef* inRef);

	Float32			GetReverbSendLevel() {return mASAReverbSendLevel;}
	Float32			GetOcclusion() {return mASAOcclusion;}
	Float32			GetObstruction() {return mASAObstruction;}

	Boolean			GetRogerBeepEnable() {return mASARogerBeepEnable;}
	Boolean			GetRogerBeepOn() {return mASARogerBeepOn;}
	Float32			GetRogerBeepGain() {return mASARogerBeepGain;}
	UInt32			GetRogerBeepSensitivity() {return mASARogerBeepSensitivity;}
	UInt32			GetRogerBeepType() {return mASARogerBeepType;}
	
	Boolean			GetDistortionEnable() {return mASADistortionEnable;}
	Boolean			GetDistortionOn() {return mASADistortionOn;}
	Float32			GetDistortionMix() {return mASADistortionMix;}
	SInt32			GetDistortionType() {return mASADistortionType;}
};	

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark _____OALSourceMap_____
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class OALSourceMap : std::multimap<ALuint, OALSource*, std::less<ALuint> > {
public:
    
    // add a new context to the map
    void Add (const	ALuint	inSourceToken, OALSource **inSource)  {
		iterator it = upper_bound(inSourceToken);
		insert(it, value_type (inSourceToken, *inSource));
	}

    OALSource* GetSourceByIndex(UInt32	inIndex) {
        iterator	it = begin();

		for (UInt32 i = 0; i < inIndex; i++) {
            if (it != end())
                ++it;
            else
                i = inIndex;
        }
        
        if (it != end())
            return ((*it).second);		
		return (NULL);
    }

    OALSource* Get(ALuint	inSourceToken) {
        iterator	it = find(inSourceToken);
        if (it != end())
            return ((*it).second);
		return (NULL);
    }

    void MarkAllSourcesForRecalculation() {
        iterator	it = begin();
        while (it != end()) {
			(*it).second->SetChannelParameters();
			++it;
		}
		return;
    }
        
    void SuspendAllSources() {
        iterator	it = begin();
        while (it != end())
		{
			(*it).second->Suspend();
			++it;
		}
		return;
    }

    void UnsuspendAllSources() {
        iterator	it = begin();
        while (it != end())
		{
			(*it).second->Unsuspend();
			++it;
		}
		return;
    }

    void Remove (const	ALuint	inSourceToken) {
        iterator 	it = find(inSourceToken);
        if (it != end())
            erase(it);
    }

    UInt32 Size () const { return size(); }
    bool Empty () const { return empty(); }
};

#endif