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

/*
	Each OALSource object maintains a BufferQueue (list) and an ACMap (multimap). The buffer queue is an ordered list of BufferInfo structs.
	These structs contain an OAL buffer and other pertinent data. The ACMap is a multimap of ACInfo structs. These structs each contain an
	AudioConverter and the input format of the AudioConverter. The AudioConverters are created as needed each time a buffer with a new 
    format is added to the queue. This allows differently formatted data to be queued seemlessly. The correct AC is used for each 
    buffer as the BufferInfo keeps track of the appropriate AC to use.

    All public methods that are not called from within the source object are guarded by a state mutex to ensure the object is not disposed from one
    thread, while running one of it's mthods from another. If the method is also used from within an object method, it takes


*/

#include "oalSource.h"

#define		LOG_PLAYBACK				0
#define		LOG_VERBOSE					0
#define		LOG_BUFFER_QUEUEING			0
#define		LOG_DOPPLER                 0

#define		CALCULATE_POSITION	1	// this should be true except for testing
#define		USE_MUTEXES			1	// this should be true except for testing
#define		WARM_THE_BUFFERS	1	// when playing, touch all the audio data in memory once before it is needed in the render proc  (RealTime thread)

// These are macros for locking around play/stop code:
#define TRY_PLAY_MUTEX										\
	bool wasLocked = false;									\
	try 													\
	{														\
		if (CallingInRenderThread()) {						\
			if (mPlayGuard.Try(wasLocked) == false)	{		\
				goto home;									\
            }												\
		} else {											\
			wasLocked = mPlayGuard.Lock();					\
		}
		

#define UNLOCK_PLAY_MUTEX									\
	home:;													\
        if (wasLocked)										\
			mPlayGuard.Unlock();							\
															\
	} catch (OSStatus stat) {								\
        if (wasLocked)										\
			mPlayGuard.Unlock();							\
        throw stat;                                         \
	} catch (...) {											\
        if (wasLocked)										\
			mPlayGuard.Unlock();							\
        throw -1;                                         	\
	}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// TEMPORARY
