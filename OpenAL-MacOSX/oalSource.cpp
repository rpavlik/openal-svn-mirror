/**********************************************************************************************************************************
*
*   OpenAL cross platform audio library
*	Copyright (c) 2004, Apple Computer, Inc., Copyright (c) 2009, Apple Inc. All rights reserved.
*
*   Redistribution and use in source and binary forms, with or without modification, are permitted provided 
*   that the following conditions are met:
*
*   1.  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. 
*   2.  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
*       disclaimer in the documentation and/or other materials provided with the distribution. 
*   3.  Neither the name of Apple Inc. ("Apple") nor the names of its contributors may be used to endorse or promote 
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

/*
	Each OALSource object maintains a BufferQueue and an ACMap. The buffer queue is an ordered list of BufferInfo structs.
	These structs contain an OAL buffer and other pertinent data. The ACMap is a multimap of ACInfo structs. These structs each contain an
	AudioConverter and the input format of the AudioConverter. The AudioConverters are created as needed each time a buffer with a new 
    format is added to the queue. This allows differently formatted data to be queued seemlessly. The correct AC is used for each 
    buffer as the BufferInfo keeps track of the appropriate AC to use.
*/

#include "oalSource.h"
#include "oalBuffer.h"
#include "oalImp.h"

#define		LOG_PLAYBACK				0
#define		LOG_VERBOSE					0
#define		LOG_BUFFER_QUEUEING			0
#define		LOG_DOPPLER                 0
#define		LOG_MESSAGE_QUEUE           0

#define		CALCULATE_POSITION	1	// this should be true except for testing

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define MIXER_PARAMS_UNDEFINED 0

#if MIXER_PARAMS_UNDEFINED
typedef struct MixerDistanceParams {
			Float32		mReferenceDistance;
			Float32		mMaxDistance;
			Float32		mMaxAttenuation;
} MixerDistanceParams;

enum {
	kAudioUnitProperty_3DMixerDistanceParams   = 3010
};
#endif

inline bool zapBadness(Float32&		inValue)
{
	Float32		absInValue = fabs(inValue);
	
	if (!(absInValue > 1e-15 && absInValue < 1e15)){
		// inValue was one of the following: 0.0, infinity, denormal or NaN
		inValue = 0.0;
		return true;
	}
	
	return false;
}