// This build flag should be on if you do not have a copy of AudioUnitProperties.h that
// defines the struct MixerDistanceParams and the constant kAudioUnitProperty_3DMixerDistanceParams

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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// OALSource
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ***** OALSources - PUBLIC *****
OALSource::OALSource (const UInt32 	 	inSelfToken, OALContext	*inOwningContext)
	: 	mSelfToken (inSelfToken),
		mOwningContext(inOwningContext),
		mOwningDevice(NULL),
        mStateMutex(kStateMutex),
		mCalculateDistance(true),
		mResetBusFormat(true),
        mResetPitch(true),
		mBufferQueueActive(NULL),
		mBufferQueueInactive(NULL),
        mBufferQueueMutex(kBufferQueueMutex),                     
		mCurrentBufferIndex(0),
		mQueueIsProcessed(true),
		mRenderThreadID (0),
		mPlayGuard ("OALAudioPlayer::Guard"),
		mCurrentPlayBus (kSourceNeedsBus),
		mShouldFade (false),
		mACMap(NULL),
		mOutputSilence(false),
		mLooping(AL_FALSE),
		mSourceRelative(AL_FALSE),
		mConeInnerAngle(360.0),
		mConeOuterAngle(360.0),
		mConeOuterGain(0.0),
        mReadIndex(0.0),
        mTempSourceStorageBufferSize(2048),             // only used if preferred mixer is unavailable
		mState(AL_INITIAL),
		mGain(1.0),
		mPitch(1.0),                                    // this is the user pitch setting changed via the OAL APIs
        mDopplerScaler(1.0),                          
		mRollOffFactor(kDefaultRolloff),
		mReferenceDistance(kDefaultReferenceDistance),
		mMaxDistance(kDefaultMaximumDistance),          // ***** should be MAX_FLOAT
		mMinDistance(1.0),
		mMinGain(0.0),
		mMaxGain(1.0)
{		
    mOwningDevice = mOwningContext->GetOwningDevice();

    mPosition[0] = 0.0;
    mPosition[1] = 0.0;
    mPosition[2] = 0.0;
    
    mVelocity[0] = 0.0;
    mVelocity[1] = 0.0;
    mVelocity[2] = 0.0;

    mDirection[0] = 0.0;
    mDirection[1] = 0.0;
    mDirection[2] = 0.0;

    mBufferQueueActive = new BufferQueue();
    mBufferQueueInactive = new BufferQueue();
    mACMap = new ACMap();

    mReferenceDistance = mOwningDevice->GetDefaultReferenceDistance();
    mMaxDistance = mOwningDevice->GetDefaultMaxDistance();
     
    if (!mOwningDevice->IsPreferredMixerAvailable())
    {
        // since the preferred mixer is not available, some temporary storgae will be needed for SRC
        // for now assume that sources will not have more than 2 channels of data
        mTempSourceStorage = (AudioBufferList *) malloc ((offsetof(AudioBufferList, mBuffers)) + (2 * sizeof(AudioBuffer)));
        mTempSourceStorage->mBuffers[0].mDataByteSize = mTempSourceStorageBufferSize;
        mTempSourceStorage->mBuffers[0].mData = malloc(mTempSourceStorageBufferSize);
        
        mTempSourceStorage->mBuffers[1].mDataByteSize = mTempSourceStorageBufferSize;
        mTempSourceStorage->mBuffers[1].mData = malloc(mTempSourceStorageBufferSize);
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OALSource::~OALSource()
{
#if LOG_VERBOSE
	DebugMessageN1("OALSource::~OALSource() - OALSource = %ld\n", mSelfToken);
#endif

    // make sure the object is not torn if one of it's methods is being executed from another thread
    CAMutex::Locker stateLock(mStateMutex);

	Stop(); // stop any playback that is in progress

    // release the 3DMixer bus if necessary
	if (mCurrentPlayBus != kSourceNeedsBus)
	{
		mOwningDevice->SetBusAsAvailable (mCurrentPlayBus);
		mCurrentPlayBus = kSourceNeedsBus;		
	}
		
    mBufferQueueMutex.Lock();
    
    // empty the two queues
    UInt32  count = mBufferQueueInactive->Size();
    for (UInt32	i = 0; i < count; i++)
    {	
        mBufferQueueInactive->RemoveQueueEntryByIndex(0);
    }
    delete (mBufferQueueInactive);
   
    count = mBufferQueueActive->Size();
    for (UInt32	i = 0; i < count; i++)
    {	
        mBufferQueueActive->RemoveQueueEntryByIndex(0);
    }
    delete (mBufferQueueActive);
    
    mBufferQueueMutex.Unlock();
	
	// remove all the AudioConverters that were created for the buffer queue of this source object
    if (mACMap)
	{
		mACMap->RemoveAllConverters();
		delete (mACMap);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// SET METHODS 
// Any set methods that do not take an isRenderThread argument should not be called from
// another method of the source object
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetPitch (float	inPitch, bool isRenderThread)
{
    // only lock if this is not being called from the render thread
    if (!isRenderThread)
        CAMutex::Locker stateLock(mStateMutex);

    if ((inPitch == mPitch) && (mResetPitch == false))
		return;			// nothing to do
	
	mPitch = inPitch;

    if (!mOwningDevice->IsPreferredMixerAvailable())
        return;         // 1.3 3DMixer does not work properly when doing SRC on a mono bus

	Float32     newPitch = mPitch * mDopplerScaler;
    if (mCurrentPlayBus != kSourceNeedsBus)
	{
		BufferInfo		*oalBuffer = mBufferQueueActive->Get(mCurrentBufferIndex);
		
		if (oalBuffer != NULL)
		{
#if LOG_GRAPH_AND_MIXER_CHANGES
    DebugMessageN2("OALSource::SetPitch: k3DMixerParam_PlaybackRate called - OALSource:mPitch = %ld:%f2\n", mSelfToken, mPitch );
#endif            
			
			OSStatus    result = AudioUnitSetParameter (	mOwningDevice->GetMixerUnit(), k3DMixerParam_PlaybackRate, kAudioUnitScope_Input, mCurrentPlayBus, newPitch /*mPitch*/, 0);
            if (result != noErr)
                DebugMessageN2("OALSource::SetPitch: k3DMixerParam_PlaybackRate called - OALSource:mPitch = %ld:%f2\n", mSelfToken, mPitch );
        }

		mResetPitch = false;
	}
	else
		mResetPitch = true; // the source is not currently connected to a bus, so make this change when play is called
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetGain (float	inGain, bool isRenderThread)
{	
#if LOG_VERBOSE
        DebugMessageN2("OALSource::SetGain - OALSource:inGain = %ld:%f\n", mSelfToken, inGain);
#endif

    // only lock if this is not being called from the render thread
    if (!isRenderThread)
        CAMutex::Locker stateLock(mStateMutex);

	if (mGain != inGain)
	{
		mGain = inGain;
		UpdateBusGain(isRenderThread);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetMinGain (Float32	inMinGain, bool isRenderThread)
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::SetMinGain - OALSource:inMinGain = %ld:%f\n", mSelfToken, inMinGain);
#endif

    // only lock if this is not being called from the render thread
    if (!isRenderThread)
        CAMutex::Locker stateLock(mStateMutex);

	if (mMinGain != inMinGain)
	{
		mMinGain = inMinGain;
		UpdateBusGain(isRenderThread);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetMaxGain (Float32	inMaxGain, bool isRenderThread)
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::SetMaxGain - OALSource:inMaxGain = %ld:%f\n", mSelfToken, inMaxGain);
#endif

    // only lock if this is not being called from the render thread
    if (!isRenderThread)
        CAMutex::Locker stateLock(mStateMutex);

	if (mMaxGain != inMaxGain)
	{
		mMaxGain = inMaxGain;
		UpdateBusGain(isRenderThread);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetReferenceDistance (Float32	inReferenceDistance, bool isRenderThread)
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::SetReferenceDistance - OALSource:inReferenceDistance = %ld/%f2\n", mSelfToken, inReferenceDistance);
#endif

    // only lock if this is not being called from the render thread
    if (!isRenderThread)
        CAMutex::Locker stateLock(mStateMutex);
				
	if (inReferenceDistance == mReferenceDistance)
		return; // nothing to do

	mReferenceDistance = inReferenceDistance;
 	
    if (mCurrentPlayBus != kSourceNeedsBus)
    {
        if (!mOwningDevice->IsPreferredMixerAvailable())	
        {
            // the pre-2.0 3DMixer does not accept kAudioUnitProperty_3DMixerDistanceParams, it has do some extra work and use the DistanceAtten property instead
            mOwningDevice->SetDistanceAttenuation(mCurrentPlayBus, mReferenceDistance, mMaxDistance, mRollOffFactor);
        }
        else
        {
            MixerDistanceParams		distanceParams;
            UInt32					outSize = sizeof(distanceParams);
            OSStatus	result = AudioUnitGetProperty(mOwningDevice->GetMixerUnit(), kAudioUnitProperty_3DMixerDistanceParams, kAudioUnitScope_Input, mCurrentPlayBus, &distanceParams, &outSize);
            if (result == noErr)
            {
                Float32     rollOff = mRollOffFactor;

                if (mOwningDevice->IsDistanceScalingRequired())
                {
                    // scale the reference distance
                    distanceParams.mReferenceDistance = (mReferenceDistance/mMaxDistance) * kDistanceScalar;
                    // limit the max distance
                    distanceParams.mMaxDistance = kDistanceScalar;
                    // scale the rolloff
                    rollOff *= (kDistanceScalar/mMaxDistance);
                }
                else
                    distanceParams.mReferenceDistance = mReferenceDistance;

                distanceParams.mMaxAttenuation = 20 * log10(distanceParams.mReferenceDistance / (distanceParams.mReferenceDistance + (rollOff * (distanceParams.mMaxDistance -  distanceParams.mReferenceDistance))));
                if (distanceParams.mMaxAttenuation < 0.0)
                    distanceParams.mMaxAttenuation *= -1.0;
                else 
                    distanceParams.mMaxAttenuation = 0.0;   // if db result was positive, clamp it to zero
                
                result = AudioUnitSetProperty(mOwningDevice->GetMixerUnit(), kAudioUnitProperty_3DMixerDistanceParams, kAudioUnitScope_Input, mCurrentPlayBus, &distanceParams, sizeof(distanceParams));
            }
        }
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetMaxDistance (Float32	inMaxDistance, bool isRenderThread)
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::SetMaxDistance - OALSource:inMaxDistance = %ld:%f2\n", mSelfToken, inMaxDistance);
#endif

    // only lock if this is not being called from the render thread
    if (!isRenderThread)
        CAMutex::Locker stateLock(mStateMutex);

	if (inMaxDistance == mMaxDistance)
		return; // nothing to do

	mMaxDistance = inMaxDistance;

    if (mCurrentPlayBus != kSourceNeedsBus)
    {
        if (!mOwningDevice->IsPreferredMixerAvailable())	
        {
            // the pre-2.0 3DMixer does not accept kAudioUnitProperty_3DMixerDistanceParams, it has do some extra work and use the DistanceAtten property instead
            mOwningDevice->SetDistanceAttenuation(mCurrentPlayBus, mReferenceDistance, mMaxDistance, mRollOffFactor);
        }
        else
        {
            MixerDistanceParams		distanceParams;
            UInt32					outSize = sizeof(distanceParams);
            OSStatus	result = AudioUnitGetProperty(mOwningDevice->GetMixerUnit(), kAudioUnitProperty_3DMixerDistanceParams, kAudioUnitScope_Input, mCurrentPlayBus, &distanceParams, &outSize);
            if (result == noErr)
            {
                Float32     rollOff = mRollOffFactor;

                if (mOwningDevice->IsDistanceScalingRequired())
                {
                    // scale the reference distance
                    distanceParams.mReferenceDistance = (mReferenceDistance/mMaxDistance) * kDistanceScalar;
                    // limit the max distance
                    distanceParams.mMaxDistance = kDistanceScalar;
                    // scale the rolloff
                    rollOff *= (kDistanceScalar/mMaxDistance);
                }
                else
                    distanceParams.mMaxDistance = mMaxDistance;

                distanceParams.mMaxAttenuation = 20 * log10(distanceParams.mReferenceDistance / (distanceParams.mReferenceDistance + (rollOff * (distanceParams.mMaxDistance -  distanceParams.mReferenceDistance))));
                if (distanceParams.mMaxAttenuation < 0.0)
                    distanceParams.mMaxAttenuation *= -1.0;
                else 
                    distanceParams.mMaxAttenuation = 0.0;   // if db result was positive, clamp it to zero
                
                result = AudioUnitSetProperty(mOwningDevice->GetMixerUnit(), kAudioUnitProperty_3DMixerDistanceParams, kAudioUnitScope_Input, mCurrentPlayBus, &distanceParams, sizeof(distanceParams));
            }
        }
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetRollOffFactor (Float32	inRollOffFactor, bool isRenderThread)
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::SetRollOffFactor - OALSource:inRollOffFactor = %ld:%f\n", mSelfToken, inRollOffFactor);
#endif

    // only lock if this is not being called from the render thread
    if (!isRenderThread)
        CAMutex::Locker stateLock(mStateMutex);
		        
	if (inRollOffFactor == mRollOffFactor)
		return; // nothing to do

	mRollOffFactor = inRollOffFactor;
 	
    if (mCurrentPlayBus != kSourceNeedsBus)
    {
        if (!mOwningDevice->IsPreferredMixerAvailable())	
        {
            // the pre-2.0 3DMixer does not accept kAudioUnitProperty_3DMixerDistanceParams, it has do some extra work and use the DistanceAtten property instead
            mOwningDevice->SetDistanceAttenuation(mCurrentPlayBus, mReferenceDistance, mMaxDistance, mRollOffFactor);
        }
        else
        {
			MixerDistanceParams		distanceParams;
			UInt32					outSize = sizeof(distanceParams);
			OSStatus	result = AudioUnitGetProperty(mOwningDevice->GetMixerUnit(), kAudioUnitProperty_3DMixerDistanceParams, kAudioUnitScope_Input, mCurrentPlayBus, &distanceParams, &outSize);
			if (result == noErr)
			{
                Float32     rollOff = mRollOffFactor;

                if (mOwningDevice->IsDistanceScalingRequired())
                {
                    // scale the reference distance
                    distanceParams.mReferenceDistance = (mReferenceDistance/mMaxDistance) * kDistanceScalar;
                    // limit the max distance
                    distanceParams.mMaxDistance = kDistanceScalar;
                    // scale the rolloff
                    rollOff *= (kDistanceScalar/mMaxDistance);
                }
				
                distanceParams.mMaxAttenuation = 20 * log10(distanceParams.mReferenceDistance / (distanceParams.mReferenceDistance + (rollOff * (distanceParams.mMaxDistance -  distanceParams.mReferenceDistance))));                
                if (distanceParams.mMaxAttenuation < 0.0)
					distanceParams.mMaxAttenuation *= -1.0;
				else 
					distanceParams.mMaxAttenuation = 0.0;   // if db result was positive, clamp it to zero
				
                result = AudioUnitSetProperty(mOwningDevice->GetMixerUnit(), kAudioUnitProperty_3DMixerDistanceParams, kAudioUnitScope_Input, mCurrentPlayBus, &distanceParams, sizeof(distanceParams));
			}
        }
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetLooping (UInt32	inLooping, bool isRenderThread)
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::SetLooping called - OALSource:inLooping = %ld:%ld\n", mSelfToken, inLooping);
#endif

    // only lock if this is not being called from the render thread
    if (!isRenderThread)
        CAMutex::Locker stateLock(mStateMutex);
    
    mLooping = inLooping;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetPosition (Float32 inX, Float32 inY, Float32 inZ, bool isRenderThread)
{
#if LOG_VERBOSE
	DebugMessageN4("OALSource::SetPosition called - OALSource:X:Y:Z = %ld:%f:%f:%f\n", mSelfToken, inX, inY, inZ);
#endif

    // only lock if this is not being called from the render thread
    if (!isRenderThread)
        CAMutex::Locker stateLock(mStateMutex);

	mPosition[0] = inX;
	mPosition[1] = inY;
	mPosition[2] = inZ;

	mCalculateDistance = true;  // change the distance next time the PreRender proc or a new Play() is called
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetVelocity (Float32 inX, Float32 inY, Float32 inZ, bool isRenderThread)
{
#if LOG_VERBOSE
	DebugMessageN1("OALSource::SetVelocity called - OALSource = %ld\n", mSelfToken);
#endif

    // only lock if this is not being called from the render thread
    if (!isRenderThread)
        CAMutex::Locker stateLock(mStateMutex);

	mVelocity[0] = inX;
	mVelocity[1] = inY;
	mVelocity[2] = inZ;

	mCalculateDistance = true;  // change the velocity next time the PreRender proc or a new Play() is called
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetDirection (Float32 inX, Float32 inY, Float32 inZ, bool isRenderThread)
{
#if LOG_VERBOSE
	DebugMessageN1("OALSource::SetDirection called - OALSource = %ld\n", mSelfToken);
#endif

    // only lock if this is not being called from the render thread
    if (!isRenderThread)
        CAMutex::Locker stateLock(mStateMutex);

	mDirection[0] = inX;
	mDirection[1] = inY;
	mDirection[2] = inZ;

	mCalculateDistance = true;  // change the direction next time the PreRender proc or a new Play() is called
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetSourceRelative (UInt32	inSourceRelative, bool isRenderThread)
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::SetSourceRelative called - OALSource:inSourceRelative = %ld:%ld\n", mSelfToken, inSourceRelative);
#endif
    // only lock if this is not being called from the render thread
    if (!isRenderThread)
        CAMutex::Locker stateLock(mStateMutex);

	mSourceRelative = inSourceRelative;
	mCalculateDistance = true;  // change the source relative next time the PreRender proc or a new Play() is called
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetChannelParameters (bool isRenderThread)	
{
#if LOG_VERBOSE
	DebugMessageN1("OALSource::SetChannelParameters called - OALSource = %ld\n", mSelfToken);
#endif

    // only lock if this is not being called from the render thread
    if (!isRenderThread)
        CAMutex::Locker stateLock(mStateMutex);

    mCalculateDistance = true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetConeInnerAngle (Float32	inConeInnerAngle, bool isRenderThread)
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::SetConeInnerAngle called - OALSource:inConeInnerAngle = %ld:%f2\n", mSelfToken, inConeInnerAngle);
#endif

    // only lock if this is not being called from the render thread
    if (!isRenderThread)
        CAMutex::Locker stateLock(mStateMutex);

    mConeInnerAngle = inConeInnerAngle;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetConeOuterAngle (Float32	inConeOuterAngle, bool isRenderThread)
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::SetConeOuterAngle called - OALSource:inConeOuterAngle = %ld:%f2\n", mSelfToken, inConeOuterAngle);
#endif

    // only lock if this is not being called from the render thread
    if (!isRenderThread)
        CAMutex::Locker stateLock(mStateMutex);

    mConeOuterAngle = inConeOuterAngle;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::SetConeOuterGain (Float32	inConeOuterGain, bool isRenderThread)
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::SetConeOuterGain called - OALSource:inConeOuterGain = %ld:%f2\n", mSelfToken, inConeOuterGain);
#endif

    // only lock if this is not being called from the render thread
    if (!isRenderThread)
        CAMutex::Locker stateLock(mStateMutex);
    
    mConeOuterGain = inConeOuterGain;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// GET METHODS 
// Any get methods that do not take an isRenderThread argument should not be called from
// another method of the source object
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Float32	OALSource::GetPitch ()
{
    CAMutex::Locker stateLock(mStateMutex);
    return mPitch;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Float32	OALSource::GetDopplerScaler ()
{
    CAMutex::Locker stateLock(mStateMutex);
    return mDopplerScaler;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Should only be called from the API set (which should not be th erender thread)
Float32	OALSource::GetGain ()
{
    CAMutex::Locker stateLock(mStateMutex);    
    return mGain;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Should only be called from the API set (which should not be th erender thread)
Float32	OALSource::GetMinGain ()
{
    CAMutex::Locker stateLock(mStateMutex);
    return mMinGain;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Should only be called from the API set (which should not be th erender thread)
Float32	OALSource::GetMaxGain ()
{
    CAMutex::Locker stateLock(mStateMutex);
    return mMaxGain;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Should only be called from the API set (which should not be th erender thread)
Float32	OALSource::GetReferenceDistance ()
{
    CAMutex::Locker stateLock(mStateMutex);    
    return mReferenceDistance;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Should only be called from the API set (which should not be th erender thread)
Float32	OALSource::GetMaxDistance ()
{
    CAMutex::Locker stateLock(mStateMutex);    
    return mMaxDistance;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Should only be called from the API set (which should not be th erender thread)
Float32	OALSource::GetRollOffFactor ()
{
    CAMutex::Locker stateLock(mStateMutex);
    return mRollOffFactor;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Should only be called from the API set (which should not be th erender thread)
UInt32	OALSource::GetLooping ()
{
    CAMutex::Locker stateLock(mStateMutex);
    return mLooping; 
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::GetPosition (Float32 &inX, Float32 &inY, Float32 &inZ, bool isRenderThread)
{
#if LOG_VERBOSE
	DebugMessageN1("OALSource::GetPosition called - OALSource = %ld\n", mSelfToken);
#endif

    // only lock if this is not being called from the render thread
    if (!isRenderThread)
        CAMutex::Locker stateLock(mStateMutex);

	inX = mPosition[0];
	inY = mPosition[1];
	inZ = mPosition[2];
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::GetVelocity (Float32 &inX, Float32 &inY, Float32 &inZ, bool isRenderThread)
{
#if LOG_VERBOSE
	DebugMessageN1("OALSource::GetVelocity called - OALSource = %ld\n", mSelfToken);
#endif
    // only lock if this is not being called from the render thread
    if (!isRenderThread)
        CAMutex::Locker stateLock(mStateMutex);

	inX = mVelocity[0];
	inY = mVelocity[1];
	inZ = mVelocity[2];
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::GetDirection (Float32 &inX, Float32 &inY, Float32 &inZ, bool isRenderThread)
{
#if LOG_VERBOSE
	DebugMessageN1("OALSource::GetDirection called - OALSource = %ld\n", mSelfToken);
#endif
    // only lock if this is not being called from the render thread
    if (!isRenderThread)
        CAMutex::Locker stateLock(mStateMutex);

	inX = mDirection[0];
	inY = mDirection[1];
	inZ = mDirection[2];
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Should only be called from the API set (which should not be th erender thread)
UInt32	OALSource::GetSourceRelative ()
{
    CAMutex::Locker stateLock(mStateMutex);
    return mSourceRelative;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Should only be called from the API set (which should not be th erender thread)
Float32	OALSource::GetConeInnerAngle ()
{
    CAMutex::Locker stateLock(mStateMutex);
    return mConeInnerAngle;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Should only be called from the API set (which should not be th erender thread)
Float32	OALSource::GetConeOuterAngle ()
{
    CAMutex::Locker stateLock(mStateMutex);
    return mConeOuterAngle;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Should only be called from the API set (which should not be th erender thread)
Float32	OALSource::GetConeOuterGain ()
{
    CAMutex::Locker stateLock(mStateMutex);
    return mConeOuterGain;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Should only be called from the API set (which should not be th erender thread)
UInt32	OALSource::GetState()
{
    CAMutex::Locker stateLock(mStateMutex);
    return mState;
}
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Should only be called from the API set (which should not be th erender thread)
UInt32	OALSource::GetToken()
{
    CAMutex::Locker stateLock(mStateMutex);
    return mSelfToken;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// BUFFER QUEUE METHODS 
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UInt32	OALSource::GetQLength()
{
    // as far as the user is concerned, the Q length is the size of the inactive & active lists
    UInt32  returnValue = 0;
    
    CAMutex::Locker qMutex(mBufferQueueMutex);

    returnValue = mBufferQueueInactive->Size() + mBufferQueueActive->Size();

#if LOG_BUFFER_QUEUEING
	DebugMessageN2("OALSource::GetQLength called - OALSource:QLength = %ld:%ld\n", mSelfToken, returnValue);
#endif

	return (returnValue);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UInt32	OALSource::GetBuffersProcessed()
{
    UInt32  returnValue = 0;
	if (mState == AL_INITIAL)
        return(0);
    else
    {
        CAMutex::Locker qMutex(mBufferQueueMutex);
        returnValue = mBufferQueueInactive->Size();
    }

#if LOG_BUFFER_QUEUEING
	DebugMessageN2("OALSource::GetBuffersProcessed called - OALSource:ProcessedCount = %ld:%ld\n", mSelfToken, returnValue);
#endif
    
	return (returnValue);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// This should not be called from the render thread, the caller can wait on the lock until it is safe to
// touch both active and inactive lists
void	OALSource::SetBuffer (UInt32 inBufferToken, OALBuffer	*inBuffer)
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::SetBuffer called - OALSource:inBufferToken = %ld:%ld\n", mSelfToken, inBufferToken);
#endif
    
    CAMutex::Locker stateLock(mBufferQueueMutex);
    
    // empty the two queues
    UInt32  count = mBufferQueueInactive->Size();
    for (UInt32	i = 0; i < count; i++)
    {	
        mBufferQueueInactive->RemoveQueueEntryByIndex(0);
    }
   
    count = mBufferQueueActive->Size();
    for (UInt32	i = 0; i < count; i++)
    {	
        mBufferQueueActive->RemoveQueueEntryByIndex(0);
    }
    	
	// if inBufferToken == 0, do nothing, passing NONE to this method is legal and should merely empty the queue
	if (inBufferToken != 0)
	{
		AppendBufferToQueue(inBufferToken, inBuffer);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UInt32	OALSource::GetBuffer ()
{
#if LOG_VERBOSE
	DebugMessageN2("OALSource::GetBuffer called - OALSource:currentBuffer = %ld:%ld\n", mSelfToken, mBufferQueueActive->GetBufferTokenByIndex(mCurrentBufferIndex));
#endif
	// for now, return the currently playing buffer in an active source, or the 1st buffer of the Q in an inactive source
	return (mBufferQueueActive->GetBufferTokenByIndex(mCurrentBufferIndex));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// This should NOT be called from the render thread
void	OALSource::AppendBufferToQueue(UInt32	inBufferToken, OALBuffer	*inBuffer)
{
#if LOG_BUFFER_QUEUEING
	DebugMessageN2("OALSource::AppendBufferToQueue called - OALSource:inBufferToken = %ld:%ld\n", mSelfToken, inBufferToken);
#endif

    CAMutex::Locker qMutex(mBufferQueueMutex);
    
	OSStatus	result = noErr;	
	try {
		if (mBufferQueueActive->Empty())
		{
			mCurrentBufferIndex = 0;	// this is the first buffer in the queue
			mQueueIsProcessed = false;
		}
								
		// do we need an AC for the format of this buffer?
		if (inBuffer->mDataHasBeenConverted)
		{
			// the data was already convertered to the mixer format, no AC is necessary (as indicated by the ACToken setting of zero)
			mBufferQueueActive->AppendBuffer(inBufferToken, inBuffer, 0);
		}
		else
		{			
			// check the format against the real format of the data, NOT the input format of the converter which may be subtly different
			// both in SampleRate and Interleaved flags
			UInt32		outToken = 0;
			mACMap->GetACForFormat(inBuffer->mDataFormat, outToken);
			if (outToken == 0)
			{
				// create an AudioConverter for this format because there isn't one yet
				AudioConverterRef				converter = 0;
				CAStreamBasicDescription		inFormat;
				
				inFormat.SetFrom(inBuffer->mDataFormat);
					
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
				info.mInputFormat = inBuffer->mDataFormat;
				info.mConverter = converter;
				
				// add this AudioConverter to the source's ACMap
				UInt32	newACToken = GetNewToken();
				mACMap->Add(newACToken, &info);
				// add the buffer to the queue - each buffer now knows which AC to use when it is converted in the render proc
				mBufferQueueActive->AppendBuffer(inBufferToken, inBuffer, newACToken);
			}
			else
			{
				// there is already an AC for this buffer's data format, so just append the buffer to the queue
				mBufferQueueActive->AppendBuffer(inBufferToken, inBuffer, outToken);
			}
		}
	}
	catch (OSStatus	 result) {
		DebugMessageN1("APPEND BUFFER FAILED %ld\n", mSelfToken);
		throw (result);
	}

#if LOG_BUFFER_QUEUEING
	DebugMessageN2("OALSource::AppendBufferToQueue called - OALSource:QLength = %ld:%ld\n", mSelfToken, mBufferQueueInactive->Size() + mBufferQueueActive->Size());
#endif

	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Do NOT call this from the render thread
void	OALSource::RemoveBuffersFromQueue(UInt32	inCount, UInt32	*outBufferTokens)
{
	if (inCount == 0)
		return;

#if LOG_BUFFER_QUEUEING
	DebugMessageN2("OALSource::RemoveBuffersFromQueue called - OALSource:inCount = %ld:%ld\n", mSelfToken, inCount);
#endif
	
	try {
		// 1) do nothing if queue is playing and in loop mode - to risky that a requested unqueue buffer will be needed
		// 2) do nothing if any of the requested buffers is in progress or not processed and queue is PLAYING
		// 3) if source state is not playing or paused, do the request if the buffer count is valid
		
		if ((mState == AL_PLAYING) || (mState == AL_PAUSED))
		{			
			if (mLooping == true)
			{
				// case #1
				throw ((OSStatus) AL_INVALID_OPERATION);		
			}
		}
        
        // it is safe to remove the buffers from the queue right now because the source is not rendering
        CAMutex::Locker qMutex(mBufferQueueMutex);

        // see if there are enough buffers in the inactive list to satisfy the request
        if (inCount > mBufferQueueInactive->Size())
        {
            mBufferQueueMutex.Unlock();
            throw ((OSStatus) AL_INVALID_OPERATION);		
        }
        
        if (!mBufferQueueInactive->Empty())
        {
            for (UInt32	i = 0; i < inCount; i++)
            {	
                UInt32		outToken = mBufferQueueInactive->RemoveQueueEntryByIndex(0);
                                    
                if (outBufferTokens)
                    outBufferTokens[i] = outToken;
            }
        }
	}
	catch (OSStatus	 result) {
		DebugMessageN1("REMOVE BUFFER FAILED, OALSource = %ld\n", mSelfToken);
		throw (result);
	}

#if LOG_BUFFER_QUEUEING
	DebugMessageN2("OALSource:RemoveBuffersFromQueue called - OALSource:QLength = %ld:%ld\n", mSelfToken, mBufferQueueInactive->Size() + mBufferQueueActive->Size());
#endif

	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// PLAYBACK METHODS 
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::Play()
{
    UInt32  outSize;
    
	if (!mOwningDevice->IsPlayable())
		return;
		
	try {
        // reset some of the class members before we start playing
		Reset();			

#if LOG_PLAYBACK
		DebugMessageN3("OALSource::Play called - OALSource:QSize:Looping = %ld:%ld:%ld\n", mSelfToken, mBufferQueueActive->Size(), mLooping);
#endif

		OSStatus result = noErr;
		
        if (mBufferQueueActive->Empty())
			return; // there isn't anything to do
		
		// ******* Get the format for the first buffer in the queue
		mCurrentBufferIndex = 0;
		
		// find the first real buffer in the queue because it is legal to start the queue with an AL_NONE buffer
		while (mBufferQueueActive->GetBufferTokenByIndex(mCurrentBufferIndex) == AL_NONE)
		{
			// mark buffer as processed
			mBufferQueueActive->SetBufferAsProcessed(mCurrentBufferIndex);
			mCurrentBufferIndex++;
		}
	
		BufferInfo		*buffer = mBufferQueueActive->Get(mCurrentBufferIndex);
        if (buffer == NULL)
            throw AL_INVALID_OPERATION;
            
#if LOG_PLAYBACK
			DebugMessage("OALSource::Play called - Format of 1st buffer in the Q = \n");
			buffer->mBuffer->mDataFormat.Print();
#endif
	
#if WARM_THE_BUFFERS
		// touch the memory now instead of in the render proc, so audio data will be paged in if it's currently in VM
		volatile UInt32	X;
		UInt32		*start = (UInt32 *)buffer->mBuffer->mData;
		UInt32		*end = (UInt32 *)(buffer->mBuffer->mData + (buffer->mBuffer->mDataSize &0xFFFFFFFC));
		while (start < end)
		{
			X = *start; 
			start += 1024;
		}		
#endif
		
		if (buffer->mBuffer->mDataHasBeenConverted == false)
			AudioConverterReset(mACMap->Get(buffer->mACToken));
		
		if (mCurrentPlayBus == kSourceNeedsBus)
		{
			// the bus stream format will get set if necessary while getting the available bus
            mCurrentPlayBus = (buffer->mBuffer->mDataFormat.NumberChannels() == 1) ? mOwningDevice->GetAvailableMonoBus() : mOwningDevice->GetAvailableStereoBus();                
            
			Float32     rollOff = mRollOffFactor;
			Float32     refDistance = mReferenceDistance;
			Float32     maxDistance = mMaxDistance;

            if (mOwningDevice->IsDistanceScalingRequired())
            {
                refDistance = (mReferenceDistance/mMaxDistance) * kDistanceScalar;
                maxDistance = kDistanceScalar;
                rollOff *= (kDistanceScalar/mMaxDistance);
            }
            
            Float32	testAttenuation  = 20 * log10(mReferenceDistance / (mReferenceDistance + (mRollOffFactor * (mMaxDistance -  mReferenceDistance))));
            if (testAttenuation < 0.0)
                testAttenuation *= -1.0;
            else 
                testAttenuation = 0.0;   // if db result was positive, clamp it to zero

			if (mOwningDevice->IsPreferredMixerAvailable())
			{
				// Set the MixerDistanceParams for the new bus if necessary
				MixerDistanceParams		distanceParams;
				outSize = sizeof(distanceParams);
				result = AudioUnitGetProperty(mOwningDevice->GetMixerUnit(), kAudioUnitProperty_3DMixerDistanceParams, kAudioUnitScope_Input, mCurrentPlayBus, &distanceParams, &outSize);

				if  ((result == noErr) 	&& ((distanceParams.mReferenceDistance != refDistance)      ||
											(distanceParams.mMaxDistance != maxDistance)            ||
											(distanceParams.mMaxAttenuation != testAttenuation)))

				{
					distanceParams.mMaxAttenuation = testAttenuation;
                
                    if (mOwningDevice->IsDistanceScalingRequired())
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
					
 					result = AudioUnitSetProperty(mOwningDevice->GetMixerUnit(), kAudioUnitProperty_3DMixerDistanceParams, kAudioUnitScope_Input, mCurrentPlayBus, &distanceParams, sizeof(distanceParams));
				}
			}
            else
            {
                // the pre-2.0 3DMixer does not accept kAudioUnitProperty_3DMixerDistanceParams, it has do some extra work and use the DistanceAtten property instead
                mOwningDevice->SetDistanceAttenuation(mCurrentPlayBus, mReferenceDistance, mMaxDistance, mRollOffFactor);
            }
		}	

        // get the sample rate of the bus
        CAStreamBasicDescription    desc;
        outSize = sizeof(desc);
        result = AudioUnitGetProperty(mOwningDevice->GetMixerUnit(), kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, mCurrentPlayBus, &desc, &outSize);
        
		mResetPitch = true;	
		mCalculateDistance = true;				
        if (desc.mSampleRate != buffer->mBuffer->mDataFormat.mSampleRate)
            mResetBusFormat = true;     // only reset the bus stream format if it is different than sample rate of the data   		
		
		// *************************** Set properties for the mixer bus
		
#if USE_MUTEXES
		TRY_PLAY_MUTEX
#endif												

		ChangeChannelSettings(false);
		UpdateBusGain(false);			

		mShouldFade = false;
		mState = AL_PLAYING;
		mQueueIsProcessed = false;

		// set the render callback for the player AU
		mPlayCallback.inputProc = SourceInputProc;
		mPlayCallback.inputProcRefCon = this;
		result = AudioUnitSetProperty (	mOwningDevice->GetMixerUnit(), kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 
										mCurrentPlayBus, &mPlayCallback, sizeof(mPlayCallback));
			THROW_RESULT
		
		result = AudioUnitAddRenderNotify(mOwningDevice->GetMixerUnit(), SourceNotificationProc, this);
			THROW_RESULT	
						
#if USE_MUTEXES
			UNLOCK_PLAY_MUTEX	
#endif
	}
	catch (OSStatus	result) {
		DebugMessageN2("PLAY FAILED source = %ld, err = %ld\n", mSelfToken, result);
		throw (result);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::Pause()
{
#if LOG_PLAYBACK
	DebugMessageN1("OALSource::Pause called - OALSource = %ld\n", mSelfToken);
#endif
#if USE_MUTEXES
	TRY_PLAY_MUTEX
#endif
	if (mCurrentPlayBus != kSourceNeedsBus)
	{            		
		AudioUnitRemoveRenderNotify(mOwningDevice->GetMixerUnit(), SourceNotificationProc,this);
					
		mPlayCallback.inputProc = 0;
		mPlayCallback.inputProcRefCon = 0;
		AudioUnitSetProperty (	mOwningDevice->GetMixerUnit(), kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 
								mCurrentPlayBus, &mPlayCallback, sizeof(mPlayCallback));

		mState = AL_PAUSED;
	}
	
#if USE_MUTEXES
	UNLOCK_PLAY_MUTEX
#endif
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::Resume()
{
#if LOG_PLAYBACK
	DebugMessageN1("OALSource::Resume called - OALSource = %ld\n", mSelfToken);
#endif

#if USE_MUTEXES
	TRY_PLAY_MUTEX
#endif
	
	if (mState = AL_PAUSED)
	{			
		OSStatus result = noErr;

		mPlayCallback.inputProc = SourceInputProc;
		mPlayCallback.inputProcRefCon = this;
		// set the render callback for the player AU
		result = AudioUnitSetProperty (	mOwningDevice->GetMixerUnit(), kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 
										mCurrentPlayBus, &mPlayCallback, sizeof(mPlayCallback));
			THROW_RESULT
		
		result = AudioUnitAddRenderNotify(mOwningDevice->GetMixerUnit(), SourceNotificationProc, this);
			THROW_RESULT	
				
		mState = AL_PLAYING;
	} 

#if USE_MUTEXES
	UNLOCK_PLAY_MUTEX
#endif
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::Stop()
{
#if LOG_PLAYBACK
	DebugMessageN1("OALSource::Stop called - OALSource = %ld\n", mSelfToken);
#endif

#if USE_MUTEXES
	TRY_PLAY_MUTEX
#endif
	if ((mCurrentPlayBus != kSourceNeedsBus) && (mState != AL_STOPPED))
	{            
		AudioUnitRemoveRenderNotify(mOwningDevice->GetMixerUnit(), SourceNotificationProc,this);
					
#if LOG_GRAPH_AND_MIXER_CHANGES
	DebugMessageN1("OALSource::Stop: set kAudioUnitProperty_SetRenderCallback (removing) called - OALSource = %ld\n", mSelfToken);
#endif
		mPlayCallback.inputProc = 0;
		mPlayCallback.inputProcRefCon = 0;
		AudioUnitSetProperty (	mOwningDevice->GetMixerUnit(), kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 
								mCurrentPlayBus, &mPlayCallback, sizeof(mPlayCallback));
							
		mOwningDevice->SetBusAsAvailable (mCurrentPlayBus);
		mCurrentPlayBus = kSourceNeedsBus;		

        Reset();    // join the buffer lists and mark the buffers as unprocessed
        mState = AL_STOPPED;
	}
    
	
#if USE_MUTEXES
	UNLOCK_PLAY_MUTEX
#endif
}

#pragma mark ***** OALSources - PRIVATE *****
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// must be called when the source has a mixer bus
void	OALSource::ChangeChannelSettings(bool isRenderThread)
{
#if CALCULATE_POSITION

	if (mCalculateDistance == true)
	{
        BufferInfo	*bufferInfo = mBufferQueueActive->Get(mCurrentBufferIndex);	
		if (bufferInfo)
		{		
#if LOG_GRAPH_AND_MIXER_CHANGES
	DebugMessageN1("OALSource::ChangeChannelSettings: k3DMixerParam_Azimuth/k3DMixerParam_Distance called - OALSource = %ld\n", mSelfToken);
#endif
			// only calculate position if sound is mono - stereo sounds get no location changes
			if ( bufferInfo->mBuffer->mDataFormat.NumberChannels() == 1)
			{
				Float32 	rel_azimuth, rel_distance, dopplerShift;
				
				CalculateDistanceAndAzimuth(&rel_distance, &rel_azimuth, &dopplerShift, isRenderThread);
                if(dopplerShift != mDopplerScaler)
                {
                    mDopplerScaler = dopplerShift;
                    mResetPitch = true;
                }
				
				if (isnan(rel_azimuth))
				{
					rel_azimuth = 0.0;	// DO NOT pass a NAN to the mixer for azimuth
					DebugMessage("ERROR: OALSOurce::ChangeChannelSettings - CalculateDistanceAndAzimuth returned a NAN for rel_azimuth\n");
				}
				
				AudioUnitSetParameter(mOwningDevice->GetMixerUnit(), k3DMixerParam_Azimuth, kAudioUnitScope_Input, mCurrentPlayBus, rel_azimuth, 0);
				
                // do not change the distance if the user has set the distance model to AL_NONE
                if (mOwningContext->GetDistanceModel() == AL_NONE)
                {
                    rel_distance = mReferenceDistance;
                }
                else
                {
                    if (mOwningDevice->IsDistanceScalingRequired())
                        rel_distance *= (kDistanceScalar/mMaxDistance);
                }
                                        
                AudioUnitSetParameter(mOwningDevice->GetMixerUnit(), k3DMixerParam_Distance, kAudioUnitScope_Input, mCurrentPlayBus, rel_distance, 0);
			}
		}
		
		mCalculateDistance = false;
	}
		
#endif	// end CALCULATE_POSITION

	SetPitch (mPitch, isRenderThread);
    UpdateBusFormat(isRenderThread);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::Reset()
{
#if LOG_VERBOSE
	DebugMessageN1("OALSource::Reset called - OALSource = %ld\n", mSelfToken);
#endif
	JoinBufferLists();
	mState = AL_INITIAL;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// this method should only be called when a looping queue reaches it's end and needs to start over (called from render thread)
void	OALSource::LoopToBeginning()
{
#if LOG_VERBOSE
	DebugMessageN1("OALSource::LoopToBeginning called - OALSource = %ld\n", mSelfToken);
#endif
    // swap the list pointers now
    BufferQueue*        tQ  = mBufferQueueActive;
    mBufferQueueActive  = mBufferQueueInactive;
    mBufferQueueInactive = tQ;

    //mShouldFade = true;

    bool    gotTheLock;
    mBufferQueueMutex.Try(gotTheLock);
    mBufferQueueActive->ResetBuffers(); 	// mark all the buffers as unprocessed
    mCurrentBufferIndex = 0;                // start at the first buffer in the queue
    mQueueIsProcessed = false;					
    if (gotTheLock)
        mBufferQueueMutex.Unlock();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// this method should only be called from a non playing state and is used to rejoin the 2 buffer Q lists
void	OALSource::JoinBufferLists()
{	
#if LOG_VERBOSE
	DebugMessageN1("OALSource::JoinBufferLists called - OALSource = %ld\n", mSelfToken);
#endif

    bool    gotTheLock;
    // Try and get the mutex so this buffer can be moved to the inactive list
    if (mBufferQueueMutex.Try(gotTheLock))
    {
        UInt32  count = mBufferQueueActive->Size();
        for (UInt32 i = 0; i < count; i++)
        {
            // Get buffer #i from Active List
            BufferInfo	*staleBufferInfo = mBufferQueueActive->Get(mCurrentBufferIndex);
            mBufferQueueActive->SetBufferAsProcessed(mCurrentBufferIndex);
            // Append it to Inactive List
            mBufferQueueInactive->AppendBuffer(staleBufferInfo->mBufferToken, staleBufferInfo->mBuffer, staleBufferInfo->mACToken);
            // Remove it from Active List
            mBufferQueueActive->RemoveQueueEntryByIndex(mCurrentBufferIndex);
        }
        
        // swap the list pointers now
        BufferQueue*        tQ  = mBufferQueueActive;
        mBufferQueueActive  = mBufferQueueInactive;
        mBufferQueueInactive = tQ;
        
        //mShouldFade = true;
        mBufferQueueActive->ResetBuffers(); 	// mark all the buffers as unprocessed
        mCurrentBufferIndex = 0;                // start at the first buffer in the queue
        mQueueIsProcessed = false;					
        
        if (gotTheLock)
            mBufferQueueMutex.Unlock();
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::UpdateBusGain (bool isRenderThread)
{
    // only lock if this is not being called from the render thread
    if (!isRenderThread)
        CAMutex::Locker stateLock(mStateMutex);

	if (mCurrentPlayBus != kSourceNeedsBus)
	{
		Float32		busGain;
		
		// figure out which gain to actually use
		if (mMinGain > mGain)
			busGain = mMinGain;
		else if (mMaxGain < mGain)
			busGain = mMaxGain;
		else
			busGain = mGain;
		
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
			
#if LOG_GRAPH_AND_MIXER_CHANGES
	DebugMessageN2("OALSource::UpdateBusGain: k3DMixerParam_Gain called - OALSource:busGain = %ld:%f2\n", mSelfToken, busGain );
#endif
			OSStatus	result = AudioUnitSetParameter (	mOwningDevice->GetMixerUnit(), k3DMixerParam_Gain, kAudioUnitScope_Input, mCurrentPlayBus, db, 0);
				THROW_RESULT
		}
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::UpdateBusFormat (bool isRenderThread)
{
#if LOG_VERBOSE
        DebugMessageN1("OALSource::UpdateBusFormat - OALSource = %ld\n", mSelfToken);
#endif

    // only lock if this is not being called from the render thread
    if (!isRenderThread)
        CAMutex::Locker stateLock(mStateMutex);

 	if (!mOwningDevice->IsPreferredMixerAvailable())	// the pre-2.0 3DMixer cannot change stream formats once initialized
		return;
		
    if (mResetBusFormat)
    {
        CAStreamBasicDescription    desc;
        UInt32  outSize = sizeof(desc);
        OSStatus result = AudioUnitGetProperty(mOwningDevice->GetMixerUnit(), kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, mCurrentPlayBus, &desc, &outSize);
        if (result == noErr)
        {
            BufferInfo	*buffer = mBufferQueueActive->Get(mCurrentBufferIndex);	
            if (buffer != NULL)
            {
                desc.mSampleRate = buffer->mBuffer->mDataFormat.mSampleRate;
                AudioUnitSetProperty(mOwningDevice->GetMixerUnit(), kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, mCurrentPlayBus, &desc, sizeof(desc));
                mResetBusFormat = false;
            }
        }
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALSource::PlayFinished()
{
#if LOG_PLAYBACK
	DebugMessageN1("OALSource::PlayFinished called - OALSource = %ld\n", mSelfToken);
#endif

#if USE_MUTEXES
	TRY_PLAY_MUTEX
#endif
	if (mCurrentPlayBus != kSourceNeedsBus)
	{                        	
		AudioUnitRemoveRenderNotify(mOwningDevice->GetMixerUnit(), SourceNotificationProc,this);
					
		mPlayCallback.inputProc = 0;
		mPlayCallback.inputProcRefCon = 0;
#if LOG_GRAPH_AND_MIXER_CHANGES
	DebugMessageN1("OALSource::Stop: set kAudioUnitProperty_SetRenderCallback (removing) called - OALSource = %ld\n", mSelfToken);
#endif
		AudioUnitSetProperty (	mOwningDevice->GetMixerUnit(), kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 
								mCurrentPlayBus, &mPlayCallback, sizeof(mPlayCallback));

		mOwningDevice->SetBusAsAvailable (mCurrentPlayBus);
		mCurrentPlayBus = kSourceNeedsBus;		
	}

#if USE_MUTEXES
	UNLOCK_PLAY_MUTEX
#endif
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus	OALSource::SourceNotificationProc (	void 						*inRefCon, 
												AudioUnitRenderActionFlags 	*inActionFlags,
												const AudioTimeStamp 		*inTimeStamp, 
												UInt32 						inBusNumber,
												UInt32 						inNumberFrames, 
												AudioBufferList 			*ioData)
{
	OALSource* THIS = (OALSource*)inRefCon;
	
	if (*inActionFlags & kAudioUnitRenderAction_PreRender)
		return THIS->DoPreRender();
		
	if (*inActionFlags & kAudioUnitRenderAction_PostRender)
		return THIS->DoPostRender();
		
	return (noErr);
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
		
	if (THIS->mOutputSilence)
		*inActionFlags |= kAudioUnitRenderAction_OutputIsSilence;	
	else
		*inActionFlags &= 0xEF; // the mask for the kAudioUnitRenderAction_OutputIsSilence bit
		
#if USE_AU_TRACER
	syscall(180, tracerStart, inBusNumber, ioData->mNumberBuffers, 0, 0);
#endif

	OSStatus result = noErr;

    if (THIS->mOwningDevice->IsPreferredMixerAvailable())
        result = THIS->DoRender (ioData);       // normal case
    else
        result = THIS->DoSRCRender (ioData);    // pre 2.0 mixer case

#if USE_AU_TRACER
	syscall(180, tracerEnd, inBusNumber, ioData->mNumberBuffers, 0, 0);
#endif

	return (result);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus OALSource::DoPreRender ()
{
        
    if (mCurrentBufferIndex > 0)
    {
    #if LOG_BUFFER_QUEUEING
        DebugMessageN2("OALSource::DoPreRender Qs being updated, OALSource:mCurrentBufferIndex %ld:%ld\n", mSelfToken, mCurrentBufferIndex);
    #endif
        // This means there were buffers that could not be moved to the inactive list before,
        // so try and move them now
        bool    gotTheLock;
        if (mBufferQueueMutex.Try(gotTheLock))
        {
            for (UInt32 i = 0; i < mCurrentBufferIndex; i++)
            {
                // Get buffer #i from Active List
                BufferInfo	*bufferInfo = mBufferQueueActive->Get(0);
                // Append it to Inactive List
                mBufferQueueInactive->AppendBuffer(bufferInfo->mBufferToken, bufferInfo->mBuffer, bufferInfo->mACToken);
                // Remove it from Active List
                mBufferQueueActive->RemoveQueueEntryByIndex(0);
            }
            mCurrentBufferIndex = 0;
            if (gotTheLock) 
                mBufferQueueMutex.Unlock();
        }
    }    
    
	BufferInfo	*bufferInfo = mBufferQueueActive->Get(mCurrentBufferIndex);
	if (bufferInfo == NULL)
    {
        mQueueIsProcessed = true;
        PlayFinished();
        mState = AL_STOPPED;
        // stop rendering, there is no more data
        return -1;	// there are no buffers
    }
        
	mRenderThreadID = (int) pthread_self();
		
	// there is a need to accomodate the spec which allows a 'NONE' buffer to be queued. In our case, just move on to the
	// next buffer in the queue until a non-NONE buffer is found
	while (bufferInfo->mBufferToken == AL_NONE)
	{
		mCurrentBufferIndex++;
		bufferInfo->mProcessedState = kProcessed;

		bufferInfo = mBufferQueueActive->Get(mCurrentBufferIndex);
		if (bufferInfo == NULL)
		{
            mQueueIsProcessed = true;
            PlayFinished();
            mState = AL_STOPPED;
        	return -1; // there are no more real buffers
        }
	}

	ChangeChannelSettings(true);

    if (mShouldFade) 
		return MuteCurrentPlayBus();

	return (noErr);
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
	
	sourcePacketsLeft = (bufferInfo->mBuffer->mDataSize - bufferInfo->mOffset) / bufferInfo->mBuffer->mDataFormat.mBytesPerPacket;

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// BUFFER EMPTY: If the buffer is empty now, decide on returning an error based on what gets played next in the queue
	if (sourcePacketsLeft == 0)
	{		
		bufferInfo->mProcessedState = kProcessed;	// this buffer is done
		bufferInfo->mOffset = 0;					// will be ready for the next time

		bool    gotTheLock;
		// Try and get the mutex so this buffer can be moved to the inactive list
		if (THIS->mBufferQueueMutex.Try(gotTheLock))
		{
			// Get buffer #i from Active List
			BufferInfo	*staleBufferInfo = THIS->mBufferQueueActive->Get(THIS->mCurrentBufferIndex);
			staleBufferInfo->mOffset = 0;
			THIS->mBufferQueueActive->SetBufferAsProcessed(THIS->mCurrentBufferIndex);
			// Append it to Inactive List
			THIS->mBufferQueueInactive->AppendBuffer(staleBufferInfo->mBufferToken, staleBufferInfo->mBuffer, staleBufferInfo->mACToken);
			// Remove it from Active List
			THIS->mBufferQueueActive->RemoveQueueEntryByIndex(THIS->mCurrentBufferIndex);
			if (gotTheLock)
				THIS->mBufferQueueMutex.Unlock();
		}
		else
			THIS->mCurrentBufferIndex++;

		// see if there is a next buffer or if the queue is looping and should return to the start
		BufferInfo	*nextBufferInfo = THIS->mBufferQueueActive->Get(THIS->mCurrentBufferIndex);
		if ((nextBufferInfo != NULL) || (THIS->mLooping == true))
		{
			// either we will loop back to the beginning or will use a new buffer
			if (nextBufferInfo == NULL)
			{
				THIS->LoopToBeginning();
				// see if the 1st buffer in the queue is the same format, return an error if so, so we get called again
				nextBufferInfo = THIS->mBufferQueueActive->Get(0);
			}
			
			if (nextBufferInfo->mBuffer->mDataFormat.mFormatID == bufferInfo->mBuffer->mDataFormat.mFormatID)
				err = OALSourceError_CallConverterAgain;

			if (nextBufferInfo->mBuffer->mDataFormat.mSampleRate != bufferInfo->mBuffer->mDataFormat.mSampleRate)
  				THIS->mResetBusFormat = true;
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
		
		ioData->mBuffers[0].mData = bufferInfo->mBuffer->mData + bufferInfo->mOffset;	// point to the data we are providing		
		ioData->mBuffers[0].mDataByteSize = *ioNumberDataPackets * bufferInfo->mBuffer->mDataFormat.mBytesPerPacket;
		bufferInfo->mOffset += ioData->mBuffers[0].mDataByteSize;
	}

	return (err);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus OALSource::DoRender (AudioBufferList 			*ioData)
{
	OSStatus   			err = noErr;
	UInt32				packetsRequestedFromRenderProc = ioData->mBuffers[0].mDataByteSize / sizeof(Float32);
	UInt32				packetsObtained = 0;
	UInt32				packetCount;
	AudioBufferList		*tempBufferList = ioData;
	UInt32				dataByteSize = ioData->mBuffers[0].mDataByteSize;
	void				*dataStarts[2];

	// save the data ptrs to restore later
	for (UInt32	i = 0; i < tempBufferList->mNumberBuffers; i++)
	{
		dataStarts[i] = tempBufferList->mBuffers[i].mData;
	}
		
	// walk through as many buffers as needed to satisfy the request AudioConverterFillComplexBuffer will
	// get called each time the data format changes until enough packets have been obtained or the q is empty.
	while ((packetsObtained < packetsRequestedFromRenderProc) && (mQueueIsProcessed == false))
	{
		BufferInfo	*bufferInfo = mBufferQueueActive->Get(mCurrentBufferIndex);
		if (bufferInfo == NULL)
		{
            // just zero out the remainder of the buffer
            mQueueIsProcessed = true;
            goto Finished;
        }
        
		bufferInfo->mProcessedState = kInProgress;

		for (UInt32	i = 0; i < tempBufferList->mNumberBuffers; i++)
		{
			tempBufferList->mBuffers[i].mDataByteSize = dataByteSize - (packetsObtained * sizeof(Float32));
			tempBufferList->mBuffers[i].mData = (Byte *) ioData->mBuffers[i].mData + (packetsObtained * sizeof(Float32));
		}		
		
		if (bufferInfo->mBuffer->mDataHasBeenConverted == false)
		{
			// CONVERT THE BUFFER DATA 
			AudioConverterRef	converter = mACMap->Get(bufferInfo->mACToken);
	
			packetCount = packetsRequestedFromRenderProc - packetsObtained;
			// if OALSourceError_CallConverterAgain is returned, there is nothing to do, just go around again and try and get the data
			err = AudioConverterFillComplexBuffer(converter, ACComplexInputDataProc, this, &packetCount, tempBufferList, NULL);
			packetsObtained += packetCount;
	
			if ((packetsObtained < packetsRequestedFromRenderProc) && (err == noErr))
			{
				// we didn't get back what we asked for, but no error implies we have used up the data of this format
				// so reset this converter so it will be ready for the next time
				AudioConverterReset(converter);
#if	LOG_VERBOSE
                DebugMessageN1("OALSource::DoRender: Resetting AudioConverter - OALSource = %ld\n", mSelfToken);
#endif
            }
        }
		else
		{
			// Data has already been converted to the mixer's format, so just do a copy (should be mono only)
			UInt32	bytesRemaining = bufferInfo->mBuffer->mDataSize - bufferInfo->mOffset;
			UInt32	framesRemaining = bytesRemaining / sizeof(Float32);
			UInt32	bytesToCopy = 0;
			UInt32	framesToCopy = packetsRequestedFromRenderProc - packetsObtained;
            
			if (framesRemaining < framesToCopy)
				framesToCopy = framesRemaining;
			
			bytesToCopy = framesToCopy * sizeof(Float32);
			memcpy(tempBufferList->mBuffers->mData, bufferInfo->mBuffer->mData + bufferInfo->mOffset, bytesToCopy);
			bufferInfo->mOffset += bytesToCopy;
			packetsObtained += framesToCopy;
						
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			// this block of code is the same as that found in the fill proc - 
			// it is for determining what to do when a buffer runs out of data

			if (bufferInfo->mOffset == bufferInfo->mBuffer->mDataSize)
			{

                bool    gotTheLock;
                // Try and get the mutex so this buffer can be moved to the inactive list
                if (mBufferQueueMutex.Try(gotTheLock))
                {
                    // Get buffer #i from Active List
                    BufferInfo	*staleBufferInfo = mBufferQueueActive->Get(mCurrentBufferIndex);
                    staleBufferInfo->mOffset = 0;
                    mBufferQueueActive->SetBufferAsProcessed(mCurrentBufferIndex);
                    // Append it to Inactive List
                    mBufferQueueInactive->AppendBuffer(staleBufferInfo->mBufferToken, staleBufferInfo->mBuffer, staleBufferInfo->mACToken);
                    // Remove it from Active List
                    mBufferQueueActive->RemoveQueueEntryByIndex(mCurrentBufferIndex);
                    if (gotTheLock)
                        mBufferQueueMutex.Unlock();
                }
                else
                    mCurrentBufferIndex++;

				// see if there is a next buffer or if the queue is looping and should return to the start
				BufferInfo	*nextBufferInfo = mBufferQueueActive->Get(mCurrentBufferIndex);
				if ((nextBufferInfo != NULL) || (mLooping == true))
				{
					if (mLooping == true)
                    {
						LoopToBeginning();
						nextBufferInfo = mBufferQueueActive->Get(mCurrentBufferIndex);
					}
					
					// if the format of the next buffer is the same, return an error so we get called again,
					// else, return what we have and no error, the AC should then be reset in the RenderProc
					if (nextBufferInfo->mBuffer->mDataFormat.mSampleRate != bufferInfo->mBuffer->mDataFormat.mSampleRate)
					{
	                    // if the next buffer is a different sample rate, the bus format will need to be changed
						mResetBusFormat = true;
					}
				}
				else
				{
					// looping is false and there are no more buffers so we are really out of data
					// return what we have and no error
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
	// if there wasn't enough data left, be sure to silence the end of the buffer
	if (packetsObtained < packetsRequestedFromRenderProc)
	{
		for (UInt32	i = 0; i < tempBufferList->mNumberBuffers; i++)
		{
			tempBufferList->mBuffers[i].mDataByteSize = dataByteSize - (packetsObtained * sizeof(Float32));
			tempBufferList->mBuffers[i].mData = (Byte *) ioData->mBuffers[i].mData + (packetsObtained * sizeof(Float32));
			memset(tempBufferList->mBuffers[i].mData, 0, tempBufferList->mBuffers[i].mDataByteSize);
		}		
	}
    
	for (UInt32	i = 0; i < tempBufferList->mNumberBuffers; i++)
	{
		tempBufferList->mBuffers[i].mData = dataStarts[i];
		tempBufferList->mBuffers[i].mDataByteSize = dataByteSize;
	}

	return (noErr);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus OALSource::DoPostRender ()
{
    if (mCurrentBufferIndex > 0)
    {
    #if LOG_BUFFER_QUEUEING
        DebugMessageN2("OALSource::DoPostRender, Qs being updated, OALSource:mCurrentBufferIndex %ld:%ld\n", mSelfToken, mCurrentBufferIndex);
    #endif
        // This means there were buffers that could not be moved to the inactive list before,
        // so try and move them now
        bool    gotTheLock;
        if (mBufferQueueMutex.Try(gotTheLock))
        {
            for (UInt32 i = 0; i < mCurrentBufferIndex; i++)
            {
                // Get buffer #i from Active List
                BufferInfo	*bufferInfo = mBufferQueueActive->Get(0);
                // Append it to Inactive List
                mBufferQueueInactive->AppendBuffer(bufferInfo->mBufferToken, bufferInfo->mBuffer, bufferInfo->mACToken);
                // Remove it from Active List
                mBufferQueueActive->RemoveQueueEntryByIndex(0);
            }
            mCurrentBufferIndex = 0;
            if (gotTheLock)
                mBufferQueueMutex.Unlock();
        }
    }    

	if ((mQueueIsProcessed == false) && (mShouldFade == false))
		return (noErr);
	
#if USE_MUTEXES
	TRY_PLAY_MUTEX
#endif

	if (mShouldFade) 
    {
		UpdateBusGain (true);  // it was turned down, so reset it now
        mShouldFade = false;
	}
	
	if (mQueueIsProcessed == true)
	{
		PlayFinished();
		mState = AL_STOPPED;
	}
	
#if USE_MUTEXES
	UNLOCK_PLAY_MUTEX
#endif	
	return noErr;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CalculateDistanceAndAzimuth() support
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
void OALSource::CalculateDistanceAndAzimuth(Float32 *outDistance, Float32 *outAzimuth, Float32	*outDopplerShift, bool isRenderThread)
{

    Float32 	ListenerOrientation[6],
                ListenerPosition[3],
                ListenerVelocity[3],
                Angle = 0.0,
                Distance = 2.0,
                Distance_squared = 4.0,
                front_back,
                SourceToListener[3],
                Look_Norm[3],
                U[3],
                tPosition[3],
                dopplerShift = 1.0;     // default at No shift
                
    *outDopplerShift = dopplerShift;    // initialize
        
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
        return;	// there are no buffers
	}
    
	// Only apply 3D calculations for mono buffers
	if (bufferInfo->mBuffer->mDataFormat.NumberChannels() == 1)
	{
		//1. Translate Listener to origin (convert to head relative)
		if (mSourceRelative == AL_FALSE)
		{
			tPosition[0] -= ListenerPosition[0];
			tPosition[1] -= ListenerPosition[1];
			tPosition[2] -= ListenerPosition[2];
		}
        //2. Align coordinate system axes
        aluCrossproduct(&ListenerOrientation[0],&ListenerOrientation[3],U); // Right-ear-vector
        aluNormalize(U); // Normalized Right-ear-vector
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
            aluNormalize(SourceToListener);
            
            Angle = 180.0 * acos (aluDotproduct(SourceToListener,U))/3.141592654f;
            
            //is the source infront of the listener or behind?
            front_back = aluDotproduct(SourceToListener,Look_Norm);
            if(front_back<0.0f)
                Angle = 360.0f - Angle;
                
            //translate from cartesian angle to 3d mixer angle
            if((Angle>=0.0f)&&(Angle<=270.0f)) Angle = 90.0f - Angle;
            else Angle = 450.0f - Angle;
        }
        
        //5. Calculate doppler
        Float32		DopplerFactor = mOwningContext->GetDopplerFactor();
        if (DopplerFactor > 0.0)
        {
            Float32     SourceVelocity[3];

            GetVelocity (SourceVelocity[0], SourceVelocity[1], SourceVelocity[2], isRenderThread);

            Float32 ProjectedSourceVelocity = aluDotproduct(SourceVelocity, SourceToListener);
            Float32 ProjectedListenerVelocity = aluDotproduct(ListenerVelocity, SourceToListener);
            if (fabs(DopplerFactor) < 1.0e-6)
            {
                ProjectedSourceVelocity *= DopplerFactor;
                ProjectedListenerVelocity *= DopplerFactor;
            }
            
            #if LOG_DOPPLER
                DebugMessageN2("CalculateDistanceAndAzimuth: ListenerVelocity/SourceVelocity %f2/%f2", ProjectedListenerVelocity, ProjectedSourceVelocity);
            #endif
            
            if (fabsf(ProjectedSourceVelocity) > mOwningContext->GetDopplerVelocity())
            {
                ProjectedSourceVelocity = ProjectedSourceVelocity > 0.0 ? mOwningContext->GetDopplerVelocity() : -mOwningContext->GetDopplerVelocity();
            #if LOG_DOPPLER
                DebugMessageN1("CalculateDistanceAndAzimuth: Pinning ProjectedSourceVelocity at =  %f2", -mOwningContext->GetDopplerVelocity());
            #endif
            }
            
            if (fabsf(ProjectedListenerVelocity) > mOwningContext->GetDopplerVelocity())
            {
                ProjectedListenerVelocity = ProjectedListenerVelocity > 0.0 ? mOwningContext->GetDopplerVelocity() : -mOwningContext->GetDopplerVelocity();
            #if LOG_DOPPLER
                DebugMessageN1("CalculateDistanceAndAzimuth: Pinning ProjectedListenerVelocity at =  %f2", mOwningContext->GetDopplerVelocity());
            #endif
            }

            dopplerShift = ((mOwningContext->GetDopplerVelocity() - ProjectedListenerVelocity)/(mOwningContext->GetDopplerVelocity() + ProjectedSourceVelocity));
                                      
            // limit the pitch shifting to 3 octave up or down
            if (dopplerShift > 1.0)
            {
                dopplerShift *= DopplerFactor;
                if (dopplerShift > 8.0)
                    dopplerShift = 8.0;
            }
            else 
            {
                dopplerShift /= DopplerFactor;
                if(dopplerShift < 0.125)
                    dopplerShift = 0.125;   
            }
            
            #if LOG_DOPPLER
				DebugMessageN1("CalculateDistanceAndAzimuth: dopplerShift after scaling =  %f2\n", dopplerShift);
            #endif
            
			*outDopplerShift = dopplerShift;
        }
    }
    else
    {
        Angle=0.0;
        Distance=0.0;
    }
        
    if (!mOwningDevice->IsPreferredMixerAvailable() && (mReferenceDistance > 1.0))
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
    BufferInfo	*bufferInfo = mBufferQueueActive->Get(mCurrentBufferIndex);
    if (bufferInfo == NULL)
        return -1;
		
	double srcSampleRate = bufferInfo->mBuffer->mDataFormat.mSampleRate;
	double dstSampleRate = mOwningDevice->GetMixerRate();
	double ratio = (srcSampleRate / dstSampleRate) * mPitch * mDopplerScaler;
	
	int nchannels = ioData->mNumberBuffers;

	if (ratio == 1.0)
	{
		// no SRC necessary so just call the normal render proc and let it fill out the buffers
        return (DoRender(ioData));
 	}

	// otherwise continue on to do dirty linear interpolation
	UInt32      inFramesToProcess = ioData->mBuffers[0].mDataByteSize / sizeof(Float32);
	float readIndex = mReadIndex;

	int readUntilIndex = (int) (2.0 + readIndex + inFramesToProcess * ratio );
	int framesToPull = readUntilIndex - 2;
	
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
    
    OSStatus result = DoRender(mTempSourceStorage);
	if (result != noErr ) 
        return result;		// !!@ something bad happened (could return error code)

	float *pullL = (float *) mTempSourceStorage->mBuffers[0].mData;
	float *pullR = nchannels > 1 ? (float *) mTempSourceStorage->mBuffers[1].mData: NULL;

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
	float *sourceL = tempL;
	float *sourceR = tempR;
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
	int n = inFramesToProcess;
	
	float *destL = (float *) ioData->mBuffers[0].mData;
	float *destR = (float *) ioData->mBuffers[1].mData;
	
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