// if dopplerShift = inifinity then peg to 16 (4 octaves up)
// if dopplershift is a denormal then peg to .125 (3 octaves down)
// if nan, then set to 1.0 (no doppler)
// if 0.0 then set to 1.0 which is no shift
inline bool zapBadnessForDopplerShift(Float32&		inValue)
{
	Float32		absInValue = fabs(inValue);

	if (isnan(inValue) || (inValue == 0.0))
		inValue = 1.0;
	else if (absInValue > 1e15)
		inValue = 16.0;
	else if (absInValue < 1e-15)
		inValue = .125;

	return false;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// OALSource
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ***** PUBLIC *****
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OALSource::OALSource (const ALuint 	 	inSelfToken, OALContext	*inOwningContext)
	: 	mSelfToken (inSelfToken),
		mSafeForDeletion(false),
		mOwningContext(inOwningContext),
		mIsSuspended(false),
		mCalculateDistance(true),
		mResetBusFormat(true),
        mResetPitch(true),
		mBufferQueueActive(NULL),
		mBufferQueueInactive(NULL),
		mBuffersQueuedForClear(0),
		mCurrentBufferIndex(0),
		mQueueIsProcessed(true),
		mEditFlag(0),
		mInUseFlag(0),
		mSourceLock("OAL:SourceLock"),
		mCurrentPlayBus (kSourceNeedsBus),
		mACMap(NULL),
		mOutputSilence(false),
		mLooping(AL_FALSE),
		mSourceRelative(AL_FALSE),
		mSourceType(AL_UNDETERMINED),
		mConeInnerAngle(360.0),
		mConeOuterAngle(360.0),
		mConeOuterGain(0.0),
		mConeGainScaler(1.0),
		mAttenuationGainScaler(1.0),
        mReadIndex(0.0),
        mTempSourceStorageBufferSize(2048),             // only used if preferred mixer is unavailable
		mState(AL_INITIAL),
		mGain(1.0),
		mPitch(1.0),                                    // this is the user pitch setting changed via the OAL APIs
        mDopplerScaler(1.0),                          
		mRollOffFactor(kDefaultRolloff),
		mReferenceDistance(kDefaultReferenceDistance),
		mMaxDistance(kDefaultMaximumDistance),          // ***** should be MAX_FLOAT
		mMinGain(0.0),
		mMaxGain(1.0),
		mRampState(kNoRamping),
		mBufferCountToUnqueueInPostRender(0),
		mTransitioningToFlushQ(false),
		mASAReverbSendLevel(0.0),
		mASAOcclusion(0.0),
		mASAObstruction(0.0),
		mRogerBeepNode(0),
		mRogerBeepAU(0),
		mASARogerBeepEnable(false),
		mASARogerBeepOn(true),								// RogerBeep AU does not bypass by default
		mASARogerBeepGain(0.0),
		mASARogerBeepSensitivity(0),
		mASARogerBeepType(0),
		mASARogerBeepPreset(0),
		mDistortionNode(0),
		mDistortionAU(0),
		mASADistortionEnable(false),
		mASADistortionOn(true),							// Distortion AU does not bypass by default
		mASADistortionMix(0.0),
		mASADistortionType(0),
		mASADistortionPreset(0)
{		
    mPosition[0] = 0.0;
    mPosition[1] = 0.0;
    mPosition[2] = 0.0;
    
    mVelocity[0] = 0.0;
    mVelocity[1] = 0.0;
    mVelocity[2] = 0.0;

    mConeDirection[0] = 0.0;
    mConeDirection[1] = 0.0;
    mConeDirection[2] = 0.0;

    mBufferQueueActive = new BufferQueue();
    mBufferQueueInactive = new BufferQueue();
    mACMap = new ACMap();

    mReferenceDistance = mOwningContext->GetDefaultReferenceDistance();
    mMaxDistance = mOwningContext->GetDefaultMaxDistance();
     
    if (Get3DMixerVersion() < k3DMixerVersion_2_0)
    {
        // since the preferred mixer is not available, some temporary storgae will be needed for SRC
        // for now assume that sources will not have more than 2 channels of data
        mTempSourceStorage = (AudioBufferList *) malloc ((offsetof(AudioBufferList, mBuffers)) + (2 * sizeof(AudioBuffer)));
        
		mTempSourceStorage->mBuffers[0].mDataByteSize = mTempSourceStorageBufferSize;
        mTempSourceStorage->mBuffers[0].mData = malloc(mTempSourceStorageBufferSize);
        
        mTempSourceStorage->mBuffers[1].mDataByteSize = mTempSourceStorageBufferSize;
        mTempSourceStorage->mBuffers[1].mData = malloc(mTempSourceStorageBufferSize);
    }
	else
	{
	   mTempSourceStorage = NULL;
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OALSource::~OALSource()
{
#if LOG_VERBOSE
	DebugMessageN1("OALSource::~OALSource() - OALSource = %ld", mSelfToken);
#endif
	
    // release the 3DMixer bus if necessary
	if (mCurrentPlayBus != kSourceNeedsBus)
	{
		ReleaseNotifyAndRenderProcs();
		mOwningContext->SetBusAsAvailable (mCurrentPlayBus);
		mCurrentPlayBus = kSourceNeedsBus;		
	}
		    
    // empty the two queues
	FlushBufferQueue();
	
	// empty the message queue
	ClearMessageQueue();
	
	if (mTempSourceStorage)
		free(mTempSourceStorage);
		
    delete (mBufferQueueInactive);
    delete (mBufferQueueActive);
    	
	// remove all the AudioConverters that were created for the buffer queue of this source object
    if (mACMap)
	{
		mACMap->RemoveAllConverters();
		delete (mACMap);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// this is only callled by the context when removing a source, which is thread protected already
void	OALSource::SetUpDeconstruction()
{
#if LOG_VERBOSE
	DebugMessageN1("OALSource::SetUpDeconstruction() - OALSource = %ld", mSelfToken);
#endif

	try {
		// wait if in render, then prevent rendering til completion
		OALRenderLocker::RenderLocker locked(mRenderLocker, InRenderThread());

		// don't allow synchronous source manipulation
		CAGuard::Locker sourceLock(mSourceLock);

		switch (mState)
		{
			// if rendering is occurring right now, then let the PostRender tear things down and set mSafeForDeletion to true
			case AL_PLAYING:
			case kTransitionState:
			{
				AddPlaybackMessage(kMQ_DeconstructionStop, NULL, 0);
			}
				break;
			
			default:
				mSafeForDeletion = true;
				break;
		}

		mState = kTransitionState;
		mTransitioningToFlushQ = true;
	}
	catch (OSStatus	result) {
		DebugMessageN2("SetUpDeconstruction FAILED source = %d, err = %d\n", (int) mSelfToken, (int)result);
		throw (result);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::Suspend ()
{
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::Unsuspend ()
{
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// SET METHODS 
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetPitch (float	inPitch)
{
	if (inPitch < 0.0f)
		throw ((OSStatus) AL_INVALID_VALUE);

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

    if ((inPitch == mPitch) && (mResetPitch == false))
		return;			// nothing to do
	
	mPitch = inPitch;

     // 1.3 3DMixer does not work properly when doing SRC on a mono bus
	 if (Get3DMixerVersion() < k3DMixerVersion_2_0)
        return;        

	Float32     newPitch = mPitch * mDopplerScaler;
    if (mCurrentPlayBus != kSourceNeedsBus)
	{
		BufferInfo		*oalBuffer = mBufferQueueActive->Get(mCurrentBufferIndex);
		
		if (oalBuffer != NULL)
		{
#if LOG_GRAPH_AND_MIXER_CHANGES
    DebugMessageN2("OALSource::SetPitch: k3DMixerParam_PlaybackRate called - OALSource:mPitch = %ld:%f2\n", mSelfToken, mPitch );
#endif            
			
			OSStatus    result = AudioUnitSetParameter (mOwningContext->GetMixerUnit(), k3DMixerParam_PlaybackRate, kAudioUnitScope_Input, mCurrentPlayBus, newPitch, 0);
            if (result != noErr)
                DebugMessageN3("OALSource::SetPitch: k3DMixerParam_PlaybackRate called - OALSource = %ld mPitch = %f2 result = %d\n", (long int) mSelfToken, mPitch, (int)result );
        }

		mResetPitch = false;
	}
	else
		mResetPitch = true; // the source is not currently connected to a bus, so make this change when play is called
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetGain (float	inGain)
{	
#if LOG_VERBOSE
        DebugMessageN2("OALSource::SetGain - OALSource:inGain = %ld:%f\n", (long int) mSelfToken, inGain);
#endif

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

	if (mGain != inGain)
	{
		mGain = inGain;
		UpdateBusGain();
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetMinGain (Float32	inMinGain)
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::SetMinGain - OALSource:inMinGain = %ld:%f\n", (long int) mSelfToken, inMinGain);
#endif
	if ((inMinGain < 0.0f) || (inMinGain > 1.0f))
		throw ((OSStatus) AL_INVALID_VALUE);

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

	if (mMinGain != inMinGain)
	{
		mMinGain = inMinGain;
		UpdateBusGain();
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetMaxGain (Float32	inMaxGain)
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::SetMaxGain - OALSource:inMaxGain = %ld:%f\n", (long int) mSelfToken, inMaxGain);
#endif

	if ((inMaxGain < 0.0f) || (inMaxGain > 1.0f))
		throw ((OSStatus) AL_INVALID_VALUE);

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

	if (mMaxGain != inMaxGain)
	{
		mMaxGain = inMaxGain;
		UpdateBusGain();
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Float32		OALSource::GetMaxAttenuation(Float32	inRefDistance, Float32 inMaxDistance, Float32 inRolloff)
{
	Float32		returnValue = 20 * log10(inRefDistance / (inRefDistance + (inRolloff * (inMaxDistance -  inRefDistance))));

	if (returnValue < 0.0)
		returnValue *= -1.0;
	else 
		returnValue = 0.0;   // if db result was positive, clamp it to zero

	return returnValue;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus	OALSource::SetDistanceParams(bool	inChangeReferenceDistance, bool inChangeMaxDistance)
{
	OSStatus	result = noErr;

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

	if (Get3DMixerVersion() < k3DMixerVersion_2_0)	
	{
		// the pre-2.0 3DMixer does not accept kAudioUnitProperty_3DMixerDistanceParams, it has do some extra work and use the DistanceAtten property instead
		mOwningContext->SetDistanceAttenuation(mCurrentPlayBus, mReferenceDistance, mMaxDistance, mRollOffFactor);
	}
	else
	{
		MixerDistanceParams		distanceParams;
		UInt32					propSize = sizeof(distanceParams);
		result = AudioUnitGetProperty(mOwningContext->GetMixerUnit(), kAudioUnitProperty_3DMixerDistanceParams, kAudioUnitScope_Input, mCurrentPlayBus, &distanceParams, &propSize);
		if (result == noErr)
		{
			Float32     rollOff = mRollOffFactor;

			if (mOwningContext->IsDistanceScalingRequired())
			{
				// scale the reference distance
				distanceParams.mReferenceDistance = (mReferenceDistance/mMaxDistance) * kDistanceScalar;
				// limit the max distance
				distanceParams.mMaxDistance = kDistanceScalar;
				// scale the rolloff
				rollOff *= (kDistanceScalar/mMaxDistance);
			}
			else
			{
				if (inChangeReferenceDistance)
					distanceParams.mReferenceDistance = mReferenceDistance;
				else if (inChangeMaxDistance)
					distanceParams.mMaxDistance = mMaxDistance;
			}

			distanceParams.mMaxAttenuation = GetMaxAttenuation(distanceParams.mReferenceDistance, distanceParams.mMaxDistance, rollOff);

			if ((mReferenceDistance == mMaxDistance) && (Get3DMixerVersion() < k3DMixerVersion_2_2))
				distanceParams.mMaxDistance = distanceParams.mReferenceDistance + .01; // pre 2.2 3DMixer may crash  if max and reference distances are equal
			
			result = AudioUnitSetProperty(mOwningContext->GetMixerUnit(), kAudioUnitProperty_3DMixerDistanceParams, kAudioUnitScope_Input, mCurrentPlayBus, &distanceParams, sizeof(distanceParams));
		}
	}
	
	return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetReferenceDistance (Float32	inReferenceDistance)
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::SetReferenceDistance - OALSource:inReferenceDistance = %ld/%f2\n", (long int) mSelfToken, inReferenceDistance);
#endif

	if (inReferenceDistance <= 0.0f)
		throw ((OSStatus) AL_INVALID_VALUE);

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);
				
	if (inReferenceDistance == mReferenceDistance)
		return; // nothing to do

	mReferenceDistance = inReferenceDistance;

	if (!mOwningContext->DoSetDistance())
		return; // nothing else to do?
 	
    if (mCurrentPlayBus != kSourceNeedsBus)
    {
		SetDistanceParams(true, false);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetMaxDistance (Float32	inMaxDistance)
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::SetMaxDistance - OALSource:inMaxDistance = %ld:%f2\n", (long int) mSelfToken, inMaxDistance);
#endif

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

	if (inMaxDistance == mMaxDistance)
		return; // nothing to do

	mMaxDistance = inMaxDistance;

	if (!mOwningContext->DoSetDistance())
		return; // nothing else to do?

    if (mCurrentPlayBus != kSourceNeedsBus)
    {
		SetDistanceParams(false, true);
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetRollOffFactor (Float32	inRollOffFactor)
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::SetRollOffFactor - OALSource:inRollOffFactor = %ld:%f\n", (long int) mSelfToken, inRollOffFactor);
#endif

	if (inRollOffFactor < 0.0f) 
		throw ((OSStatus) AL_INVALID_VALUE);

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

	if (inRollOffFactor == mRollOffFactor)
		return; // nothing to do

	mRollOffFactor = inRollOffFactor;
	
	if (!mOwningContext->DoSetDistance())
		return; // nothing else to do?
 	
    if (mCurrentPlayBus != kSourceNeedsBus)
    {
		SetDistanceParams(false, false);
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetLooping (UInt32	inLooping)
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::SetLooping called - OALSource:inLooping = %ld:%ld\n", (long int) mSelfToken, inLooping);
#endif

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

    mLooping = inLooping;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetPosition (Float32 inX, Float32 inY, Float32 inZ)
{
#if LOG_VERBOSE
	DebugMessageN4("OALSource::SetPosition called - OALSource:X:Y:Z = %ld:%f:%f:%f\n", (long int) mSelfToken, inX, inY, inZ);
#endif

	if (isnan(inX) || isnan(inY) || isnan(inZ))
		throw ((OSStatus) AL_INVALID_VALUE);

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

	mPosition[0] = inX;
	mPosition[1] = inY;
	mPosition[2] = inZ;

	mCalculateDistance = true;  // change the distance next time the PreRender proc or a new Play() is called
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetVelocity (Float32 inX, Float32 inY, Float32 inZ)
{
#if LOG_VERBOSE
	DebugMessageN1("OALSource::SetVelocity called - OALSource = %ld", (long int) mSelfToken);
#endif

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

	mVelocity[0] = inX;
	mVelocity[1] = inY;
	mVelocity[2] = inZ;

	mCalculateDistance = true;  // change the velocity next time the PreRender proc or a new Play() is called
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetDirection (Float32 inX, Float32 inY, Float32 inZ)
{
#if LOG_VERBOSE
	DebugMessageN1("OALSource::SetDirection called - OALSource = %ld", (long int) mSelfToken);
#endif

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

	mConeDirection[0] = inX;
	mConeDirection[1] = inY;
	mConeDirection[2] = inZ;

	mCalculateDistance = true;  // change the direction next time the PreRender proc or a new Play() is called
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetSourceRelative (UInt32	inSourceRelative)
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::SetSourceRelative called - OALSource:inSourceRelative = %ld:%ld", (long int) mSelfToken, inSourceRelative);
#endif

	if ((inSourceRelative != AL_FALSE) && (inSourceRelative != AL_TRUE))
		throw ((OSStatus) AL_INVALID_VALUE);

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);
	
	mSourceRelative = inSourceRelative;
	mCalculateDistance = true;  // change the source relative next time the PreRender proc or a new Play() is called
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetChannelParameters ()	
{
#if LOG_VERBOSE
	DebugMessageN1("OALSource::SetChannelParameters called - OALSource = %ld\n", (long int) mSelfToken);
#endif

	SetReferenceDistance(mReferenceDistance);
	SetMaxDistance(mMaxDistance);
	SetRollOffFactor(mRollOffFactor);
	
	mCalculateDistance = true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetConeInnerAngle (Float32	inConeInnerAngle)
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::SetConeInnerAngle called - OALSource:inConeInnerAngle = %ld:%f2\n", (long int) mSelfToken, inConeInnerAngle);
#endif

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

    if (mConeInnerAngle != inConeInnerAngle)
	{
		mConeInnerAngle = inConeInnerAngle;
		mCalculateDistance = true;  
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetConeOuterAngle (Float32	inConeOuterAngle)
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::SetConeOuterAngle called - OALSource:inConeOuterAngle = %ld:%f2\n", (long int) mSelfToken, inConeOuterAngle);
#endif

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

    if (mConeOuterAngle != inConeOuterAngle)
	{
		mConeOuterAngle = inConeOuterAngle;
		mCalculateDistance = true;  
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetConeOuterGain (Float32	inConeOuterGain)
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::SetConeOuterGain called - OALSource:inConeOuterGain = %ld:%f2\n", (long int) mSelfToken, inConeOuterGain);
#endif
	if (inConeOuterGain >= 0.0 && inConeOuterGain <= 1.0)
	{
		// don't allow synchronous source manipulation
		CAGuard::Locker sourceLock(mSourceLock);

		if (mConeOuterGain != inConeOuterGain)
		{
			mConeOuterGain = inConeOuterGain;
			mCalculateDistance = true;  
		}
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// GET METHODS 
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Float32	OALSource::GetPitch ()
{
    return mPitch;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Float32	OALSource::GetDopplerScaler ()
{
    return mDopplerScaler;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Float32	OALSource::GetGain ()
{
    return mGain;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Float32	OALSource::GetMinGain ()
{
    return mMinGain;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Float32	OALSource::GetMaxGain ()
{
    return mMaxGain;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Float32	OALSource::GetReferenceDistance ()
{
    return mReferenceDistance;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Float32	OALSource::GetMaxDistance ()
{
    return mMaxDistance;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Float32	OALSource::GetRollOffFactor ()
{
    return mRollOffFactor;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UInt32	OALSource::GetLooping ()
{
    return mLooping; 
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::GetPosition (Float32 &inX, Float32 &inY, Float32 &inZ)
{
#if LOG_VERBOSE
	DebugMessageN1("OALSource::GetPosition called - OALSource = %ld\n", (long int) mSelfToken);
#endif

	inX = mPosition[0];
	inY = mPosition[1];
	inZ = mPosition[2];
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::GetVelocity (Float32 &inX, Float32 &inY, Float32 &inZ)
{
#if LOG_VERBOSE
	DebugMessageN1("OALSource::GetVelocity called - OALSource = %ld\n", (long int) mSelfToken);
#endif

	inX = mVelocity[0];
	inY = mVelocity[1];
	inZ = mVelocity[2];
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::GetDirection (Float32 &inX, Float32 &inY, Float32 &inZ)
{
#if LOG_VERBOSE
	DebugMessageN1("OALSource::GetDirection called - OALSource = %ld\n", (long int) mSelfToken);
#endif

	inX = mConeDirection[0];
	inY = mConeDirection[1];
	inZ = mConeDirection[2];
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UInt32	OALSource::GetSourceRelative ()
{
    return mSourceRelative;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UInt32	OALSource::GetSourceType ()
{
    return mSourceType;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Float32	OALSource::GetConeInnerAngle ()
{
    return mConeInnerAngle;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Float32	OALSource::GetConeOuterAngle ()
{
    return mConeOuterAngle;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Float32	OALSource::GetConeOuterGain ()
{
    return mConeOuterGain;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UInt32	OALSource::GetState()
{
    // this is a special situation. The only time mState will be kTransitionState is when the source was stopped
	// while currently playing, but the deferred Q has not yet cleaned up. Effectively this is a stopped state for the
	// caller, as the source should behave as if it was already stopped. It's an internal state.
	if (mState == kTransitionState)
		return AL_STOPPED;
		
	return mState;
}
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALuint	OALSource::GetToken()
{
    return mSelfToken;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// BUFFER QUEUE METHODS 
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// this should only be called when the source is render and edit protected
UInt32	OALSource::GetQLength()
{
    // as far as the user is concerned, the Q length is the size of the inactive & active lists, 
	// minus the inactive buffers that are pending removal
	return mBufferQueueInactive->Size() + mBufferQueueActive->Size() - mBuffersQueuedForClear;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UInt32	OALSource::GetBuffersProcessed()
{
	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

	if (mState == AL_INITIAL)
        return 0;
		
	if (mState == kTransitionState)
		return GetQLength();	// transistioning to Stop is the same as being in a stopped state for this call, but the queues may not be joined yet
   
	if (mQueueIsProcessed)
	{
		// fixes 4085888
		// When the Q ran dry it might not have been possible to modify the Buffer Q Lists
		// This means that there could be one left over buffer in the active Q list which should not be there
		ClearActiveQueue();
	}
		
	return mBufferQueueInactive->Size();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetBuffer (ALuint inBufferToken, OALBuffer	*inBuffer)
{
	if (inBuffer == NULL)
		return;	// invalid case
    
	// wait if in render, then prevent rendering til completion
	OALRenderLocker::RenderLocker locked(mRenderLocker, InRenderThread());

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

	switch (mState)
	{
		case AL_PLAYING:
		case AL_PAUSED:
#if LOG_VERBOSE
			DebugMessageN2("OALSource::SetBuffer ERROR is already playing - OALSource:inBufferToken = %ld:%ld", (long int) mSelfToken, inBufferToken);
#endif
			throw AL_INVALID_OPERATION;
			break;
		
		case kTransitionState:
		{
#if LOG_VERBOSE
			DebugMessageN2("OALSource::SetBuffer Deferred - OALSource:inBufferToken = %ld:%ld", (long int) mSelfToken, (long int) inBufferToken);
#endif
			if (inBufferToken == 0)
			{
				mSourceType = AL_UNDETERMINED;
				mTransitioningToFlushQ = true;
			}
				
#if	LOG_MESSAGE_QUEUE					
			DebugMessageN1("OALSource::SetBuffer - kMQ_SetBuffer added to MQ - OALSource = %ld", (long int) mSelfToken);
#endif
			AddPlaybackMessage(kMQ_SetBuffer, inBuffer, 0);
			break;
		}
		
		default:
		{										
#if LOG_VERBOSE
			DebugMessageN2("OALSource::SetBuffer NOW - OALSource:inBufferToken = %ld:%ld", mSelfToken, inBufferToken);
#endif
			// In the initial or stopped state it is ok to flush the buffer Qs and add this new buffer right now, so empty the two queues
			FlushBufferQueue();
				
			// if inBufferToken == 0, do nothing, passing NONE to this method is legal and should merely empty the queue
			if (inBufferToken != 0)
			{
				AppendBufferToQueue(inBufferToken, inBuffer);
				mSourceType = AL_STATIC;
			}
			else
				mSourceType = AL_UNDETERMINED;
		}
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// this should only be called when the source is render and edit protected
void	OALSource::FlushBufferQueue()
{
	UInt32	count = mBufferQueueInactive->Size();
	for (UInt32	i = 0; i < count; i++)
	{	
		mBufferQueueInactive->RemoveQueueEntryByIndex(this, 0, true);
		mBuffersQueuedForClear = 0;
	}
   
	count = mBufferQueueActive->Size();
	for (UInt32	i = 0; i < count; i++)
	{	
		mBufferQueueActive->RemoveQueueEntryByIndex(this, 0, true);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALuint	OALSource::GetBuffer()
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::GetBuffer called - OALSource:currentBuffer = %ld:%ld\n", mSelfToken, mBufferQueueActive->GetBufferTokenByIndex(mCurrentBufferIndex));
#endif
	// wait if in render, then prevent rendering til completion
	OALRenderLocker::RenderLocker locked(mRenderLocker, InRenderThread());

	// need to make sure no other thread touches the active queue
	CAGuard::Locker sourceLock(mSourceLock);

	// for now, return the currently playing buffer in an active source, or the 1st buffer of the Q in an inactive source
	return (mBufferQueueActive->GetBufferTokenByIndex(mCurrentBufferIndex));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// public method
void	OALSource::AddToQueue(ALuint	inBufferToken, OALBuffer	*inBuffer)
{
	// wait if in render, then prevent rendering til completion
	OALRenderLocker::RenderLocker locked(mRenderLocker, InRenderThread());

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

	if (mSourceType == AL_STATIC)
		throw AL_INVALID_OPERATION;
			
	if (mSourceType == AL_UNDETERMINED)
		mSourceType = AL_STREAMING;
			
	AppendBufferToQueue(inBufferToken, inBuffer);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// this should only be called when the source is render and edit protected
void	OALSource::AppendBufferToQueue(ALuint	inBufferToken, OALBuffer	*inBuffer)
{
	OSStatus	result = noErr;	
#if LOG_BUFFER_QUEUEING
	DebugMessageN2("OALSource::AppendBufferToQueue called (START) - OALSource:inBufferToken = %ld:%ld", (long int)mSelfToken, (long int)inBufferToken);
#endif
    
	try {
		if (mBufferQueueActive->Empty())
		{
			mCurrentBufferIndex = 0;	// this is the first buffer in the queue
			mQueueIsProcessed = false;
		}
								
		// do we need an AC for the format of this buffer?
		if (inBuffer->HasBeenConverted())
		{
			// the data was already convertered to the mixer format, no AC is necessary (as indicated by the ACToken setting of zero)
			mBufferQueueActive->AppendBuffer(this, inBufferToken, inBuffer, 0);
		}
		else
		{			
			// check the format against the real format of the data, NOT the input format of the converter which may be subtly different
			// both in SampleRate and Interleaved flags
			ALuint		outToken = 0;
			mACMap->GetACForFormat(inBuffer->GetFormat(), outToken);
			if (outToken == 0)
			{
				// create an AudioConverter for this format because there isn't one yet
				AudioConverterRef				converter = 0;
				CAStreamBasicDescription		inFormat;
				
				inFormat.SetFrom(*(inBuffer->GetFormat()));
					
				// if the source is mono, set the flags to be non interleaved, so a reinterleaver does not get setup when
				// completely unnecessary - since the flags on output are always set to non interleaved
				if (inFormat.NumberChannels() == 1)
					inFormat.mFormatFlags |= kAudioFormatFlagIsNonInterleaved; 
						
				// output should have actual number of channels, but frame/packet size is for single channel
				// this is to make de interleaved data work correctly with > 1 channel
				CAStreamBasicDescription 	outFormat;
				outFormat.mChannelsPerFrame = inFormat.NumberChannels();
				outFormat.mSampleRate = inFormat.mSampleRate;
				outFormat.mFormatID = kAudioFormatLinearPCM;
				outFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
				outFormat.mBytesPerPacket = sizeof (Float32);	
				outFormat.mFramesPerPacket = 1;	
				outFormat.mBytesPerFrame = sizeof (Float32);
				outFormat.mBitsPerChannel = sizeof (Float32) * 8;	
	
				result = AudioConverterNew(&inFormat, &outFormat, &converter);
					THROW_RESULT
				
				ACInfo	info;
				info.mInputFormat = *(inBuffer->GetFormat());
				info.mConverter = converter;
				
				// add this AudioConverter to the source's ACMap
				ALuint	newACToken = GetNewToken();
				mACMap->Add(newACToken, &info);
				// add the buffer to the queue - each buffer now knows which AC to use when it is converted in the render proc
				mBufferQueueActive->AppendBuffer(this, inBufferToken, inBuffer, newACToken);
			}
			else
			{
				// there is already an AC for this buffer's data format, so just append the buffer to the queue
				mBufferQueueActive->AppendBuffer(this, inBufferToken, inBuffer, outToken);
			}
		}
		
		inBuffer->UseThisBuffer(this);
	}
	catch (OSStatus	 result) {
		DebugMessageN1("APPEND BUFFER FAILED %ld\n", (long int) mSelfToken);
		throw (result);
	}

#if LOG_BUFFER_QUEUEING
	DebugMessageN2("OALSource::AppendBufferToQueue called (END) - OALSource:QLength = %ld:%ld", (long int)mSelfToken, mBufferQueueInactive->Size() + mBufferQueueActive->Size());
#endif

	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Do NOT call this from the render thread
void	OALSource::RemoveBuffersFromQueue(UInt32	inCount, ALuint	*outBufferTokens)
{
	if (inCount == 0)
		return;

#if LOG_BUFFER_QUEUEING
	DebugMessageN2("OALSource::RemoveBuffersFromQueue called (START) - OALSource:inCount = %ld:%ld", (long int)mSelfToken, inCount);
#endif
	
	try {
        
		// wait if in render, then prevent rendering til completion
		OALRenderLocker::RenderLocker locked(mRenderLocker, InRenderThread());

		// don't allow synchronous source manipulation
		CAGuard::Locker sourceLock(mSourceLock);

		if (inCount > GetQLength())
			throw ((OSStatus) AL_INVALID_OPERATION);			

		if (outBufferTokens == NULL)
			throw ((OSStatus) AL_INVALID_OPERATION);			
		
		// 1) determine if it is a legal request
		// 2) determine if buffers can be removed now or at PostRender time
		// 3) get the buffer names that will be removed and if safe, remove them now
		
		if ((mState == AL_PLAYING) || (mState == AL_PAUSED))
		{			
			if (mLooping == true)
				throw ((OSStatus) AL_INVALID_OPERATION);
			else if (inCount > mBufferQueueInactive->Size())
				throw ((OSStatus) AL_INVALID_OPERATION);
		}
		else if (mState == AL_STOPPED)
		{
			ClearActiveQueue();
			if (inCount > mBufferQueueInactive->Size())
			{
				DebugMessageN3("	OALSource::RemoveBuffersFromQueue mState == AL_STOPPED - OALSource:GetQLength():mBufferQueueInactive->Size() = %d:%d:%d", (int) mSelfToken, (int)GetQLength(), (int)mBufferQueueInactive->Size());
				throw ((OSStatus) AL_INVALID_OPERATION);	
			}
		}
		else if (mState == kTransitionState)
		{
#if LOG_BUFFER_QUEUEING
			DebugMessageN2("	RemoveBuffersFromQueue kTransitionState - OALSource:inCount = %ld:%ld", (long int)mSelfToken, inCount);
#endif
#if	LOG_MESSAGE_QUEUE					
			DebugMessageN1("	RemoveBuffersFromQueue ADDING : kMQ_ClearBuffersFromQueue - OALSource = %ld", (long int) mSelfToken);
#endif

			AddPlaybackMessage(kMQ_ClearBuffersFromQueue, NULL, inCount);
		}
		
		for (UInt32	i = mBuffersQueuedForClear; i < mBuffersQueuedForClear+inCount; i++)
		{	
			if (mState == kTransitionState)
			{
				//DebugMessageN1("	* RemoveBuffersFromQueue kTransitionState - GetQLength() = %ld", (long int) GetQLength());
				// we're transitioning, so let the caller know what buffers will be removed, but don't actually do it until the deferred message is acted on
				// first return the token for the buffers in the inactive queue, then the active queue
				if (i < mBufferQueueInactive->Size())
				{
					outBufferTokens[i-mBuffersQueuedForClear] = mBufferQueueInactive->GetBufferTokenByIndex(i);
					//DebugMessageN1("	DEFERRED * RemoveBuffersFromQueue kTransitionState - mBufferQueueInactive->GetBufferTokenByIndex(i) = %ld", (long int) outBufferTokens[i]);
				}
				else
				{
					outBufferTokens[i-mBuffersQueuedForClear] = mBufferQueueActive->GetBufferTokenByIndex(i - mBufferQueueInactive->Size());
					//DebugMessageN1("	DEFERRED * RemoveBuffersFromQueue kTransitionState - mBufferQueueActive->GetBufferTokenByIndex(i) = %ld", (long int) outBufferTokens[i]);
				}
			}
			else
			{
				ALuint	outToken = 0;
				// it is safe to remove the buffers from the inactive Q
				outToken = mBufferQueueInactive->RemoveQueueEntryByIndex(this, 0, true);
				mBuffersQueuedForClear = 0;
				#if	LOG_MESSAGE_QUEUE					
					DebugMessageN1("	Just Removed Buffer Id from inactive Q = %ld", (long int) outToken);
				#endif
				outBufferTokens[i] = outToken;
			}
		}
		
		if (mState == kTransitionState)
		{
			// we need to keep track of how many buffers need to be clear but have not yet
			mBuffersQueuedForClear+= inCount;
		}
		
		if (GetQLength() == 0)
			mSourceType = AL_UNDETERMINED;	// Q has been cleared and is now available for both static or streaming usage

	}
	catch (OSStatus	 result) {
		DebugMessageN1("	REMOVE BUFFER FAILED, OALSource = %ld\n", (long int) mSelfToken);
		throw (result);
	}

#if LOG_BUFFER_QUEUEING
	DebugMessageN2("	OALSource:RemoveBuffersFromQueue called (END) - OALSource:QLength = %ld:%ld", (long int)mSelfToken, mBufferQueueInactive->Size() + mBufferQueueActive->Size());
#endif

	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Called Only From The Post Render Proc
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::PostRenderRemoveBuffersFromQueue(UInt32	inBuffersToUnqueue)
{

#if LOG_BUFFER_QUEUEING
	DebugMessageN2("OALSource::PostRenderRemoveBuffersFromQueue called (START) - OALSource:inCount = %ld:%ld", (long int)mSelfToken, inBuffersToUnqueue);
#endif
	
	try {
		ClearActiveQueue();

		if (inBuffersToUnqueue > mBufferQueueInactive->Size())
		{
			DebugMessageN3("	OALSource::PostRenderRemoveBuffersFromQueue mState == AL_STOPPED - OALSource:GetQLength():mBufferQueueInactive->Size() = %d:%d:%d", (int) mSelfToken, (int)GetQLength(), (int)mBufferQueueInactive->Size());
			throw ((OSStatus) AL_INVALID_OPERATION);	
		}
		
		for (UInt32	i = 0; i < inBuffersToUnqueue; i++)
		{	
			mBufferQueueInactive->RemoveQueueEntryByIndex(this, 0, true);
			// we have now cleared all buffers slated for removal, so we can reset the queued for clear index
			mBuffersQueuedForClear = 0;
			//DebugMessageN1("***** OALSource::PostRenderRemoveBuffersFromQueue buffer removed = %ld", bufferToken);
		}
		
		if (GetQLength() == 0)
			mSourceType = AL_UNDETERMINED;	// Q has been cleared and is now available for both static or streaming usage

	}
	catch (OSStatus	 result) {
		DebugMessageN1("	REMOVE BUFFER FAILED, OALSource = %ld\n", (long int) mSelfToken);
		throw (result);
	}

#if LOG_BUFFER_QUEUEING
	DebugMessageN2("	OALSource:PostRenderRemoveBuffersFromQueue called (END) - OALSource:QLength = %ld:%ld\n", (long int)mSelfToken, mBufferQueueInactive->Size() + mBufferQueueActive->Size());
#endif

	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::PostRenderSetBuffer (ALuint inBufferToken, OALBuffer	*inBuffer)
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::PostRenderSetBuffer - OALSource:inBufferToken = %ld:%ld", mSelfToken, inBufferToken);
#endif

	FlushBufferQueue();		
	
	// if inBufferToken == 0, do nothing, passing NONE to this method is legal and should merely empty the queue
	if (inBufferToken != 0)
	{
		AppendBufferToQueue(inBufferToken, inBuffer);
		mSourceType = AL_STATIC;
	}
	else
		mSourceType = AL_UNDETERMINED;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// PLAYBACK METHODS 
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void	OALSource::SetupMixerBus()
{
	OSStatus					result = noErr;
	CAStreamBasicDescription    desc;
	UInt32						propSize = 0;
	BufferInfo*					buffer = mBufferQueueActive->Get(mCurrentBufferIndex);

	try {
		if (buffer == NULL)
			throw (OSStatus)-1; 

		if (mCurrentPlayBus == kSourceNeedsBus)
		{		
			// the bus stream format will get set if necessary while getting the available bus
			mCurrentPlayBus = (buffer->mBuffer->GetNumberChannels() == 1) ? mOwningContext->GetAvailableMonoBus(mSelfToken) : mOwningContext->GetAvailableStereoBus(mSelfToken);                
			if (mCurrentPlayBus == -1)
				throw (OSStatus) -1; 
			
			if (Get3DMixerVersion() >= k3DMixerVersion_2_0)
			{
				Float32     rollOff = mRollOffFactor;
				Float32     refDistance = mReferenceDistance;
				Float32     maxDistance = mMaxDistance;

				if (mOwningContext->IsDistanceScalingRequired())
				{
					refDistance = (mReferenceDistance/mMaxDistance) * kDistanceScalar;
					maxDistance = kDistanceScalar;
					rollOff *= (kDistanceScalar/mMaxDistance);
				}
				
				Float32	testAttenuation = GetMaxAttenuation(mReferenceDistance, mMaxDistance, mRollOffFactor);
				
				// Set the MixerDistanceParams for the new bus if necessary
				MixerDistanceParams		distanceParams;
				propSize = sizeof(distanceParams);
				result = AudioUnitGetProperty(mOwningContext->GetMixerUnit(), kAudioUnitProperty_3DMixerDistanceParams, kAudioUnitScope_Input, mCurrentPlayBus, &distanceParams, &propSize);

				if  ((result == noErr) 	&& ((distanceParams.mReferenceDistance != refDistance)      ||
											(distanceParams.mMaxDistance != maxDistance)            ||
											(distanceParams.mMaxAttenuation != testAttenuation)))
				{
					distanceParams.mMaxAttenuation = testAttenuation;
				
					if (mOwningContext->IsDistanceScalingRequired())
					{

						distanceParams.mReferenceDistance = (mReferenceDistance/mMaxDistance) * kDistanceScalar;
						// limit the max distance
						distanceParams.mMaxDistance = kDistanceScalar;
						distanceParams.mMaxAttenuation = testAttenuation;
					}
					else
					{
						distanceParams.mReferenceDistance = mReferenceDistance;
						distanceParams.mMaxDistance = mMaxDistance;
					}
					
					if ((mReferenceDistance == mMaxDistance) && (Get3DMixerVersion() < k3DMixerVersion_2_2))
						distanceParams.mMaxDistance = distanceParams.mReferenceDistance + .01; // pre 2.2 3DMixer may crash  if max and reference distances are equal

					result = AudioUnitSetProperty(mOwningContext->GetMixerUnit(), kAudioUnitProperty_3DMixerDistanceParams, kAudioUnitScope_Input, mCurrentPlayBus, &distanceParams, sizeof(distanceParams));
				}
			}
			else
			{
				// the pre-2.0 3DMixer does not accept kAudioUnitProperty_3DMixerDistanceParams, it has do some extra work and use the DistanceAtten property instead
				mOwningContext->SetDistanceAttenuation(mCurrentPlayBus, mReferenceDistance, mMaxDistance, mRollOffFactor);
			}
			
			UpdateBusReverb();
			UpdateBusOcclusion();
			UpdateBusObstruction();
		}

		// get the sample rate of the bus
		propSize = sizeof(desc);
		result = AudioUnitGetProperty(mOwningContext->GetMixerUnit(), kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, mCurrentPlayBus, &desc, &propSize);
		
		mResetPitch = true;	
		mCalculateDistance = true;				
		if (desc.mSampleRate != buffer->mBuffer->GetSampleRate())
			mResetBusFormat = true;     // only reset the bus stream format if it is different than sample rate of the data   		
		
		// *************************** Set properties for the mixer bus
		
		ChangeChannelSettings();
		UpdateBusGain();
	}
	catch (OSStatus	 result) {
		DebugMessageN2("	SetupMixerBus FAILED, OALSource:error = %d:%d\n", (int) mSelfToken, (int)result);
		throw (result);
	}
	catch (...) {
		DebugMessageN1("	SetupMixerBus FAILED, OALSource = %ld\n", (long int) mSelfToken);
		throw;
	}
	
	return;			
}

void OALSource::SetupRogerBeepAU()
{
		
	ComponentDescription desc;
	desc.componentFlags = 0;        
	desc.componentFlagsMask = 0;     
	desc.componentType = kAudioUnitType_Effect;          
	desc.componentSubType = kRogerBeepType;       
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;  

	// CREATE NEW NODE FOR THE GRAPH
	OSStatus result = AUGraphNewNode (mOwningContext->GetGraph(), &desc, 0, NULL, &mRogerBeepNode);
		THROW_RESULT

	result = AUGraphGetNodeInfo (mOwningContext->GetGraph(), mRogerBeepNode, 0, 0, 0, &mRogerBeepAU);
		THROW_RESULT   
}

void OALSource::SetupDistortionAU()
{
	ComponentDescription desc;
	desc.componentFlags = 0;        
	desc.componentFlagsMask = 0;     
	desc.componentType = kAudioUnitType_Effect;          
	desc.componentSubType = kDistortionType;       
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;  

	// CREATE NEW NODE FOR THE GRAPH
	OSStatus result = AUGraphNewNode (mOwningContext->GetGraph(), &desc, 0, NULL, &mDistortionNode);
		THROW_RESULT

	result = AUGraphGetNodeInfo (mOwningContext->GetGraph(), mDistortionNode, 0, 0, 0, &mDistortionAU);
		THROW_RESULT   
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// returns false if there is no data to play
bool	OALSource::PrepBufferQueueForPlayback()
{
    BufferInfo		*buffer = NULL;

	JoinBufferLists();
	
	if (mBufferQueueActive->Empty())
		return false; // there isn't anything to do
		
	// Get the format for the first buffer in the queue
	buffer = NextPlayableBufferInActiveQ();
	if (buffer == NULL)
		return false; // there isn't anything to do
	
#if LOG_PLAYBACK
	DebugMessage("OALSource::PrepBufferQueueForPlayback called - Format of 1st buffer in the Q =\n");
	buffer->mBuffer->PrintFormat();
#endif
			
	// WARM THE BUFFERS
	// when playing, touch all the audio data in memory once before it is needed in the render proc  (RealTime thread)
	{
		volatile UInt32	X;
		UInt32		*start = (UInt32 *)buffer->mBuffer->GetDataPtr();
		UInt32		*end = (UInt32 *)(buffer->mBuffer->GetDataPtr() + (buffer->mBuffer->GetDataSize() &0xFFFFFFFC));
		while (start < end)
		{
			X = *start; 
			start += 1024;
		}
	}		
		
	if (buffer->mBuffer->HasBeenConverted() == false)
		AudioConverterReset(mACMap->Get(buffer->mACToken));

	return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::Play()
{			
	try {
		// wait if in render, then prevent rendering til completion
		OALRenderLocker::RenderLocker locked(mRenderLocker, InRenderThread());

		// don't allow synchronous source manipulation
		CAGuard::Locker sourceLock(mSourceLock);

		if (GetQLength() == 0)
			return; // nothing to do
        
		switch (mState)
		{
			case AL_PLAYING:
#if LOG_PLAYBACK
				DebugMessageN3("OALSource::Play Deferred Rewind - OALSource:QSize:Looping = %ld:%ld:%ld", (long int) mSelfToken, mBufferQueueActive->Size(), (long int) mLooping);
#endif
				if (mRampState != kRampingComplete)	
				{
					mRampState = kRampDown;
#if	LOG_MESSAGE_QUEUE					
					DebugMessageN1("OALSource::Play (AL_PLAYING state)  - kMQ_Rewind added to MQ - OALSource = %ld", (long int) mSelfToken);
#endif
					AddPlaybackMessage(kMQ_Rewind, NULL, 0);
			 	}
				break;
			
			case AL_PAUSED:
#if LOG_PLAYBACK
				DebugMessage("OALSource::Play Resuming");
#endif
				Resume();
				break;
			
			case kTransitionState:
#if LOG_PLAYBACK
				DebugMessageN3("OALSource::Play Deferred Play - OALSource:QSize:Looping = %ld:%ld:%ld", (long int) mSelfToken, mBufferQueueActive->Size(), (long int) mLooping);
#endif
				if (mRampState != kRampingComplete)
				{
					mRampState = kRampDown;
#if	LOG_MESSAGE_QUEUE					
					DebugMessageN1("OALSource::Play (kTransitionState state)  - kMQ_Play added to MQ - OALSource = %ld", (long int) mSelfToken);
#endif
					AddPlaybackMessage(kMQ_Play, NULL, 0);
				}
				break;
			
			default:
			{								
#if LOG_PLAYBACK
				DebugMessageN3("OALSource::Play Starting from a Stopped or Initial state (BEFORE PREP) - OALSource:QSize:Looping = %ld:%ld:%ld", (long int) mSelfToken,mBufferQueueActive->Size(), mLooping);
#endif
				// get the buffer q in a ready state for playback
				PrepBufferQueueForPlayback();	
#if LOG_PLAYBACK
				DebugMessageN3("OALSource::Play Starting from a Stopped or Initial state (AFTER PREP) - OALSource:QSize:Looping = %ld:%ld:%ld", (long int) mSelfToken,mBufferQueueActive->Size(), mLooping);
#endif
				
				// set up a mixer bus now
				SetupMixerBus();
				CAStreamBasicDescription	format;
				UInt32                      propSize = sizeof(format);
				OSStatus result = noErr;
				
				if(mASARogerBeepEnable || mASADistortionEnable)
				{	
					mRenderElement = 0;
					result = AudioUnitGetProperty(mOwningContext->GetMixerUnit(), kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, mCurrentPlayBus, &format, &propSize);
						THROW_RESULT	
								
					if(mASARogerBeepEnable)
					{
						// set AU format to mixer format for input/output
						result = AudioUnitSetProperty (mRogerBeepAU, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &format, sizeof(format));
							THROW_RESULT
						result = AudioUnitSetProperty (mRogerBeepAU, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &format, sizeof(format));
							THROW_RESULT
						result = AudioUnitInitialize(mRogerBeepAU);
							THROW_RESULT
						// connect roger beep AU to 3D Mixer
						result = AUGraphConnectNodeInput(mOwningContext->GetGraph(), mRogerBeepNode, 0, mOwningContext->GetMixerNode(), mCurrentPlayBus);
							THROW_RESULT	
						
						if(!mASADistortionEnable)
						{
							// connect render proc to unit if distortion is not enabled
							result = AUGraphGetNodeInfo (mOwningContext->GetGraph(), mRogerBeepNode, 0, 0, 0, &mRenderUnit);
								THROW_RESULT;
						}
					}
					
					if(mASADistortionEnable)
					{

						// set AU format to mixer format for input/output
						result = AudioUnitSetProperty (mDistortionAU, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &format, sizeof(format));
							THROW_RESULT
						result = AudioUnitSetProperty (mDistortionAU, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &format, sizeof(format));
							THROW_RESULT
						result = AudioUnitInitialize(mDistortionAU);
							THROW_RESULT
						
						// distortion unit will always be first if it exists
						result = AUGraphGetNodeInfo (mOwningContext->GetGraph(), mDistortionNode, 0, 0, 0, &mRenderUnit);
							THROW_RESULT
						
						if(mASARogerBeepEnable)
							result = AUGraphConnectNodeInput(mOwningContext->GetGraph(), mDistortionNode, 0, mRogerBeepNode, 0);
						else
							result = AUGraphConnectNodeInput(mOwningContext->GetGraph(), mDistortionNode, 0, mOwningContext->GetMixerNode(), mCurrentPlayBus);
								
							THROW_RESULT
					}
					
					result = AUGraphUpdate(mOwningContext->GetGraph(), NULL);
						THROW_RESULT
				}	
				else
				{
					mRenderUnit = mOwningContext->GetMixerUnit();
					mRenderElement = mCurrentPlayBus;
				}
					THROW_RESULT
				
				mState = AL_PLAYING;
				mQueueIsProcessed = false;
				// attach the notify and render procs to the first unit in the sequence
				AddNotifyAndRenderProcs();
			}
		}
	}
	catch (OSStatus	result) {
		DebugMessageN2("PLAY FAILED source = %ld, err = %ld\n", (long int) mSelfToken, (long int)result);
		throw (result);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::Rewind()
{
#if LOG_PLAYBACK
	DebugMessageN1("OALSource::Pause called - OALSource = %ld", (long int) mSelfToken);
#endif

	// wait if in render, then prevent rendering til completion
	OALRenderLocker::RenderLocker locked(mRenderLocker, InRenderThread());

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

	switch (mState)
	{
		case AL_PLAYING:
		{
			if (mRampState != kRampingComplete)	
				mRampState = kRampDown;
#if	LOG_MESSAGE_QUEUE					
			DebugMessageN1("Rewind(AL_PLAYING)  ADDING : kMQ_Rewind - OALSource = %ld", (long int) mSelfToken);
#endif
			AddPlaybackMessage(kMQ_Rewind, NULL, 0);
		}
		case AL_PAUSED:
		case AL_STOPPED:
			LoopToBeginning();	// just reset the buffer queue now
			break;
		case kTransitionState:
		case AL_INITIAL:
			break;
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::Pause()
{
#if LOG_PLAYBACK
	DebugMessageN1("OALSource::Pause called - OALSource = %ld", (long int) mSelfToken);
#endif

	// wait if in render, then prevent rendering til completion
	OALRenderLocker::RenderLocker locked(mRenderLocker, InRenderThread());

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

	switch (mState)
	{
		case AL_PLAYING:
		{
			if (mRampState != kRampingComplete)	
				mRampState = kRampDown;
#if	LOG_MESSAGE_QUEUE					
				DebugMessageN1("Pause(AL_PLAYING)  ADDING : kMQ_Pause - OALSource = %ld", (long int) mSelfToken);
#endif
			AddPlaybackMessage(kMQ_Pause, NULL, 0);
		}
		case AL_INITIAL:
		case AL_STOPPED:
		case AL_PAUSED:
		case kTransitionState:
			break;
				
		default:
			// do nothing it's either stopped or initial right now
			break;
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::Resume()
{
#if LOG_PLAYBACK
	DebugMessageN1("OALSource::Resume called - OALSource = %ld", (long int) mSelfToken);
#endif

	// wait if in render, then prevent rendering til completion
	OALRenderLocker::RenderLocker locked(mRenderLocker, InRenderThread());

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

	switch (mState)
	{
		case AL_PLAYING:
		case AL_INITIAL:
		case AL_STOPPED:
		case kTransitionState:
			break;

		case AL_PAUSED:
			mRampState = kRampUp;
			AddNotifyAndRenderProcs();
			mState = AL_PLAYING;
			break;
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::Stop()
{
#if LOG_PLAYBACK
	DebugMessageN2("OALSource::Stop called - OALSource = %ld : mState = %ld\n", (long int) mSelfToken, mState);
#endif

	// wait if in render, then prevent rendering til completion
	OALRenderLocker::RenderLocker locked(mRenderLocker, InRenderThread());

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

	switch (mState)
	{
		case AL_PAUSED:
		if (mCurrentPlayBus != kSourceNeedsBus)
		{
			mOwningContext->SetBusAsAvailable (mCurrentPlayBus);
			mCurrentPlayBus = kSourceNeedsBus;
			mState = AL_STOPPED;
		}
			break;
		case AL_PLAYING:
		{
#if	LOG_MESSAGE_QUEUE					
			DebugMessageN1("OALSource::Stop (AL_PLAYING state)  ADDING : kMQ_Stop - OALSource = %ld", (long int) mSelfToken);
#endif
			mState = kTransitionState;
			if (mRampState != kRampingComplete)	
				mRampState = kRampDown;
			AddPlaybackMessage(kMQ_Stop, NULL, 0);
		}
			break;
		
		case kTransitionState:
		{
#if	LOG_MESSAGE_QUEUE					
			DebugMessageN1("OALSource::Stop (kTransitionState state)  ADDING : kMQ_Stop - OALSource = %ld", (long int) mSelfToken);
#endif
			AddPlaybackMessage(kMQ_Stop, NULL, 0);
		}
			break;
				
		default:
			// do nothing it's either stopped or initial right now
			break;
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UInt32	OALSource::SecondsToFrames(Float32	inSeconds)
{
	BufferInfo	*curBufferInfo = mBufferQueueActive->Get(mCurrentBufferIndex);
	if (curBufferInfo)
		return (UInt32)(inSeconds * curBufferInfo->mBuffer->GetSampleRate());

	return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// this method should round off to the packet index that contains the requested frame offset
UInt32	OALSource::BytesToFrames(Float32	inBytes)
{
	BufferInfo	*curBufferInfo = mBufferQueueActive->Get(mCurrentBufferIndex);
	if (curBufferInfo)
	{
		UInt32	frameIndex = (UInt32)(inBytes / curBufferInfo->mBuffer->GetBytesPerPacket());	// how many packets is this
		frameIndex *= curBufferInfo->mBuffer->GetFramesPerPacket();					// translate packets to frames
		return frameIndex;
	}
	
	return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UInt32	OALSource::FramesToSeconds(UInt32	inFrames)
{
	BufferInfo	*curBufferInfo = mBufferQueueActive->Get(mCurrentBufferIndex);
	if (curBufferInfo)
		return (UInt32)(inFrames / curBufferInfo->mBuffer->GetSampleRate());

	return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UInt32	OALSource::FramesToBytes(UInt32	inFrames)
{
	// any buffer in the q will do, they should all be the same
	BufferInfo	*curBufferInfo = mBufferQueueActive->Get(mCurrentBufferIndex);
	if (curBufferInfo)
		return (inFrames / curBufferInfo->mBuffer->GetFramesPerPacket()) * curBufferInfo->mBuffer->GetBytesPerPacket();
	
	return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UInt32	OALSource::GetQueueOffset(UInt32	inOffsetType)
{
	CAGuard::Locker sourceLock(mSourceLock);

	UInt32		inInactiveQFrames = mBufferQueueInactive->GetQueueSizeInFrames();
	UInt32		inActiveQFrames = 0;

	for (UInt32	i = 0; i < mCurrentBufferIndex; i++)
	{
		inActiveQFrames += mBufferQueueActive->GetBufferFrameCount(i);
	}

	inActiveQFrames += mBufferQueueActive->GetCurrentFrame(mCurrentBufferIndex);

	switch (inOffsetType)
	{
		case kSecondsOffset:
			return (FramesToSeconds(inInactiveQFrames + inActiveQFrames));
			break;
		case kSampleOffset:
			return (inInactiveQFrames + inActiveQFrames);
			break;
		case kByteOffset:
			return (FramesToBytes(inInactiveQFrames + inActiveQFrames));
			break;
		default:
			break;
	}
	
	return  0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetQueueOffset(UInt32	inOffsetType,	Float32	inOffset)
{
#if LOG_PLAYBACK
	DebugMessageN1("OALSource::SetQueueOffset - OALSource = %ld", (long int) mSelfToken);
#endif
	UInt32		frameOffset = 0;

	// wait if in render, then prevent rendering til completion
	OALRenderLocker::RenderLocker locked(mRenderLocker, InRenderThread());

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);
	
	// first translate the entered offset into a frame offset
	switch (inOffsetType)
	{
		case kSecondsOffset:
			frameOffset = SecondsToFrames(inOffset);
			break;
		case kSampleOffset:
			frameOffset = (UInt32) inOffset;	// samples = frames in this case
			break;
		case kByteOffset:
			frameOffset = BytesToFrames(inOffset);
			break;
		default:
			return;
			break;
	}

	// move the queue to the requested offset either right now, or in the PostRender proc via a queued message
	switch (mState)
	{
		case AL_INITIAL:
		case AL_STOPPED:
		case AL_PAUSED:
		{
			Rewind();	// join the active and inactive lists				
			AdvanceQueueToFrameIndex(frameOffset);
		}
			break;
			
		case AL_PLAYING:
		case kTransitionState:
		{
			mRampState = kRampDown;			
			// currently, no consideration for ramping the samples is being taken, so there could be clicks if playhead is moved during a playback
			mPlaybackHeadPosition = frameOffset;
			AddPlaybackMessage(kMQ_SetFramePosition, NULL, 0);
		}
			break;
				
		default:
			break;
	}
}	

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// this should only be called when the source is render and edit protected
void	OALSource::AdvanceQueueToFrameIndex(UInt32	inFrameOffset)
{
	// Queue should be in an initial state right now.
	UInt32	totalQueueFrames = mBufferQueueActive->GetQueueSizeInFrames();
	if (inFrameOffset > totalQueueFrames)
		throw AL_INVALID_VALUE;
	
	// Walk through the buffers until we reach the one that contains this offset. Mark all preceeding buffers as processed and move to inActive Queue
		
	UInt32		frameStartOfCurrentBuffer = 0;
	
	UInt32		count = mBufferQueueActive->Size();
	for (UInt32	i = 0; i < count; i++)
	{
		BufferInfo*		curBufferInfo = mBufferQueueActive->Get(0);							// get the first buffer info in the Q
		UInt32			bufferFrameCount = curBufferInfo->mBuffer->GetFrameCount();			// how many frames of data are in this buffer?
		
		if (frameStartOfCurrentBuffer + bufferFrameCount > inFrameOffset)
		{
			// this is the buffer that contains the desired offset
			mBufferQueueActive->SetFirstBufferOffset(inFrameOffset - frameStartOfCurrentBuffer) ;
			break; // we're done
		}
		else
		{
			// not there yet. Mark this buffer as processed and move to inactive queue
			mBufferQueueActive->SetBufferAsProcessed(0);
			mBufferQueueActive->RemoveQueueEntryByIndex(this, 0, false);			
			mBufferQueueInactive->AppendBuffer(this, curBufferInfo->mBufferToken, curBufferInfo->mBuffer, curBufferInfo->mACToken);
		}
		frameStartOfCurrentBuffer += bufferFrameCount;
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ***** PRIVATE *****
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::AddPlaybackMessage(UInt32 inMessage, OALBuffer* inDeferredAppendBuffer, UInt32	inBuffersToUnqueue)
{
	PlaybackMessage*		pbm = new PlaybackMessage((UInt32) inMessage, inDeferredAppendBuffer, inBuffersToUnqueue);
	mMessageQueue.push_atomic(pbm);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::ClearMessageQueue()
{
	PlaybackMessage	*messages = mMessageQueue.pop_all_reversed();
	while (messages != NULL)
	{
		PlaybackMessage	*lastMessage = messages;
		messages = messages->get_next();
		delete (lastMessage); // made it, so now get rid of it
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ***** Buffer Queue Methods *****
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void	OALSource::ClearActiveQueue()
{
	while (mBufferQueueActive->Size() > 0)
	{
		// Get buffer #i from Active List
		BufferInfo	*staleBufferInfo = mBufferQueueActive->Get(0);
		mBufferQueueActive->SetBufferAsProcessed(0);
		// Append it to Inactive List
		mBufferQueueInactive->AppendBuffer(this, staleBufferInfo->mBufferToken, staleBufferInfo->mBuffer, staleBufferInfo->mACToken);
		// Remove it from Active List
		mBufferQueueActive->RemoveQueueEntryByIndex(this, 0, false);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// this method should only be called when a looping queue reaches it's end and needs to start over (called from render thread)
void	OALSource::LoopToBeginning()
{
#if LOG_VERBOSE
	DebugMessageN1("OALSource::LoopToBeginning called - OALSource = %ld\n", (long int) mSelfToken);
#endif

	ClearActiveQueue();
	
	// swap the list pointers now
    BufferQueue*        tQ  = mBufferQueueActive;
    mBufferQueueActive  = mBufferQueueInactive;
    mBufferQueueInactive = tQ;

    mBufferQueueActive->ResetBuffers(); 	// mark all the buffers as unprocessed
    mCurrentBufferIndex = 0;                // start at the first buffer in the queue
    mQueueIsProcessed = false;					
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// this method should only be called from a non playing state and is used to rejoin the 2 buffer Q lists
void	OALSource::JoinBufferLists()
{	
#if LOG_VERBOSE
	DebugMessageN1("OALSource::JoinBufferLists called - OALSource = %ld\n", (long int) mSelfToken);
#endif

	UInt32  count = mBufferQueueActive->Size();
	for (UInt32 i = 0; i < count; i++)
	{
		// Get buffer #i from Active List
		BufferInfo	*staleBufferInfo = mBufferQueueActive->Get(mCurrentBufferIndex);
		if (staleBufferInfo)
		{
			mBufferQueueActive->SetBufferAsProcessed(mCurrentBufferIndex);
			// Append it to Inactive List
			mBufferQueueInactive->AppendBuffer(this, staleBufferInfo->mBufferToken, staleBufferInfo->mBuffer, staleBufferInfo->mACToken);
			// Remove it from Active List
			mBufferQueueActive->RemoveQueueEntryByIndex(this, mCurrentBufferIndex, false);
		}
	}
	
	// swap the list pointers now
	BufferQueue*        tQ  = mBufferQueueActive;
	mBufferQueueActive  = mBufferQueueInactive;
	mBufferQueueInactive = tQ;
	
	mBufferQueueActive->ResetBuffers(); 	// mark all the buffers as unprocessed
	mCurrentBufferIndex = 0;                // start at the first buffer in the queue
	mQueueIsProcessed = false;					
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// this is ONLY called from the render thread
void OALSource::UpdateQueue ()
{
	if (mCurrentBufferIndex > 0)
    {
		BufferInfo			*bufferInfo = NULL;
		for (UInt32 i = 0; i < mCurrentBufferIndex; i++)
		{
			// Get buffer #i from Active List
			bufferInfo = mBufferQueueActive->Get(0);
			if (bufferInfo)
			{
				// Append it to Inactive List
				mBufferQueueInactive->AppendBuffer(this, bufferInfo->mBufferToken, bufferInfo->mBuffer, bufferInfo->mACToken);
				// Remove it from Active List
				mBufferQueueActive->RemoveQueueEntryByIndex(this, 0, false);
			}
		}
		mCurrentBufferIndex = 0;
    }    
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool	OALSource::IsSourceTransitioningToFlushQ()
{
	return mTransitioningToFlushQ;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ***** Mixer Bus Methods *****
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::DisconnectFromBus()
{
	try {
		ReleaseNotifyAndRenderProcs();
		OSStatus result = noErr;

		// remove the connection between the AUs, if they exist
		if(mRogerBeepNode || mDistortionNode)
		{
			if(mRogerBeepNode && mDistortionNode)
				result = AUGraphDisconnectNodeInput(mOwningContext->GetGraph(), mRogerBeepNode, 0);
			result = AUGraphDisconnectNodeInput(mOwningContext->GetGraph(), mOwningContext->GetMixerNode(), mCurrentPlayBus);
			result = AUGraphUpdate(mOwningContext->GetGraph(), NULL);
		}	
			
		if (mCurrentPlayBus != kSourceNeedsBus)
		{
			mOwningContext->SetBusAsAvailable (mCurrentPlayBus);
			mCurrentPlayBus = kSourceNeedsBus;		
		}
	}
	catch (OSStatus	result) {
		// swallow the error
	}
}	

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// must be called when the source has a mixer bus
void	OALSource::ChangeChannelSettings()
{
#if CALCULATE_POSITION

	bool	coneGainChange = false;
	bool	attenuationGainChange = false;
	
	if (mCalculateDistance == true)
	{
        BufferInfo	*bufferInfo = mBufferQueueActive->Get(mCurrentBufferIndex);	
		if (bufferInfo)
		{		
#if LOG_GRAPH_AND_MIXER_CHANGES
	DebugMessageN1("OALSource::ChangeChannelSettings: k3DMixerParam_Azimuth/k3DMixerParam_Distance called - OALSource = %ld\n", mSelfToken);
#endif
			// only calculate position if sound is mono - stereo sounds get no location changes
			if ( bufferInfo->mBuffer->GetNumberChannels() == 1)
			{
				Float32 	rel_azimuth, rel_distance, rel_elevation, dopplerShift;
				
				CalculateDistanceAndAzimuth(&rel_distance, &rel_azimuth, &rel_elevation, &dopplerShift);

                if (dopplerShift != mDopplerScaler)
                {
                    mDopplerScaler = dopplerShift;
                    mResetPitch = true;
                }
								
				// set azimuth
				AudioUnitSetParameter(mOwningContext->GetMixerUnit(), k3DMixerParam_Azimuth, kAudioUnitScope_Input, mCurrentPlayBus, rel_azimuth, 0);
				// set elevation
				AudioUnitSetParameter(mOwningContext->GetMixerUnit(), k3DMixerParam_Elevation, kAudioUnitScope_Input, mCurrentPlayBus, rel_elevation, 0);

				mAttenuationGainScaler = 1.0;
				if (!mOwningContext->DoSetDistance())
				{
					AudioUnitSetParameter(mOwningContext->GetMixerUnit(), k3DMixerParam_Distance, kAudioUnitScope_Input, mCurrentPlayBus, mReferenceDistance, 0);///////

					// If 1.3-2.1 Mixer AND it's Linear, Exponential DO calculate Gain scaler - DO NOT  set distance
					switch (mOwningContext->GetDistanceModel())
					{
						case AL_LINEAR_DISTANCE:
						case AL_LINEAR_DISTANCE_CLAMPED:							
							if (mOwningContext->GetDistanceModel() == AL_LINEAR_DISTANCE_CLAMPED)
							{
								if (rel_distance < mReferenceDistance) rel_distance = mReferenceDistance;
								if (rel_distance > mMaxDistance) rel_distance = mMaxDistance;
							}
							mAttenuationGainScaler = (1 - mRollOffFactor * (rel_distance - mReferenceDistance) / (mMaxDistance-mReferenceDistance));
							DebugMessageN1("AL_LINEAR_DISTANCE scaler =  %f2\n", mAttenuationGainScaler);
							attenuationGainChange = true;
							break;
							
						case AL_EXPONENT_DISTANCE:
						case AL_EXPONENT_DISTANCE_CLAMPED:
							if (mOwningContext->GetDistanceModel() == AL_EXPONENT_DISTANCE_CLAMPED)
							{
								if (rel_distance < mReferenceDistance) rel_distance = mReferenceDistance;
								if (rel_distance > mMaxDistance) rel_distance = mMaxDistance;
							}
							mAttenuationGainScaler = pow((rel_distance / mReferenceDistance), (-mRollOffFactor));
							DebugMessageN1("AL_EXPONENT_DISTANCE scaler =  %f2\n", mAttenuationGainScaler);
							attenuationGainChange = true;
							break;
							
						case AL_NONE:
							mAttenuationGainScaler = 1.0;
							break;	// nothing to do
					}
				}
				else
				{
					// if 2.0 and Inverse, SCALE before setting distance
					if (mOwningContext->IsDistanceScalingRequired())	// only true for 2.0 mixer doing inverse curve
						rel_distance *= (kDistanceScalar/mMaxDistance);
				
					// set distance
					AudioUnitSetParameter(mOwningContext->GetMixerUnit(), k3DMixerParam_Distance, kAudioUnitScope_Input, mCurrentPlayBus, rel_distance, 0);
				}
				
				// Source Cone Support Here
				coneGainChange = ConeAttenuation();
			}
		}
		
		mCalculateDistance = false;
	}
		
#endif	// end CALCULATE_POSITION

	UpdateBusGain();

	SetPitch (mPitch);
    UpdateBusFormat();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::UpdateBusGain ()
{
	if (mCurrentPlayBus != kSourceNeedsBus)
	{
		Float32		busGain = mGain;
		
		busGain *= mConeGainScaler;
		busGain *= mAttenuationGainScaler;
		
		// figure out which gain to actually use
		if (mMinGain > busGain)
			busGain = mMinGain;
		else if (mMaxGain < busGain)
			busGain = mMaxGain;
		
		// clamp the gain used to 0.0-1.0
        if (busGain > 1.0)
			busGain = 1.0;
		else if (busGain < 0.0)
			busGain = 0.0;
	
		mOutputSilence = busGain > 0.0 ? false : true;

		if (busGain > 0.0)
		{
			Float32	db = 20.0 * log10(busGain); 	// convert to db
			if (db < -120.0)
				db = -120.0;						// clamp minimum audible level at -120db
			
//#if LOG_GRAPH_AND_MIXER_CHANGES
//	DebugMessageN3("OALSource::UpdateBusGain: k3DMixerParam_Gain called - OALSource:busGain:db = %ld:%f2:%f2\n", mSelfToken, busGain, db );
//#endif

			OSStatus	result = AudioUnitSetParameter (	mOwningContext->GetMixerUnit(), k3DMixerParam_Gain, kAudioUnitScope_Input, mCurrentPlayBus, db, 0);
				THROW_RESULT
		}
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::UpdateBusFormat ()
{
#if LOG_VERBOSE
		DebugMessageN1("OALSource::UpdateBusFormat - OALSource = %ld\n", (long int) mSelfToken);
#endif

 	if (Get3DMixerVersion() < k3DMixerVersion_2_0)	// the pre-2.0 3DMixer cannot change stream formats once initialized
		return;
		
    if (mResetBusFormat)
    {
        CAStreamBasicDescription    desc;
        UInt32  propSize = sizeof(desc);
        OSStatus result = AudioUnitGetProperty(mOwningContext->GetMixerUnit(), kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, mCurrentPlayBus, &desc, &propSize);
        if (result == noErr)
        {
            BufferInfo	*buffer = mBufferQueueActive->Get(mCurrentBufferIndex);	
            if (buffer != NULL)
            {
                desc.mSampleRate = buffer->mBuffer->GetSampleRate();
                AudioUnitSetProperty(mOwningContext->GetMixerUnit(), kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, mCurrentPlayBus, &desc, sizeof(desc));
                mResetBusFormat = false;
            }
        }
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::UpdateBusReverb ()
{
#if LOG_VERBOSE
        DebugMessageN1("OALSource::UpdateBusReverb - OALSource = %ld\n", mSelfToken);
#endif

	if (mCurrentPlayBus != kSourceNeedsBus)
	{
		if (mOwningContext->GetReverbState() == 0)	// either reverb is off or not available on this system
			return;     

		AudioUnitSetParameter(mOwningContext->GetMixerUnit(), 5 /*k3DMixerParam_ReverbBlend*/, kAudioUnitScope_Input, mCurrentPlayBus, mASAReverbSendLevel * 100.0, 0);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::UpdateBusOcclusion ()
{
#if LOG_VERBOSE
        DebugMessageN1("OALSource::UpdateBusOcclusion - OALSource = %ld\n", mSelfToken);
#endif

	if (mCurrentPlayBus != kSourceNeedsBus)
	{
		if (Get3DMixerVersion() < k3DMixerVersion_2_2)	// the pre-2.2 3DMixer does not have occlusion
			return;
		
		AudioUnitSetParameter(mOwningContext->GetMixerUnit(), 7 /*k3DMixerParam_OcclusionAttenuation*/, kAudioUnitScope_Input, mCurrentPlayBus, mASAOcclusion, 0);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::UpdateBusObstruction ()
{
#if LOG_VERBOSE
        DebugMessageN1("OALSource::UpdateBusObstruction - OALSource = %ld\n", mSelfToken);
#endif

	if (mCurrentPlayBus != kSourceNeedsBus)
	{
		if (Get3DMixerVersion() < k3DMixerVersion_2_2)	// the pre-2.2 3DMixer does not have obstruction
			return;
		
		AudioUnitSetParameter(mOwningContext->GetMixerUnit(), 8 /*k3DMixerParam_ObstructionAttenuation*/, kAudioUnitScope_Input, mCurrentPlayBus, mASAObstruction, 0);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ***** Render Proc Methods *****
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::AddNotifyAndRenderProcs()
{
	if (mCurrentPlayBus == kSourceNeedsBus)
		return;
		
	OSStatus	result = noErr;
	
	mPlayCallback.inputProc = SourceInputProc;
	mPlayCallback.inputProcRefCon = this;
	result = AudioUnitSetProperty (	mRenderUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 
							mRenderElement, &mPlayCallback, sizeof(mPlayCallback));	
			THROW_RESULT
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::ReleaseNotifyAndRenderProcs()
{
	OSStatus	result = noErr;
	
	mPlayCallback.inputProc = 0;
	mPlayCallback.inputProcRefCon = 0;

	result = AudioUnitSetProperty (	mRenderUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 
							mRenderElement, &mPlayCallback, sizeof(mPlayCallback));	
		THROW_RESULT
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#if USE_AU_TRACER
static UInt32 tracerStart = 0xca3d0000;
static UInt32 tracerEnd = 0xca3d0004;
#include <sys/syscall.h>
#include <unistd.h>
#endif
OSStatus	OALSource::SourceInputProc (	void 						*inRefCon, 
											AudioUnitRenderActionFlags 	*inActionFlags,
											const AudioTimeStamp 		*inTimeStamp, 
											UInt32 						inBusNumber,
											UInt32 						inNumberFrames, 
											AudioBufferList 			*ioData)
{
	OALSource* THIS = (OALSource*)inRefCon;
	
	THIS->SetInUseFlag();
	
	if (THIS->mOutputSilence)
		*inActionFlags |= kAudioUnitRenderAction_OutputIsSilence;	
	else
		*inActionFlags &= 0xEF; // the mask for the kAudioUnitRenderAction_OutputIsSilence bit
		
#if USE_AU_TRACER
	syscall(180, tracerStart, inBusNumber, ioData->mNumberBuffers, 0, 0);
#endif

	OSStatus result = noErr;
    if (Get3DMixerVersion() >= k3DMixerVersion_2_0)
        result = THIS->DoRender (ioData);       // normal case
    else
        result = THIS->DoSRCRender (ioData);    // pre 2.0 mixer case

#if USE_AU_TRACER
	syscall(180, tracerEnd, inBusNumber, ioData->mNumberBuffers, 0, 0);
#endif

	THIS->ClearInUseFlag();

	return (result);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus OALSource::DoPreRender ()
{    
	BufferInfo	*bufferInfo = NULL;
	OSStatus	err = noErr;

	OALRenderLocker::RenderTryer tried(mRenderLocker);

	if (!tried.Acquired())
		return err;
	
	bufferInfo = mBufferQueueActive->Get(mCurrentBufferIndex);
	if (bufferInfo == NULL)
    {
        // if there are no messages on the Q by now, then the source will be disconnected and reset in the PostRender Proc
		mQueueIsProcessed = true;
        err = -1;	// there are no buffers
    }

	return (err);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus OALSource::ACComplexInputDataProc	(	AudioConverterRef				inAudioConverter,
												UInt32							*ioNumberDataPackets,
												AudioBufferList					*ioData,
												AudioStreamPacketDescription	**outDataPacketDescription,
												void*							inUserData)
{
	OSStatus		err = noErr;
	OALSource* 		THIS = (OALSource*)inUserData;
    BufferInfo		*bufferInfo = THIS->mBufferQueueActive->Get(THIS->mCurrentBufferIndex);
    UInt32			sourcePacketsLeft = 0;

	if (bufferInfo == NULL)
	{
		ioData->mBuffers[0].mData = NULL;				// return nothing			
		ioData->mBuffers[0].mDataByteSize = 0;			// return nothing
		*ioNumberDataPackets = 0;
		return -1;
	}
	
	sourcePacketsLeft = (bufferInfo->mBuffer->GetDataSize() - bufferInfo->mOffset) / bufferInfo->mBuffer->GetBytesPerPacket();

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// BUFFER EMPTY: If the buffer is empty now, decide on returning an error based on what gets played next in the queue
	if (sourcePacketsLeft == 0)
	{		
		bufferInfo->mProcessedState = kProcessed;	// this buffer is done
		bufferInfo->mOffset = 0;					// will be ready for the next time

		THIS->mCurrentBufferIndex++;
		BufferInfo	*nextBufferInfo = THIS->NextPlayableBufferInActiveQ();

		// see if there is a next buffer or if the queue is looping and should return to the start
		if ((nextBufferInfo != NULL) || (THIS->mLooping == true))
		{
			// either we will loop back to the beginning or will use a new buffer
			if (nextBufferInfo == NULL)
			{
				THIS->LoopToBeginning();
			}
				
			err = OALSourceError_CallConverterAgain;
		}
		else
		{
			// looping is false and there are no more buffers so we are really out of data
			// return what we have and no error, the AC should then be reset in the RenderProc
			// and what ever data is in the AC should get returned
			THIS->mBufferQueueActive->SetBufferAsProcessed(THIS->mCurrentBufferIndex);
			THIS->mQueueIsProcessed = true;		// we are done now, the Q is dry            
		}

		ioData->mBuffers[0].mData = NULL;				// return nothing			
		ioData->mBuffers[0].mDataByteSize = 0;			// return nothing
		*ioNumberDataPackets = 0;
	}
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// BUFFER HAS DATA
	else
	{
		// return the entire request or the remainder of the buffer
		if (sourcePacketsLeft < *ioNumberDataPackets)
			*ioNumberDataPackets = sourcePacketsLeft;
		
		ioData->mBuffers[0].mData = bufferInfo->mBuffer->GetDataPtr() + bufferInfo->mOffset;	// point to the data we are providing		
		ioData->mBuffers[0].mDataByteSize = *ioNumberDataPackets * bufferInfo->mBuffer->GetBytesPerPacket();
		bufferInfo->mOffset += ioData->mBuffers[0].mDataByteSize;
	}

	return (err);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Find the next playable buffer in the Active Buffer Q
BufferInfo*  OALSource::NextPlayableBufferInActiveQ()// this method updates mCurrentBufferIndex as well	
{
	// try and walk through the active buffer list
	BufferInfo*		bufferInfo = NULL;
	bool			done = false;
		
	while (!done)
	{
		bufferInfo = mBufferQueueActive->Get(mCurrentBufferIndex);
		if (bufferInfo == NULL)
		{
			done = true;  // there are no more valid buffers in this list
        }
		else if (bufferInfo->mBufferToken == AL_NONE)
		{
			// mark as processed
			bufferInfo->mProcessedState = kProcessed;	// this buffer is done
			mCurrentBufferIndex++;
		}
		else if (bufferInfo->mBuffer->GetDataPtr() == NULL)
		{
			// mark as processed
			bufferInfo->mProcessedState = kProcessed;	// this buffer is done
			mCurrentBufferIndex++;
		}
		else
			return bufferInfo;
	}
	return NULL;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus OALSource::DoRender (AudioBufferList 			*ioData)
{
	OSStatus   			err = noErr;
	UInt32				packetsRequestedFromRenderProc = ioData->mBuffers[0].mDataByteSize / sizeof(Float32);
	UInt32				packetsObtained = 0;
	UInt32				packetCount;
	struct {
			AudioBufferList	abl;
			AudioBuffer		buffer;
	}t;	
	AudioBufferList	*	tBufferList = (AudioBufferList	*) &t.abl;
	UInt32				dataByteSize = ioData->mBuffers[0].mDataByteSize;
	BufferInfo			*bufferInfo = NULL;

	tBufferList->mNumberBuffers = ioData->mNumberBuffers;
	// buffer 1
	tBufferList->mBuffers[0].mNumberChannels = ioData->mBuffers[0].mNumberChannels;
	tBufferList->mBuffers[0].mDataByteSize = ioData->mBuffers[0].mDataByteSize;
	tBufferList->mBuffers[0].mData = ioData->mBuffers[0].mData;
	if (tBufferList->mNumberBuffers > 1)
	{
		// buffer 2
		tBufferList->mBuffers[1].mNumberChannels = ioData->mBuffers[1].mNumberChannels;
		tBufferList->mBuffers[1].mDataByteSize = ioData->mBuffers[1].mDataByteSize;
		tBufferList->mBuffers[1].mData = ioData->mBuffers[1].mData;
	}
	
	OALRenderLocker::RenderTryer tried(mRenderLocker);

	if (!tried.Acquired()) 
	{
		// source is being edited. can't render, so the ioData buffers must be cleared out 
		for (UInt32	i = 0; i < ioData->mNumberBuffers; i++)
			memset(ioData->mBuffers[i].mData, 0, ioData->mBuffers[i].mDataByteSize);
		return noErr;
	}
		
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 1st move past any AL_NONE Buffers
	
    // if there are no more buffers in the active Q, go back to the beginning if in Loop mode, else pad zeroes and clean up in the PostRender proc
	bufferInfo = NextPlayableBufferInActiveQ();	// this method updates mCurrentBufferIndex as well

	// update the Q lists before returning any data
	UpdateQueue();	

	if ((bufferInfo == NULL) && (mLooping == true))
	{
		// swap the list pointers now
		BufferQueue*        tQ  = mBufferQueueActive;
		mBufferQueueActive  = mBufferQueueInactive;
		mBufferQueueInactive = tQ;
	}
	else if (bufferInfo == NULL)
    {
        // if there are no messages on the Q by now, then the source will be disconnected and reset in the PostRender Proc
        mQueueIsProcessed = true;
        // stop rendering, there is no more data
        goto Finished;	// there are no buffers
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	ChangeChannelSettings();
	
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		
	// walk through as many buffers as needed to satisfy the request AudioConverterFillComplexBuffer will
	// get called each time the data format changes until enough packets have been obtained or the q is empty.
	while ((packetsObtained < packetsRequestedFromRenderProc) && (mQueueIsProcessed == false))
	{
		BufferInfo	*bufferInfo = NextPlayableBufferInActiveQ();	// this method updates mCurrentBufferIndex as well
		if (bufferInfo == NULL)
		{
            // just zero out the remainder of the buffer
			// if there are no messages on the Q by now, then the source will be disconnected and reset in the PostRender Proc
            mQueueIsProcessed = true;
            goto Finished;
        }

		bufferInfo->mProcessedState = kInProgress;
		// buffer 1
		UInt32	byteCount = packetsObtained * sizeof(Float32);
		tBufferList->mBuffers[0].mDataByteSize = dataByteSize - byteCount;
		tBufferList->mBuffers[0].mData = (Byte *) ioData->mBuffers[0].mData + byteCount;
		if (tBufferList->mNumberBuffers > 1)
		{
			// buffer 2
			tBufferList->mBuffers[1].mDataByteSize = tBufferList->mBuffers[0].mDataByteSize;
			tBufferList->mBuffers[1].mData = (Byte *) ioData->mBuffers[1].mData + byteCount;
		}
		
		if (bufferInfo->mBuffer->HasBeenConverted() == false)
		{
			// CONVERT THE BUFFER DATA 
			AudioConverterRef	converter = mACMap->Get(bufferInfo->mACToken);
	
			packetCount = packetsRequestedFromRenderProc - packetsObtained;
			// if OALSourceError_CallConverterAgain is returned, there is nothing to do, just go around again and try and get the data
			err = AudioConverterFillComplexBuffer(converter, ACComplexInputDataProc, this, &packetCount, tBufferList, NULL);
			packetsObtained += packetCount;
	
			if (mQueueIsProcessed == true)
			{
				AudioConverterReset(converter);
			}
			else if ((packetsObtained < packetsRequestedFromRenderProc) && (err == noErr))
			{
				// we didn't get back what we asked for, but no error implies we have used up the data of this format
				// so reset this converter so it will be ready for the next time
				AudioConverterReset(converter);
#if	LOG_VERBOSE
                DebugMessageN1("OALSource::DoRender: Resetting AudioConverter - OALSource = %ld\n", (long int) mSelfToken);
#endif
            }
        }
		else
		{
			// Data has already been converted to the mixer's format, so just do a copy (should be mono only)
			UInt32	bytesRemaining = bufferInfo->mBuffer->GetDataSize() - bufferInfo->mOffset;
			UInt32	framesRemaining = bytesRemaining / sizeof(Float32);
			UInt32	bytesToCopy = 0;
			UInt32	framesToCopy = packetsRequestedFromRenderProc - packetsObtained;
            
			if (framesRemaining < framesToCopy)
				framesToCopy = framesRemaining;
			
			bytesToCopy = framesToCopy * sizeof(Float32);
			// we're in a mono only case, so only copy the first buffer
			memcpy(tBufferList->mBuffers->mData, bufferInfo->mBuffer->GetDataPtr() + bufferInfo->mOffset, bytesToCopy);
			bufferInfo->mOffset += bytesToCopy;
			packetsObtained += framesToCopy;
						
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			// this block of code is the same as that found in the fill proc - 
			// it is for determining what to do when a buffer runs out of data

			if (bufferInfo->mOffset == bufferInfo->mBuffer->GetDataSize())
			{
				mCurrentBufferIndex++;
				// see if there is a next buffer or if the queue is looping and should return to the start
				BufferInfo	*nextBufferInfo = NextPlayableBufferInActiveQ();	// this method updates mCurrentBufferIndex as well
				if ((nextBufferInfo != NULL) || (mLooping == true))
				{
					if (mLooping == true)
						LoopToBeginning();
				}
				else
				{
					// looping is false and there are no more buffers so we are really out of data
					// return what we have and no error
					// if there are no messages on the Q by now, then the source will be disconnected and reset in the PostRender Proc
					mQueueIsProcessed = true;		// we are done now, the Q is dry
                    
                    // swap the list pointers now
                    BufferQueue*        tQ  = mBufferQueueActive;
                    mBufferQueueActive = mBufferQueueInactive;
                    mBufferQueueInactive = tQ;
				}
				// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			}
		}
	}

Finished:

	// if there wasn't enough data left, be sure to silence the end of the buffers
	if (packetsObtained < packetsRequestedFromRenderProc)
	{
		UInt32	byteCount = packetsObtained * sizeof(Float32);
		tBufferList->mBuffers[0].mDataByteSize = dataByteSize - byteCount;
		tBufferList->mBuffers[0].mData = (Byte *) ioData->mBuffers[0].mData + byteCount;
		memset(tBufferList->mBuffers[0].mData, 0, tBufferList->mBuffers[0].mDataByteSize);
		if (tBufferList->mNumberBuffers > 1)
		{
			tBufferList->mBuffers[1].mDataByteSize = tBufferList->mBuffers[0].mDataByteSize;
			tBufferList->mBuffers[1].mData = (Byte *) ioData->mBuffers[1].mData + byteCount;
			memset(tBufferList->mBuffers[1].mData, 0, tBufferList->mBuffers[1].mDataByteSize);
		}
	}

	// ramp the buffer up or down to avoid any clicking
	if (mRampState == kRampDown)
	{
		// ramp down these samples to avoid any clicking - this is the last buffer before disconnecting in Post Render
		RampDown(ioData);
		mRampState = kRampingComplete;
	}
	else if (mRampState == kRampUp)
	{
		// this is the first buffer since resuming, so ramp these samples up
		RampUp(ioData);
		mRampState = kRampingComplete;
	}
			
	return (noErr);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus OALSource::DoPostRender ()
{
	bool				renderProcsRemoved = false;
	PlaybackMessage*	lastMessage;

	OALRenderLocker::RenderTryer tried(mRenderLocker);

	if (!tried.Acquired())
		return noErr;

	try {
		// all messages must be executed after the last buffer has been ramped down
		if (mRampState == kRampingComplete)
		{
#if	LOG_MESSAGE_QUEUE					
			DebugMessageN1("FLUSHING MESSAGE QUEUE START - OALSource = %ld", (long int) mSelfToken);
#endif
			PlaybackMessage	*messages = mMessageQueue.pop_all_reversed();
			while (messages != NULL)
			{
				switch (messages->mMessageID)
				{
					case kMQ_Stop:
#if	LOG_MESSAGE_QUEUE					
						DebugMessage("     MQ:kMQ_Stop");
#endif
						if (mState != AL_STOPPED)
						{
							DisconnectFromBus();
							mState = AL_STOPPED;
							ClearActiveQueue();
							mQueueIsProcessed = false;
						}
						break;
						
					case kMQ_Rewind:
#if	LOG_MESSAGE_QUEUE					
						DebugMessage("     MQ:kMQ_Rewind");
#endif
						LoopToBeginning();
						if (renderProcsRemoved)
						{
							AddNotifyAndRenderProcs();
							renderProcsRemoved = false;
						}
						mState = AL_PLAYING; // in case it was also paused while processing these Q commands
						break;
						
					case kMQ_ClearBuffersFromQueue:
#if	LOG_MESSAGE_QUEUE					
						DebugMessage("     MQ:kMQ_ClearBuffersFromQueue");
#endif
						// when unqueue buffers is called while a source is in transition, the action must be deferred so the audio data can finish up
						PostRenderRemoveBuffersFromQueue(messages->mBuffersToUnqueueInPostRender);
						break;
						
					case kMQ_SetBuffer:
#if	LOG_MESSAGE_QUEUE					
						DebugMessageN1("     MQ:kMQ_SetBuffer: mAppendBuffer->GetToken() = %ld", (long int) messages->mAppendBuffer->GetToken());
#endif
						PostRenderSetBuffer(messages->mAppendBuffer->GetToken(), messages->mAppendBuffer);
						mState = AL_STOPPED;
						break;
						
					case kMQ_Play:
#if	LOG_MESSAGE_QUEUE					
						DebugMessage("     MQ:kMQ_Play");
#endif
						Play();
						break;
						
					case kMQ_Pause:
#if	LOG_MESSAGE_QUEUE					
						DebugMessage("     MQ:kMQ_Pause");
#endif
						mState = AL_PAUSED;
						ReleaseNotifyAndRenderProcs();
						renderProcsRemoved = true;
						break;
						
					case kMQ_SetFramePosition:
#if	LOG_MESSAGE_QUEUE					
						DebugMessage("     MQ:kMQ_SetFramePosition");
#endif
						// Rewind the Buffer Q
						LoopToBeginning();
						AdvanceQueueToFrameIndex(mPlaybackHeadPosition);
						mPlaybackHeadPosition = 0;
						break;
						
					case kMQ_DeconstructionStop:
					{
#if	LOG_MESSAGE_QUEUE					
						DebugMessage("     MQ:kMQ_DeconstructionStop");
#endif
						DisconnectFromBus();
						mState = AL_STOPPED;
						FlushBufferQueue();				// release attachement to any buffers
						mSafeForDeletion = true;		// now the CleanUp Sources method of the context can safely delete this object
						
						// before returning, delete all remaining messages on the queue so they do not get leaked when the object is deconstructed
						ClearMessageQueue();
						
						goto Finished;					// skip any remaining MQ messages
					}
				}
				
				lastMessage = messages;
				messages = messages->get_next();
				delete (lastMessage); // made it, so now get rid of it
			}
			mRampState = kNoRamping;
#if	LOG_MESSAGE_QUEUE					
			DebugMessageN1("FLUSHING MESSAGE QUEUE END - OALSource = %ld", (long int) mSelfToken);
#endif
		}

Finished:
		
		if (mQueueIsProcessed)
		{
			// this means that the data ran out on it's own and we are not stopped as a result of a queued message
			DisconnectFromBus();
			mState = AL_STOPPED;
			ClearActiveQueue();
			mQueueIsProcessed = false;
		}
	}
	catch(...){
		DebugMessageN1("OALSource::DoPostRender:ERROR - OALSource = %ld", (long int)  mSelfToken);
	}
	
	return noErr;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void OALSource::RampDown (AudioBufferList 			*ioData)
{
	UInt32		sampleCount = (ioData->mBuffers[0].mDataByteSize / sizeof (Float32));
			
	Float32		slope = 1.0/sampleCount;
	Float32		scalar = 1.0;
	for (UInt32	i = 0; i < ioData->mNumberBuffers; i++)
	{
		Float32		*sample = (Float32*) ioData->mBuffers[i].mData;
		for (UInt32	count = sampleCount; count > 0 ; count--)
		{
			*sample *= scalar;
			scalar -= slope;
			sample++;
		}
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void OALSource::RampUp (AudioBufferList 			*ioData)
{
	UInt32		sampleCount = (ioData->mBuffers[0].mDataByteSize / sizeof (Float32));
			
	Float32		slope = 1.0/sampleCount;
	Float32		scalar = 0.0;
	for (UInt32	i = 0; i < ioData->mNumberBuffers; i++)
	{
		Float32		*sample = (Float32*) ioData->mBuffers[i].mData;
		for (UInt32	count = sampleCount; count > 0 ; count--)
		{
			*sample *= scalar;
			scalar += slope;
			sample++;
		}
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Support for Pre 2.0 3DMixer
//
// Pull the audio data by using DoRender(), and then Sample Rate Convert it to the mixer's 
// output sample rate so the 1.3 mixer doesn't have to do any SRC.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

OSStatus	OALSource::DoSRCRender(	AudioBufferList 			*ioData )
{   
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	BufferInfo	*bufferInfo = NULL;

	OALRenderLocker::RenderTryer tried(mRenderLocker);

	if (!tried.Acquired()) 
	{
		// source is being edited. can't render, so the ioData buffers must be cleared out 
		for (UInt32	i = 0; i < ioData->mNumberBuffers; i++)
			memset(ioData->mBuffers[i].mData, 0, ioData->mBuffers[i].mDataByteSize);

		return noErr;
	}	

	// 1st move past any AL_NONE Buffers	
	bufferInfo = NextPlayableBufferInActiveQ();
	if (bufferInfo == NULL)
	{
		// if there are no messages on the Q by now, then the source will be disconnected and reset in the PostRender Proc
		mQueueIsProcessed = true;
		return -1;	// there are no more buffers
	}
	
	// update the Q lists before returning any data
	UpdateQueue();	
	
    // if there are no more buffers in the active Q, go back to the beginning if in Loop mode, else pad zeroes and clean up in the PostRender proc
	bufferInfo = mBufferQueueActive->Get(mCurrentBufferIndex);
	if ((bufferInfo == NULL) && (mLooping == true))
	{
		// swap the list pointers now
		BufferQueue*        tQ  = mBufferQueueActive;
		mBufferQueueActive  = mBufferQueueInactive;
		mBufferQueueInactive = tQ;
	}
	else if (bufferInfo == NULL)
    {
        // if there are no messages on the Q by now, then the source will be disconnected and reset in the PostRender Proc
        mQueueIsProcessed = true;
        // stop rendering, there is no more data
        return -1;	// there are no buffers
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	ChangeChannelSettings();
        			
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		
	double srcSampleRate; srcSampleRate = bufferInfo->mBuffer->GetSampleRate();
	double dstSampleRate; dstSampleRate = mOwningContext->GetMixerRate();
	double ratio; ratio = (srcSampleRate / dstSampleRate) * mPitch * mDopplerScaler;
	
	int nchannels; nchannels = ioData->mNumberBuffers;

	if (ratio == 1.0)
	{
		// no SRC necessary so just call the normal render proc and let it fill out the buffers
        return (DoRender(ioData));
 	}

	// otherwise continue on to do dirty linear interpolation
	UInt32      inFramesToProcess; inFramesToProcess = ioData->mBuffers[0].mDataByteSize / sizeof(Float32);
	float readIndex; readIndex = mReadIndex;

	int readUntilIndex; readUntilIndex = (int) (2.0 + readIndex + inFramesToProcess * ratio );
	int framesToPull; framesToPull = readUntilIndex - 2;
	
	if (framesToPull == 0) 
        return -1;
	
    // set the buffer size so DoRender will get the correct amount of frames
    mTempSourceStorage->mBuffers[0].mDataByteSize = framesToPull * sizeof(UInt32);
    mTempSourceStorage->mBuffers[1].mDataByteSize = framesToPull * sizeof(UInt32);
    
    // if the size of the buffers are too small, reallocate them now
    if (mTempSourceStorageBufferSize < (framesToPull * sizeof(UInt32)))
    {
        if (mTempSourceStorage->mBuffers[0].mData != NULL)
            free(mTempSourceStorage->mBuffers[0].mData);

        if (mTempSourceStorage->mBuffers[1].mData != NULL)
            free(mTempSourceStorage->mBuffers[1].mData);
            
        mTempSourceStorageBufferSize = (framesToPull * sizeof(UInt32));
        mTempSourceStorage->mBuffers[0].mData = malloc(mTempSourceStorageBufferSize);
        mTempSourceStorage->mBuffers[1].mData = malloc(mTempSourceStorageBufferSize);
    }
       
	// get input source audio
    mTempSourceStorage->mNumberBuffers = ioData->mNumberBuffers;
    for (UInt32 i = 0; i < mTempSourceStorage->mNumberBuffers; i++)
    {
        mTempSourceStorage->mBuffers[i].mDataByteSize = framesToPull * sizeof(UInt32);
    }
    
    OSStatus result; result = DoRender(mTempSourceStorage);
	if (result != noErr ) 
        return result;		// !!@ something bad happened (could return error code)

	float *pullL; pullL = (float *) mTempSourceStorage->mBuffers[0].mData;
	float *pullR; pullR = nchannels > 1 ? (float *) mTempSourceStorage->mBuffers[1].mData: NULL;

	// setup a small array of the previous two cached values, plus the first new input frame
	float tempL[4];
	float tempR[4];
	tempL[0] = mCachedInputL1;
	tempL[1] = mCachedInputL2;
	tempL[2] = pullL[0];

	if (pullR)
	{
		tempR[0] = mCachedInputR1;
		tempR[1] = mCachedInputR2;
		tempR[2] = pullR[0];
	}

	// in first loop start out getting source from this small array, then change sourceL/sourceR to point
	// to the buffers containing the new pulled input for the main loop
	float *sourceL; sourceL = tempL;
	float *sourceR; sourceR = tempR;
	if(!pullR) 
        sourceR = NULL;

	// keep around for next time
	mCachedInputL1 = pullL[framesToPull - 2];
	mCachedInputL2 = pullL[framesToPull - 1];
	
	if(pullR)
	{
		mCachedInputR1 = pullR[framesToPull - 2];
		mCachedInputR2 = pullR[framesToPull - 1];
	}

	// quick-and-dirty linear interpolation
	int n; n = inFramesToProcess;
	
	float *destL; destL = (float *) ioData->mBuffers[0].mData;
	float *destR; destR = (float *) ioData->mBuffers[1].mData;
	
	if (!sourceR)
	{
		// mono input
		
		// generate output based on previous cached values
		while (readIndex < 2.0 &&  n > 0)
		{
			int iReadIndex = (int)readIndex;
			int iReadIndex2 = iReadIndex + 1;
			
			float frac = readIndex - float(iReadIndex);

			float s1 = sourceL[iReadIndex];
			float s2 = sourceL[iReadIndex2];
			float left  = s1 + frac * (s2-s1);
			
			*destL++ = left;
			
			readIndex += ratio;
			
			n--;
		}

		// generate output based on new pulled input

		readIndex -= 2.0;

		sourceL = pullL;

		while (n--)
		{
			int iReadIndex = (int)readIndex;
			int iReadIndex2 = iReadIndex + 1;
			
			float frac = readIndex - float(iReadIndex);

			float s1 = sourceL[iReadIndex];
			float s2 = sourceL[iReadIndex2];
			float left  = s1 + frac * (s2-s1);
			
			*destL++ = left;
			
			readIndex += ratio;
		}

		readIndex += 2.0;
	}
	else
	{
		// stereo input
		// generate output based on previous cached values
		while(readIndex < 2.0 &&  n > 0)
		{
			int iReadIndex = (int)readIndex;
			int iReadIndex2 = iReadIndex + 1;
			
			float frac = readIndex - float(iReadIndex);
			
			float s1 = sourceL[iReadIndex];
			float s2 = sourceL[iReadIndex2];
			float left  = s1 + frac * (s2-s1);
			
			float s3 = sourceR[iReadIndex];
			float s4 = sourceR[iReadIndex2];
			float right  = s3 + frac * (s4-s3);
			
			*destL++ = left;
			*destR++ = right;

			readIndex += ratio;
			
			n--;
		}

		// generate output based on new pulled input

		readIndex -= 2.0;

		sourceL = pullL;
		sourceR = pullR;

		while (n--)
		{
			int iReadIndex = (int)readIndex;
			int iReadIndex2 = iReadIndex + 1;
			
			float frac = readIndex - float(iReadIndex);
			
			float s1 = sourceL[iReadIndex];
			float s2 = sourceL[iReadIndex2];
			float left  = s1 + frac * (s2-s1);
			
			float s3 = sourceR[iReadIndex];
			float s4 = sourceR[iReadIndex2];
			float right  = s3 + frac * (s4-s3);
			
			*destL++ = left;
			*destR++ = right;

			readIndex += ratio;
		}

		readIndex += 2.0;

	}
	
	// normalize read index back to start of buffer for next time around...
	
	readIndex -= float(framesToPull);
    
	mReadIndex = readIndex;

	return noErr;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CalculateDistanceAndAzimuth() support
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Float32 MAG(ALfloat *inVector)
{
	return sqrt(inVector[0] * inVector[0] + inVector[1] * inVector[1] + inVector[2] * inVector[2]);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void aluCrossproduct(ALfloat *inVector1,ALfloat *inVector2,ALfloat *outVector)
{
	outVector[0]=(inVector1[1]*inVector2[2]-inVector1[2]*inVector2[1]);
	outVector[1]=(inVector1[2]*inVector2[0]-inVector1[0]*inVector2[2]);
	outVector[2]=(inVector1[0]*inVector2[1]-inVector1[1]*inVector2[0]);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Float32 aluDotproduct(ALfloat *inVector1,ALfloat *inVector2)
{
	return (inVector1[0]*inVector2[0]+inVector1[1]*inVector2[1]+inVector1[2]*inVector2[2]);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void aluNormalize(ALfloat *inVector)
{
	ALfloat length,inverse_length;

	length=(ALfloat)sqrt(aluDotproduct(inVector,inVector));
	if (length != 0)
	{
		inverse_length=(1.0f/length);
		inVector[0]*=inverse_length;
		inVector[1]*=inverse_length;
		inVector[2]*=inverse_length;
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void aluMatrixVector(ALfloat *vector,ALfloat matrix[3][3])
{
	ALfloat result[3];

	result[0]=vector[0]*matrix[0][0]+vector[1]*matrix[1][0]+vector[2]*matrix[2][0];
	result[1]=vector[0]*matrix[0][1]+vector[1]*matrix[1][1]+vector[2]*matrix[2][1];
	result[2]=vector[0]*matrix[0][2]+vector[1]*matrix[1][2]+vector[2]*matrix[2][2];
	memcpy(vector,result,sizeof(result));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
inline bool	IsZeroVector(Float32* inVector)
{
	if ((inVector[0] == 0.0) && (inVector[1] == 0.0) && (inVector[2] == 0.0))
		return true;
	else
		return false;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define	LOG_SOURCE_CONES	0
bool OALSource::ConeAttenuation()
{	
	// determine if attenuation  is needed at all
	if (	IsZeroVector(mConeDirection)		||
			((mConeInnerAngle == 360.0) && (mConeOuterAngle == 360.0))	)
	{
		// Not Needed if: AL_Direction is 0,0,0, OR if (Inner and Outer Angle are both 360.0)
		if (mConeGainScaler != 1.0)
		{
			// make sure to reset the bus gain if the current cone scaler is not 1.0 and source cone scaling is no longer required
			mConeGainScaler = 1.0;
			return true;	// let the caller know the bus gain needs resetting
		}
		return false;	// no change has occurred to require a bus gain reset
	}
	
	// Calculate the Source Cone Attenuation scaler
	
	Float32		vsl[3];		// source to listener vector
	Float32		coneDirection[3];
	Float32		angle;
	
	mOwningContext->GetListenerPosition(&vsl[0], &vsl[1], &vsl[2]);

	// calculate the source to listener vector
	vsl[0] -= mPosition[0];
	vsl[1] -= mPosition[1];
	vsl[2] -= mPosition[2];
	aluNormalize(vsl);				// Normalized source to listener vector

    coneDirection[0] = mConeDirection[0];
	coneDirection[1] = mConeDirection[1];
	coneDirection[2] = mConeDirection[2];
	aluNormalize(coneDirection);	// Normalized cone direction vector
	
	// calculate the angle between the cone direction vector and the source to listener vector
	angle = 180.0 * acos (aluDotproduct(vsl, coneDirection))/M_PI; // convert from radians to degrees
	
	Float32		absAngle = fabs(angle);
	Float32		absInnerAngle = fabs(mConeInnerAngle)/2.0;	// app provides the size of the entire inner angle
	Float32		absOuterAngle = fabs(mConeOuterAngle)/2.0;	// app provides the size of the entire outer angle
	Float32		newScaler;
	
	if (absAngle <= absInnerAngle)
	{
		 // listener is within the inner cone angle, no attenuation required
		 newScaler = 1.0;
#if LOG_SOURCE_CONES
		DebugMessage("ConeAttenuation - Listener is within the inner angle, no Attenuation required");
#endif
	}
	else if (absAngle >= absOuterAngle)
	{
		 // listener is outside the outer cone angle, sett attenuation to outer cone gain
#if LOG_SOURCE_CONES
		DebugMessageN1("ConeAttenuation - Listener is outside the outer angle, scaler equals the Outer Cone Gain = %f2", mConeOuterGain);
#endif
		newScaler = mConeOuterGain;
	}
	else
	{
		// this source to listener vector is between the inner and outer cone angles so apply some gain scaling
		// db or linear?
	
		// as you move from inner to outer, x goes from 0->1
		Float32 x =  (absAngle - absInnerAngle ) / (absOuterAngle - absInnerAngle );
		
		newScaler = 1.0/* cone inner gain */ * (1.0 - x)   +    mConeOuterGain * x;
#if LOG_SOURCE_CONES
		DebugMessageN1("ConeAttenuation - Listener is between inner and outer angles, scaler equals = %f2", newScaler);
#endif
	}
	
	// there is no need to reset the bus gain if the scaler has not changed (a common scenario)
	// change is only necessaery when moving around within the transition zone or crossing between inner, transition and outer zones
	if (newScaler != mConeGainScaler)
	{
		mConeGainScaler = newScaler;
		return true;	// let the caller know the bus gain needs resetting
	}
	
	return false;	// no change has occurred to require a bus gain reset
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void OALSource::CalculateDistanceAndAzimuth(Float32 *outDistance, Float32 *outAzimuth, Float32 *outElevation, Float32	*outDopplerShift)
{

    Float32 	ListenerOrientation[6],
                ListenerPosition[3],
                ListenerVelocity[3],
                Angle = 0.0,
                Distance = 2.0,
				Elevation = 0.0,
                Distance_squared = 4.0,
                front_back,
                SourceToListener[3],
				ProjectedSource[3],
				UpProjection,
                Look_Norm[3],
                RightEarVector[3],  // previously named U
				Up[3],
                tPosition[3],
                dopplerShift = 1.0;     // default at No shift
                
    *outDopplerShift = dopplerShift;    // initialize

	SourceToListener[0]=0;		// initialize
	SourceToListener[1]=0;		// initialize
	SourceToListener[2]=0;		// initialize
	Up[0]=0;					// initialize
	Up[1]=0;					// initialize
	Up[2]=0;					// initialize
        
    tPosition[0] = mPosition[0];
	tPosition[1] = mPosition[1];
	tPosition[2] = mPosition[2];
	
#if LOG_VERBOSE
        DebugMessageN3("CalculateDistanceAndAzimuth source position = %f2/%f2/%f2\n", mPosition[0], mPosition[1], mPosition[2]);
#endif
			
	//Get listener properties
	mOwningContext->GetListenerPosition(&ListenerPosition[0], &ListenerPosition[1], &ListenerPosition[2]);
	mOwningContext->GetListenerVelocity(&ListenerVelocity[0], &ListenerVelocity[1], &ListenerVelocity[2]);
	mOwningContext->GetListenerOrientation(&ListenerOrientation[0], &ListenerOrientation[1], &ListenerOrientation[2],
											&ListenerOrientation[3], &ListenerOrientation[4], &ListenerOrientation[5]);

#if LOG_VERBOSE
    DebugMessageN3("CalculateDistanceAndAzimuth listener position = %f2/%f2/%f2\n", ListenerPosition[0], ListenerPosition[1], ListenerPosition[2]);
#endif

	// Get buffer properties
	BufferInfo	*bufferInfo = mBufferQueueActive->Get(mCurrentBufferIndex);
	if (bufferInfo == NULL)
	{
        // Not sure if this should be the error case
        *outDistance = 0.0;
        *outAzimuth = 0.0;
        *outElevation = 0.0;
        return;	// there are no buffers
	}
    
	// Only apply 3D calculations for mono buffers
	if (bufferInfo->mBuffer->GetNumberChannels() == 1)
	{
		//1. Translate Listener to origin (convert to head relative)
		if (mSourceRelative == AL_FALSE)
		{
			tPosition[0] -= ListenerPosition[0];
			tPosition[1] -= ListenerPosition[1];
			tPosition[2] -= ListenerPosition[2];
		}
        //2. Align coordinate system axes
        aluCrossproduct(&ListenerOrientation[0],&ListenerOrientation[3],RightEarVector); // Right-ear-vector
        aluNormalize(RightEarVector); // Normalized Right-ear-vector
        Look_Norm[0] = ListenerOrientation[0];
        Look_Norm[1] = ListenerOrientation[1];
        Look_Norm[2] = ListenerOrientation[2];
        aluNormalize(Look_Norm);
                
       //3. Calculate distance attenuation
        Distance_squared = aluDotproduct(tPosition,tPosition);
		Distance = sqrt(Distance_squared);
                                                                                                       
        Angle = 0.0f;

	  //4. Determine Angle of source relative to listener
	  if(Distance>0.0f){
		SourceToListener[0]=tPosition[0];    
		SourceToListener[1]=tPosition[1];
		SourceToListener[2]=tPosition[2];
		// Note: SourceToListener doesn't need to be normalized here.
		// Probably better to move this next line into the Doppler
		// calculation code so that it can be optimized away if
		// DopplerFactor is 0.
		aluNormalize(SourceToListener);

		aluCrossproduct(RightEarVector, Look_Norm, Up);
		UpProjection = aluDotproduct(SourceToListener,Up);
		ProjectedSource[0] = SourceToListener[0] - UpProjection*Up[0];
		ProjectedSource[1] = SourceToListener[1] - UpProjection*Up[1];
		ProjectedSource[2] = SourceToListener[2] - UpProjection*Up[2];
		aluNormalize(ProjectedSource);

		Angle = 180.0 * acos (aluDotproduct(ProjectedSource, RightEarVector))/M_PI;
		zapBadness(Angle); // remove potential NANs

		//is the source infront of the listener or behind?
		front_back = aluDotproduct(ProjectedSource,Look_Norm);
		if(front_back<0.0f)
		  Angle = 360.0f - Angle;

		//translate from cartesian angle to 3d mixer angle
		if((Angle>=0.0f)&&(Angle<=270.0f)) 
			Angle = 90.0f - Angle;
		else 
			Angle = 450.0f - Angle;
	  }
		
        //5. Calculate elevation
		Elevation = 90.0 - 180.0 * acos(    aluDotproduct(SourceToListener, Up)   )/ 3.141592654f;
		zapBadness(Elevation); // remove potential NANs

		if(SourceToListener[0]==0.0 && SourceToListener[1]==0.0 && SourceToListener[2]==0.0 )
		   Elevation = 0.0;

		if (Elevation > 90.0) 
			Elevation = 180.0 - Elevation;
		if (Elevation < -90.0) 
			Elevation = -180.0 - Elevation;
			
		//6. Calculate doppler
		Float32		dopplerFactor = mOwningContext->GetDopplerFactor();
        if (dopplerFactor > 0.0)
        {
			Float32		speedOfSound = mOwningContext->GetSpeedOfSound();
            
			Float32     SourceVelocity[3];
            GetVelocity (SourceVelocity[0], SourceVelocity[1], SourceVelocity[2]);
			
			// don't do all these calculations if the sourec and listener have zero velocity
			bool	SourceHasVelocity = !IsZeroVector(SourceVelocity);
			bool	ListenerHasVelocity = !IsZeroVector(ListenerVelocity);
			if (SourceHasVelocity || ListenerHasVelocity)
			{
				Float32	NUvls = (aluDotproduct(SourceToListener, ListenerVelocity))/MAG(SourceToListener);
				Float32	NUvss = (aluDotproduct(SourceToListener, SourceVelocity))/MAG(SourceToListener);

				NUvls = -NUvls; // COMMENT HERE PLEASE
				NUvss = -NUvss; // COMMENT HERE PLEASE
				
				NUvls = fmin(NUvls, speedOfSound/dopplerFactor);
				NUvss = fmin(NUvss, speedOfSound/dopplerFactor);
								
				dopplerShift = ( (speedOfSound - dopplerFactor * NUvls) / (speedOfSound - dopplerFactor * NUvss)   );
				zapBadnessForDopplerShift(dopplerShift); // remove potential NANs
																												
				// limit the pitch shifting to 4 octaves up and 3 octaves down
				if (dopplerShift > 16.0)
					dopplerShift = 16.0;
				else if(dopplerShift < 0.125)
					dopplerShift = 0.125;   

				#if LOG_DOPPLER
					DebugMessageN1("CalculateDistanceAndAzimuth: dopplerShift after scaling =  %f2\n", dopplerShift);
				#endif
				
				*outDopplerShift = dopplerShift;
			}
        }
    }
    else
    {
        Angle=0.0;
        Distance=0.0;
    }
       
	if ((Get3DMixerVersion() < k3DMixerVersion_2_0) && (mReferenceDistance > 1.0))
	{
        // the pre 2.0 mixer does not have the DistanceParam property so to compensate,
        // set the DistanceAtten property correctly for refDist, maxDist, and rolloff,
        // and then scale our calculated distance to a reference distance of 1.0 before passing to the mixer
        Distance = Distance/mReferenceDistance;
        if (Distance > mMaxDistance/mReferenceDistance) 
            Distance = mMaxDistance/mReferenceDistance; // clamp the distance to the max distance
	}

    *outDistance = Distance;
    *outAzimuth = Angle;
    *outElevation = Elevation;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Apple Environmental Audio (ASA) Extension
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ***** ASA Extension Methods *****
void OALSource::SetReverbSendLevel(Float32 inReverbLevel)
{
#if LOG_GRAPH_AND_MIXER_CHANGES
    DebugMessageN2("OALSource::SetReverbSendLevel:  OALSource: = %ld : inReverbLevel %f2\n", mSelfToken, inReverbLevel );
#endif            

	if (inReverbLevel < 0.0f || inReverbLevel > 1.0f)
		throw AL_INVALID_VALUE; // must be within 0.0-1.0 range

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

    if (inReverbLevel == mASAReverbSendLevel)
		return;			// nothing to do

		
	mASAReverbSendLevel = inReverbLevel;
	UpdateBusReverb();
}
	
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetOcclusion(Float32 inOcclusion)
{
#if LOG_GRAPH_AND_MIXER_CHANGES
    DebugMessageN2("OALSource::SetOcclusion:  OALSource: = %ld : inOcclusion %f2\n", mSelfToken, inOcclusion );
#endif            
	if (inOcclusion < -100.0f || inOcclusion > 0.0f)
		throw AL_INVALID_VALUE; // must be within -100.0 - 0.0 range

    if (inOcclusion == mASAOcclusion)
		return;			// nothing to do	
		

	mASAOcclusion = inOcclusion;
			
	UpdateBusOcclusion();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetObstruction(Float32 inObstruction)
{
#if LOG_GRAPH_AND_MIXER_CHANGES
    DebugMessageN2("OALSource::SetObstruction:  OALSource: = %ld : inObstruction %f2\n", mSelfToken, inObstruction );
#endif            
	if (inObstruction < -100.0f || inObstruction > 0.0f)
		throw AL_INVALID_VALUE; // must be within -100.0 - 0.0 range

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

    if (inObstruction == mASAObstruction)
		return;			// nothing to do	
		
	mASAObstruction = inObstruction;
			
	UpdateBusObstruction();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetRogerBeepEnable(Boolean inEnable)
{
#if LOG_GRAPH_AND_MIXER_CHANGES
    DebugMessageN2("OALSource::SetRogerBeepEnable:  OALSource: = %ld : inEnable %d\n", (long int)mSelfToken, inEnable );
#endif            

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

	// Enable cannot be set during playback 
	if ((mState == AL_PLAYING) || (mState == AL_PAUSED))
		throw (OSStatus) AL_INVALID_OPERATION;
		
    if (inEnable == mASARogerBeepEnable)
		return;			// nothing to do	
		
	mASARogerBeepEnable = inEnable;
	
	if (mASARogerBeepEnable)
	{
		SetupRogerBeepAU();
		
		// first get the initial values
		UInt32 propSize = sizeof(UInt32);
		UInt32 bypassValue;
		if(AudioUnitGetProperty(mRogerBeepAU, kAudioUnitProperty_BypassEffect, kAudioUnitScope_Global, 0, &bypassValue, &propSize) == noErr)
			mASARogerBeepOn = bypassValue ? 0 : 1;
		
		Float32 paramValue;
		if(AudioUnitGetParameter(mRogerBeepAU, 6/*kRogerBeepParam_RogerGain*/, kAudioUnitScope_Global, 0, &paramValue) == noErr)
			mASARogerBeepGain = paramValue;
		
		if(AudioUnitGetParameter(mRogerBeepAU, 4/*kRogerBeepParam_Sensitivity*/, kAudioUnitScope_Global, 0, &paramValue) == noErr)
			mASARogerBeepSensitivity = (UInt32)paramValue;
		
		if(AudioUnitGetParameter(mRogerBeepAU, 5 /*kRogerBeepParam_RogerType*/, kAudioUnitScope_Global, 0, &paramValue) == noErr)
			mASARogerBeepType = (UInt32)paramValue;
			
		//now default the unit off
		SetRogerBeepOn(false);
	}
	
	else
	{
		if(mRogerBeepNode)
			AUGraphRemoveNode(mOwningContext->GetGraph(), mRogerBeepNode);
		mRogerBeepNode = 0;
		mRogerBeepAU = 0;
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetRogerBeepOn(Boolean inOn)
{
#if LOG_GRAPH_AND_MIXER_CHANGES
    DebugMessageN2("OALSource::SetRogerBeepOn:  OALSource: = %ld : inOn %d\n", mSelfToken, inOn );
#endif
      
	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);
		      
    if (inOn == mASARogerBeepOn)
		return;			// nothing to do	
		
	mASARogerBeepOn = inOn;
	
	UInt32 bypassValue = mASARogerBeepOn ? 0 : 1;
	AudioUnitSetProperty(mRogerBeepAU, kAudioUnitProperty_BypassEffect, kAudioUnitScope_Global, 0, &bypassValue, sizeof(UInt32));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetRogerBeepGain(Float32 inGain)
{
#if LOG_GRAPH_AND_MIXER_CHANGES
    DebugMessageN2("OALSource::SetGain:  OALSource: = %ld : inGain %f2\n", mSelfToken, inGain );
#endif

	if (inGain < -100.0f || inGain > 20.0f)
		throw AL_INVALID_VALUE; // must be within -100.0 - 20.0 range      

	if(!mASARogerBeepEnable)
		throw (OSStatus) AL_INVALID_OPERATION;
		
    if (inGain == mASARogerBeepGain)
		return;			// nothing to do	
		
	mASARogerBeepGain = inGain;
			
	AudioUnitSetParameter(mRogerBeepAU, 6/*kRogerBeepParam_RogerGain*/, kAudioUnitScope_Global, 0, inGain, 0 );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetRogerBeepSensitivity(UInt32 inSensitivity)
{
#if LOG_GRAPH_AND_MIXER_CHANGES
    DebugMessageN2("OALSource::SetRogerBeepSensitivity:  OALSource: = %ld : inSensitivity %d\n", mSelfToken, inSensitivity );
#endif        
	if (inSensitivity < 0 || inSensitivity > 2)
		throw AL_INVALID_VALUE; // must be within 0 - 2 range

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

	if(!mASARogerBeepEnable)
		throw AL_INVALID_OPERATION;
		    
    if (inSensitivity == mASARogerBeepSensitivity)
		return;			// nothing to do	
		
	mASARogerBeepSensitivity = inSensitivity;
			
	AudioUnitSetParameter(mRogerBeepAU, 4/*kRogerBeepParam_Sensitivity*/, kAudioUnitScope_Global, 0, inSensitivity, 0 );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetRogerBeepType(UInt32 inType)
{
#if LOG_GRAPH_AND_MIXER_CHANGES
    DebugMessageN2("OALSource::SetRogerBeepType:  OALSource: = %ld : inType %d\n", mSelfToken, inType );
#endif

	if (inType < 0 || inType > 3)
		throw AL_INVALID_VALUE; // must be within 0 - 3 range

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

	if(!mASARogerBeepEnable)
		throw AL_INVALID_OPERATION;
		 
    if (inType == mASARogerBeepType)
		return;			// nothing to do	
		
	mASARogerBeepType = inType;
			
	AudioUnitSetParameter(mRogerBeepAU, 5 /*kRogerBeepParam_RogerType*/, kAudioUnitScope_Global, 0, inType, 0 );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetRogerBeepPreset(FSRef* inRef)
{
	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

	try {
			Boolean				status; 
			SInt32				result = 0;
			CFURLRef			fileURL = CFURLCreateFromFSRef (kCFAllocatorDefault, inRef);
			if (fileURL) 
			{
				// Read the XML file.
				CFDataRef		resourceData = NULL;
				
				status = CFURLCreateDataAndPropertiesFromResource (kCFAllocatorDefault, fileURL, &resourceData,	NULL, NULL, &result);
				CFRelease (fileURL);	// no longer needed
				
				if (status == false || result)				
					throw (OSStatus) -1;			
				else
				{
					CFStringRef			errString = NULL;
					CFPropertyListRef   theData = NULL;
					theData = CFPropertyListCreateFromXMLData (kCFAllocatorDefault, resourceData, kCFPropertyListImmutable, &errString);
					CFRelease (resourceData);
					if (errString)
						CFRelease (errString);
					
					if (theData == NULL || errString) 
					{
						if (theData)
							CFRelease (theData);
						throw (OSStatus) -1;			
					}
					else
					{
						result = AudioUnitSetProperty(mRogerBeepAU, kAudioUnitProperty_ClassInfo, kAudioUnitScope_Global, 0, &theData, sizeof(theData) );
						CFRelease (theData);
						THROW_RESULT
					}				
				}
			}
			else
				throw (OSStatus) -1;			
	}
	catch (OSStatus result) {
		throw result;
	}
	catch (...) {
		throw (OSStatus) -1;			
	}
	
	return;

}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetDistortionEnable(Boolean inEnable)
{
#if LOG_GRAPH_AND_MIXER_CHANGES
    DebugMessageN2("OALSource::SetDistortionEnable:  OALSource: = %ld : inEnable %d\n", mSelfToken, inEnable );
#endif

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

    if (inEnable == mASADistortionEnable)
		return;			// nothing to do	
		
	mASADistortionEnable = inEnable;
	if (mASADistortionEnable)
	{
		SetupDistortionAU();
		// first get the default values
		UInt32 propSize = sizeof(UInt32);
		
		UInt32 bypassValue;
		if(AudioUnitGetProperty(mDistortionAU, kAudioUnitProperty_BypassEffect, kAudioUnitScope_Global, 0, &bypassValue, &propSize) == noErr) 
			mASADistortionOn = bypassValue ? 0 : 1;
		
		Float32 paramValue;
		if(AudioUnitGetParameter(mDistortionAU, 15/*kDistortionParam_FinalMix*/, kAudioUnitScope_Global, 0, &paramValue) == noErr)
			mASADistortionMix = paramValue;
			
		AUPreset distortionType;
		propSize = sizeof(distortionType);
		if(AudioUnitGetProperty(mDistortionAU, kAudioUnitProperty_PresentPreset, kAudioUnitScope_Global, 0, &distortionType, &propSize) == noErr)
		{
			if(distortionType.presetName) CFRelease(distortionType.presetName);
			mASADistortionType = distortionType.presetNumber;
		}
		
		// now default the unit off
		SetDistortionOn(false);
	}
	
	else
	{
		if(mDistortionNode)
			AUGraphRemoveNode(mOwningContext->GetGraph(), mDistortionNode);
		mDistortionNode = 0;
		mDistortionAU = 0;
	}	
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetDistortionOn(Boolean inOn)
{
#if LOG_GRAPH_AND_MIXER_CHANGES
    DebugMessageN2("OALSource::SetDistortionOn:  OALSource: = %ld : inOn %d\n", mSelfToken, inOn );
#endif   

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock); 
		       
    if (inOn == mASADistortionOn)
		return;			// nothing to do	
		
	mASADistortionOn = inOn;
	
	UInt32 bypassValue = mASADistortionOn ? 0 : 1;
	
	AudioUnitSetProperty(mDistortionAU, kAudioUnitProperty_BypassEffect, kAudioUnitScope_Global, 0, &bypassValue, sizeof(UInt32));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetDistortionMix(Float32 inMix)
{
#if LOG_GRAPH_AND_MIXER_CHANGES
    DebugMessageN2("OALSource::SetDistortionMix:  OALSource: = %ld : inMix %f2\n", mSelfToken, inMix );
#endif

	if (inMix < 0.0f || inMix > 100.0f)
		throw AL_INVALID_VALUE; // must be within 0.0 - 100.0 range

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);
      
	if(!mASADistortionEnable)
		throw AL_INVALID_OPERATION;
		
    if (inMix == mASADistortionMix)
		return;			// nothing to do	

	mASADistortionMix = inMix;
	AudioUnitSetParameter(mDistortionAU, 15/*kDistortionParam_FinalMix*/, kAudioUnitScope_Global, 0, inMix, 0 );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetDistortionType(SInt32 inType)
{
#if LOG_GRAPH_AND_MIXER_CHANGES
    DebugMessageN2("OALSource::SetDistortionType:  OALSource: = %u : inType %d\n", mSelfToken, inType );
#endif     

	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);

	if(!mASADistortionEnable)
		throw AL_INVALID_OPERATION; 
		       
    if (inType == mASADistortionType)
		return;			// nothing to do	
		
	mASADistortionType = inType;
	AUPreset distortionType;
	distortionType.presetNumber = mASADistortionType;
	distortionType.presetName = NULL;
			
	AudioUnitSetProperty(mDistortionAU, kAudioUnitProperty_PresentPreset, kAudioUnitScope_Global, 0, &distortionType, sizeof(AUPreset));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetDistortionPreset(FSRef* inRef)
{   
	// don't allow synchronous source manipulation
	CAGuard::Locker sourceLock(mSourceLock);
		
	try {
			Boolean				status; 
			SInt32				result = 0;
			CFURLRef			fileURL = CFURLCreateFromFSRef (kCFAllocatorDefault, inRef);
			if (fileURL) 
			{
				// Read the XML file.
				CFDataRef		resourceData = NULL;
				
				status = CFURLCreateDataAndPropertiesFromResource (kCFAllocatorDefault, fileURL, &resourceData,	NULL, NULL, &result);
				CFRelease (fileURL);	// no longer needed
				
				if (status == false || result)				
					throw (OSStatus) -1;			
				else
				{
					CFStringRef			errString = NULL;
					CFPropertyListRef   theData = NULL;
					theData = CFPropertyListCreateFromXMLData (kCFAllocatorDefault, resourceData, kCFPropertyListImmutable, &errString);
					CFRelease (resourceData);
					if (errString)
						CFRelease (errString);
					
					if (theData == NULL || errString) 
					{
						if (theData)
							CFRelease (theData);
						throw (OSStatus) -1;			
					}
					else
					{
						result = AudioUnitSetProperty(mDistortionAU, kAudioUnitProperty_ClassInfo, kAudioUnitScope_Global, 0, &theData, sizeof(theData) );
						CFRelease (theData);
						THROW_RESULT
					}				
				}
			}
			else
				throw (OSStatus) -1;			
	}
	catch (OSStatus result) {
		throw result;
	}
	catch (...) {
		throw (OSStatus) -1;			
	}
	
	return;

}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark _____BufferQueue_____
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UInt32     BufferQueue::GetCurrentFrame(UInt32	inBufferIndex)  
{
	iterator	it = begin();
	std::advance(it, inBufferIndex);
	if (it != end())
		return (it->mOffset/it->mBuffer->GetBytesPerPacket());

	return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void 	BufferQueue::AppendBuffer(OALSource*	thisSource, ALuint	inBufferToken, OALBuffer	*inBuffer, ALuint	inACToken)
{
	BufferInfo	newBuffer;
			
	newBuffer.mBufferToken = inBufferToken;
	newBuffer.mBuffer = inBuffer;
	newBuffer.mOffset = 0;
	newBuffer.mProcessedState = kPendingProcessing;
	newBuffer.mACToken = inACToken;

	push_back(value_type (newBuffer));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALuint 	BufferQueue::RemoveQueueEntryByIndex(OALSource*	thisSource, UInt32	inIndex, bool	inReleaseIt) 
{
	iterator	it = begin();
	ALuint		outBufferToken = 0;

	std::advance(it, inIndex);				
	if (it != end())
	{
		outBufferToken = it->mBufferToken;
		if (inReleaseIt)
			it->mBuffer->ReleaseBuffer(thisSource); // if this release decrements the attchment count of this source to zero, it 
													// will be deleted from the buffers list of attached sources
		erase(it);
	}
	
	return (outBufferToken);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UInt32 	BufferQueue::GetQueueSizeInFrames() 
{
	iterator	it = begin();
	UInt32		totalFrames = 0;

	while (it != end())
	{
		totalFrames += it->mBuffer->GetFrameCount();
		++it;
	}
	
	return (totalFrames);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UInt32 	BufferQueue::GetBufferFrameCount(UInt32	inBufferIndex) 
{
	iterator	it = begin();
	std::advance(it, inBufferIndex);
	if (it != end())
		return (it->mBuffer->GetFrameCount());
		
	return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALuint 	BufferQueue::GetBufferTokenByIndex(UInt32	inBufferIndex) 
{
	iterator	it = begin();
	std::advance(it, inBufferIndex);
	if (it != end())
		return (it->mBuffer->GetToken());
		
	return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void 	BufferQueue::SetFirstBufferOffset(UInt32	inFrameOffset) 
{
	iterator	it = begin();
	if (it == end())
		return;
	
	UInt32		packetOffset = FrameOffsetToPacketOffset(inFrameOffset);
	UInt32		packetSize = GetPacketSize();
	
	it->mOffset = packetOffset * packetSize;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UInt32     BufferQueue::GetPacketSize()  
{
	iterator	it = begin();        
	if (it != end())
		return(it->mBuffer->GetBytesPerPacket());
	return (0);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UInt32 	BufferQueue::FrameOffsetToPacketOffset(UInt32	inFrameOffset) 
{
	return inFrameOffset; // this is correct for pcm which is all we're doing right now
	
	// if non pcm formats are used return the packet that contains inFrameOffset, which may back up the
	// requested frame - round backward not forward
}