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

#include "oalContext.h"
#include "oalSource.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// OALContexts
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma mark ***** OALContexts - Public Methods *****
OALContext::OALContext (const uintptr_t	inSelfToken, OALDevice    *inOALDevice, UInt32	inBusCount)
	: 	mSelfToken (inSelfToken),
		mProcessingActive(true),
		mOwningDevice(inOALDevice),
		mMixerNode(0), 
		mMixerUnit (0),
		mSourceMap (NULL),
		mDistanceModel(AL_INVERSE_DISTANCE_CLAMPED),			
		mDopplerFactor(1.0),
		mDopplerVelocity(1.0),
		mListenerGain(1.0),
		mAttributeListSize(0),
		mAttributeList(NULL),
		mDistanceScalingRequired(false),
		mCalculateDistance(true),
		mStoredInverseAttenuation(1.0),
		mRenderQuality(ALC_SPATIAL_RENDERING_QUALITY_LOW),
		mSpatialSetting(0),
		mBusCount(inBusCount),
		mMixerOutputRate(kDefaultMixerRate),
		mDefaultReferenceDistance(1.0),
        mDefaultMaxDistance(100000.0),
		mUserSpecifiedBusCounts(false),
		mReverbState(0)
#if LOG_BUS_CONNECTIONS
		, mMonoSourcesConnected(0),
		mStereoSourcesConnected(0)
#endif
{
		mBusInfo = (BusInfo *) calloc (1, sizeof(BusInfo) * mBusCount);

		UInt32		stereoSources = 1; // default
		
		// initialize  mContextInfo
		mListenerPosition[0] = 0.0;
		mListenerPosition[1] = 0.0;
		mListenerPosition[2] = 0.0;
		
		mListenerVelocity[0] = 0.0;
		mListenerVelocity[1] = 0.0;
		mListenerVelocity[2] = 0.0;
		
		mListenerOrientationForward[0] = 0.0;
		mListenerOrientationForward[1] = 0.0;
		mListenerOrientationForward[2] = -1.0;
		
		mListenerOrientationUp[0] = 0.0;
		mListenerOrientationUp[1] = 1.0;
		mListenerOrientationUp[2] = 0.0;

		InitializeMixer(stereoSources);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OALContext::~OALContext()
{
	// delete all the sources that were created by this context
	if (mSourceMap)
	{
		for (UInt32  i = 0; i < mSourceMap->Size(); i++)
		{
			OALSource	*oalSource = mSourceMap->GetSourceByIndex(0);
			mSourceMap->Remove(oalSource->GetToken());
			if (oalSource != NULL)
				delete oalSource;
		}
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/*
	3DMixer Version Info
	
	- Pre 2.0 Mixer must be at least version 1.3 for OpenAL
	- 3DMixer 2.0
		Bug in Distance Attenuationg/Reverb code requires that distances be scaled in OAL before passing to mixer
	- 3DMixer 2.1
		Fixes bug in 2.0 but also has a bug related to OAL fixes for correctly caclulating the vector of moving object
*/

void		OALContext::InitializeMixer(UInt32	inStereoBusCount)
{
    OSStatus result = noErr;

	try {
		// ~~~~~~~~~~~~~~~~~~~ GET 3DMIXER VERSION
		ComponentDescription	mixerCD;
		mixerCD.componentFlags = 0;        
		mixerCD.componentFlagsMask = 0;     
		mixerCD.componentType = kAudioUnitType_Mixer;          
		mixerCD.componentSubType = kAudioUnitSubType_3DMixer;       
		mixerCD.componentManufacturer = kAudioUnitManufacturer_Apple;  

		ComponentInstance   mixerInstance = OpenComponent(FindNextComponent(0, &mixerCD));
		long  version = CallComponentVersion(mixerInstance);
		CloseComponent(mixerInstance);
		
		if (version < kMinimumMixerVersion)
			throw -1;                           // we do not have a current enough 3DMixer to use, so set an error and throw
		else if (version < 0x20000)
		{
			SetPreferred3DMixerPresent(false);
		}
		else
		{
			// if the Distance model is changed from the default (Inverse), we will need this value to reset it on the 2.0 and 2.1 3DMixers upon a call to SetDistanceModel()
			UInt32	size = sizeof(mStoredInverseAttenuation);
			AudioUnitGetProperty(mMixerUnit, kAudioUnitProperty_3DMixerDistanceAtten, kAudioUnitScope_Input, 1, &mStoredInverseAttenuation, &size);

			SetPreferred3DMixerPresent(true);

			// it's at least version 2.0, that's good
			if (version == 0x20000)
				mDistanceScalingRequired = true; 
		}
		
		// CREATE NEW NODE FOR THE GRAPH
		result = AUGraphNewNode (mOwningDevice->GetGraph(), &mixerCD, 0, NULL, &mMixerNode);
			THROW_RESULT

		result = AUGraphGetNodeInfo (mOwningDevice->GetGraph(), mMixerNode, 0, 0, 0, &mMixerUnit);
			THROW_RESULT   

		// Get Default Distance Setting when the good 3DMixer is around
		if (IsPreferred3DMixerPresent())
		{
			MixerDistanceParams		distanceParams;
			UInt32                  outSize;
			result = AudioUnitGetProperty(mMixerUnit, kAudioUnitProperty_3DMixerDistanceParams, kAudioUnitScope_Input, 1, &distanceParams, &outSize);
			if (result == noErr)
			{
				mDefaultReferenceDistance = distanceParams.mReferenceDistance;
				mDefaultMaxDistance = distanceParams.mMaxDistance;
			}
		}
		
		// REVERB off by default
		result = AudioUnitSetProperty(mMixerUnit, kAudioUnitProperty_UsesInternalReverb, kAudioUnitScope_Global, 0, &mReverbState, sizeof(mReverbState));
		// ignore result

		// MIXER BUS COUNT
		UInt32	outSize;
		if (!IsPreferred3DMixerPresent())
		{
			mBusCount = kDefaultMaximumMixerBusCount; // 1.3 version of the mixer did not allow a change in the bus count
		}
		else
		{
			// set the bus count on the mixer if necessary	
			UInt32  currentBusCount;
			outSize = sizeof(currentBusCount);
			result = AudioUnitGetProperty (	mMixerUnit, kAudioUnitProperty_BusCount, kAudioUnitScope_Input, 0, &currentBusCount, &outSize);
			if ((result == noErr) && (mBusCount != currentBusCount))
			{
				result = AudioUnitSetProperty (	mMixerUnit, kAudioUnitProperty_BusCount, kAudioUnitScope_Input, 0, &mBusCount, outSize);
				if (result != noErr)
				{
					// couldn't set the bus count so make sure we know just how many busses there are
					outSize = sizeof(mBusCount);
					AudioUnitGetProperty (	mMixerUnit, kAudioUnitProperty_BusCount, kAudioUnitScope_Input, 0, &mBusCount, &outSize);
				}
			}
		}

		// SET UP STEREO/MONO BUSSES
		CAStreamBasicDescription 	theOutFormat;
		theOutFormat.mSampleRate = mMixerOutputRate;
		theOutFormat.mFormatID = kAudioFormatLinearPCM;
		theOutFormat.mFramesPerPacket = 1;	
		theOutFormat.mBytesPerFrame = sizeof (Float32);
		theOutFormat.mBitsPerChannel = sizeof (Float32) * 8;	
		theOutFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
		theOutFormat.mBytesPerPacket = sizeof (Float32);	

		for (UInt32	i = 0; i < mBusCount; i++)
		{
			// Distance Attenuation: for pre v2.0 mixer
			SetDistanceAttenuation (i, kDefaultReferenceDistance, kDefaultMaximumDistance, kDefaultRolloff);			

			theOutFormat.mChannelsPerFrame = (i < inStereoBusCount) ? 2 : 1;
			OSStatus	result = AudioUnitSetProperty (	mMixerUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 
															i, &theOutFormat, sizeof(CAStreamBasicDescription));
				THROW_RESULT

			mBusInfo[i].mNumberChannels = theOutFormat.mChannelsPerFrame; 
			mBusInfo[i].mIsAvailable = true;
			mBusInfo[i].mReverbState = mReverbState;

			// set kAudioUnitProperty_SpatializationAlgorithm
			UInt32		spatAlgo = (theOutFormat.mChannelsPerFrame == 2) ? kSpatializationAlgorithm_StereoPassThrough : mSpatialSetting;
			AudioUnitSetProperty(	mMixerUnit, kAudioUnitProperty_SpatializationAlgorithm, kAudioUnitScope_Input, i, &spatAlgo, sizeof(spatAlgo));

			// set kAudioUnitProperty_3DMixerRenderingFlags (distance attenuation) for mono busses
			if (theOutFormat.mChannelsPerFrame == 1)
			{
				UInt32 		render_flags_3d = k3DMixerRenderingFlags_DistanceAttenuation;
				if (mRenderQuality == ALC_SPATIAL_RENDERING_QUALITY_HIGH)
					render_flags_3d += k3DMixerRenderingFlags_InterAuralDelay; // off by default, on if the user sets High Quality rendering

				// Render Flags
				result = AudioUnitSetProperty(	mMixerUnit, kAudioUnitProperty_3DMixerRenderingFlags, kAudioUnitScope_Input, i, &render_flags_3d, sizeof(render_flags_3d));
			}
		}

		// Initialize Busses - attributes may affect this operation
		InitRenderQualityOnBusses(); 	
	}
	catch(OSStatus	result){
		throw result;
	}
	catch(...){
		throw -1;
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void		OALContext::AddSource(ALuint	inSourceToken)
{
	if (mSourceMap == NULL)
		mSourceMap = new OALSourceMap();

	try {
			OALSource	*newSource = new OALSource (inSourceToken, this);
	
			mSourceMap->Add(inSourceToken, &newSource);
	}
	catch (...) {
		throw;
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OALSource*		OALContext::GetSource(ALuint	inSourceToken)
{
	return (mSourceMap->Get(inSourceToken));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void		OALContext::RemoveSource(ALuint	inSourceToken)
{
	OALSource	*oalSource = mSourceMap->Get(inSourceToken);
	if (oalSource != NULL)
	{
		oalSource->Stop();
		mSourceMap->Remove(inSourceToken);
		delete(oalSource);	
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void		OALContext::ProcessContext()
{
	if (mProcessingActive == true)
		return; // NOP
	
	DeviceConnect();
	mProcessingActive = true;
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void		OALContext::SuspendContext()
{
	if (mProcessingActive == false)
		return; // NOP

	DeviceDisconnect();
	mProcessingActive = false;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void		OALContext::DeviceConnect()
{
	mOwningDevice->ConnectContext(this);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void		OALContext::DeviceDisconnect()
{
	mOwningDevice->DisconnectContext(this);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void		OALContext::SetDistanceModel(UInt32	inDistanceModel)
{
#if LOG_GRAPH_AND_MIXER_CHANGES
	DebugMessageN1("OALContext::SetDistanceModel model = 0x%X", inDistanceModel);
#endif
	
	if (mDistanceModel != inDistanceModel)
	{
		switch (inDistanceModel)
		{
			case AL_INVERSE_DISTANCE:
			case AL_INVERSE_DISTANCE_CLAMPED:
				mCalculateDistance = true;
				break;
								
			case AL_NONE:
				mCalculateDistance = false;	// turn off distance attenuation altogether
				break;
				
			default:
				break;
		}

		mDistanceModel = inDistanceModel;
		if (mSourceMap)
		{
             mSourceMap->MarkAllSourcesForRecalculation();
		}
	}
}	

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void		OALContext::SetDopplerFactor(Float32		inDopplerFactor)
{
	if (mDopplerFactor != inDopplerFactor)
	{
		mDopplerFactor = inDopplerFactor;
		if (mSourceMap)
			mSourceMap->MarkAllSourcesForRecalculation();
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void		OALContext::SetDopplerVelocity(Float32	inDopplerVelocity)
{
	if (mDopplerVelocity != inDopplerVelocity)
	{
		mDopplerVelocity = inDopplerVelocity;
		if (mSourceMap)
			mSourceMap->MarkAllSourcesForRecalculation();
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void		OALContext::SetListenerGain(Float32	inGain)
{
	if (mListenerGain != inGain)
	{
		mListenerGain = inGain;
		
		Float32	db = 20.0 * log10(inGain); 				// convert to db
		AudioUnitSetParameter (mMixerUnit, k3DMixerParam_Gain, kAudioUnitScope_Output, 0, db, 0);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UInt32		OALContext::GetSourceCount()
{
	if (mSourceMap)
		return (mSourceMap->Size());

	return (0);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void		OALContext::SetListenerPosition(Float32	posX, Float32	posY, Float32	posZ) 
{ 
	if (isnan(posX) || isnan(posY) || isnan(posZ))
		throw ((OSStatus) AL_INVALID_VALUE);                        

	if (	(mListenerPosition[0] == posX) 	&& 
			(mListenerPosition[1] == posY )	&& 
			(mListenerPosition[2] == posZ)		)
		return;
	
	mListenerPosition[0] = posX;
	mListenerPosition[1] = posY;
	mListenerPosition[2] = posZ;

	if (mSourceMap)
	{
#if LOG_GRAPH_AND_MIXER_CHANGES
	DebugMessageN4("OALContext::SetListenerPosition called - OALSource = %f:%f:%f/%ld\n", posX, posY, posZ, mSelfToken);
#endif
		// moving the listener effects the coordinate translation for ALL the sources
		mSourceMap->MarkAllSourcesForRecalculation();
	}	
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void		OALContext::SetListenerVelocity(Float32	posX, Float32	posY, Float32	posZ) 
{ 
	mListenerVelocity[0] = posX;
	mListenerVelocity[1] = posY;
	mListenerVelocity[2] = posZ;

	if (mSourceMap)
	{
#if LOG_GRAPH_AND_MIXER_CHANGES
	DebugMessage("OALContext::SetListenerVelocity: MarkAllSourcesForRecalculation called\n");
#endif
		// moving the listener effects the coordinate translation for ALL the sources
        mSourceMap->MarkAllSourcesForRecalculation();
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALContext::SetListenerOrientation( Float32	forwardX, 	Float32	forwardY,	Float32	forwardZ,
											Float32	 upX, 		Float32	 upY, 		Float32	 upZ)
{	
	if (isnan(forwardX) || isnan(forwardY) || isnan(forwardZ) || isnan(upX) || isnan(upY) || isnan(upZ))
		throw ((OSStatus) AL_INVALID_VALUE);                        

	if (	(mListenerOrientationForward[0] == forwardX) 	&& 
			(mListenerOrientationForward[1] == forwardY )	&& 
			(mListenerOrientationForward[2] == forwardZ) 	&&
			(mListenerOrientationUp[0] == upX) 				&& 
			(mListenerOrientationUp[1] == upY ) 			&& 
			(mListenerOrientationUp[2] == upZ)					)
		return;

	mListenerOrientationForward[0] = forwardX;
	mListenerOrientationForward[1] = forwardY;
	mListenerOrientationForward[2] = forwardZ;
	mListenerOrientationUp[0] = upX;
	mListenerOrientationUp[1] = upY;
	mListenerOrientationUp[2] = upZ;

	if (mSourceMap)
	{
#if LOG_GRAPH_AND_MIXER_CHANGES
	DebugMessage("OALContext::SetListenerOrientation: MarkAllSourcesForRecalculation called\n");
#endif
		// moving the listener effects the coordinate translation for ALL the sources
		mSourceMap->MarkAllSourcesForRecalculation();
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UInt32	OALContext::GetDesiredRenderChannels(UInt32	inDeviceChannels)
{
    UInt32	returnValue = inDeviceChannels;
	
	if ((!IsPreferred3DMixerPresent()) && (returnValue == 4))
    {
        // quad did not work properly before version 2.0 of the 3DMixer, so just render to stereo
        returnValue = 2;
    }
    else if (inDeviceChannels < 4)
    {
        // guard against the possibility of multi channel hw that has never been given a preferred channel layout
        // Or, that a 3 channel layout was returned (which is unsupported by the 3DMixer)
        returnValue = 2; 
    } 
    else if (inDeviceChannels > 5)
    {
        // 3DMixer currently does not render to more than 5 channels
        returnValue = 5;    
    }
	return returnValue;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void OALContext::InitRenderQualityOnBusses()
{
	UInt32		channelCount = mOwningDevice->GetDesiredRenderChannelCount();

	if (channelCount > 2)
	{
        // at this time, there is only one spatial quality being used for multi channel hw
        DebugMessage("********** InitRenderQualityOnBusses:kDefaultMultiChannelQuality ***********");
		mSpatialSetting = kDefaultMultiChannelQuality;
	}
	else if (mRenderQuality == ALC_SPATIAL_RENDERING_QUALITY_LOW)
	{
		// this is the default case for stereo
        DebugMessage("********** InitRenderQualityOnBusses:kDefaultLowQuality ***********");
		mSpatialSetting =  kDefaultLowQuality;
	}
	else
	{
		DebugMessage("********** InitRenderQualityOnBusses:kDefaultHighQuality ***********");
		mSpatialSetting = kDefaultHighQuality;
	}
	
	UInt32 		render_flags_3d = k3DMixerRenderingFlags_DistanceAttenuation;
	if (mRenderQuality == ALC_SPATIAL_RENDERING_QUALITY_HIGH)
	{
    	 // off by default, on if the user sets High Quality rendering, as HRTF requires InterAuralDelay to be on
         render_flags_3d += k3DMixerRenderingFlags_InterAuralDelay;     
	}
    
    if (mReverbState > 0)
	{
    	 // off by default, on if the user turns on Reverb, as it requires DistanceDiffusion to be on
    	render_flags_3d += k3DMixerRenderingFlags_DistanceDiffusion;
		render_flags_3d += (1L << 6 /* k3DMixerRenderingFlags_ConstantReverbBlend*/);    
	}
    
	OSStatus                    result = noErr;
    UInt32                      outSize;
    CAStreamBasicDescription	format;
	for (UInt32	i = 0; i < mBusCount; i++)
	{
		outSize = sizeof(format);
		result = AudioUnitGetProperty (	mMixerUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, i, &format, &outSize);
		
		// only reset the mono channels, stereo channels are always set to stereo pass thru regardless of render quality setting
        if ((result == noErr) && (format.NumberChannels() == 1)) 
		{
			// Spatialization
			result = AudioUnitSetProperty(	mMixerUnit, kAudioUnitProperty_SpatializationAlgorithm, kAudioUnitScope_Input, 
											i, &mSpatialSetting, sizeof(mSpatialSetting));

			// Render Flags                
            result = AudioUnitSetProperty(	mMixerUnit, kAudioUnitProperty_3DMixerRenderingFlags, kAudioUnitScope_Input, 
											i, &render_flags_3d, sizeof(render_flags_3d));
			
			// Doppler - This must be done AFTER the spatialization setting, because some algorithms explicitly turn doppler on
			UInt32		doppler = kDopplerDefault;
			result = AudioUnitSetProperty(mMixerUnit, kAudioUnitProperty_DopplerShift, kAudioUnitScope_Input, i, &doppler, sizeof(doppler));
		}
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void OALContext::SetRenderQuality (UInt32 inRenderQuality)
{
	if (mRenderQuality == inRenderQuality)
		return;	// nothing to do;

	// make sure a valid quality setting is requested
	if (!IsValidRenderQuality(inRenderQuality))
		throw (OSStatus) AL_INVALID_VALUE;

	mRenderQuality = inRenderQuality;
			
	// change the spatialization for all mono busses on the mixer
	InitRenderQualityOnBusses(); 
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void    OALContext::SetDistanceAttenuation(UInt32    inBusIndex, Float64 inRefDist, Float64 inMaxDist, Float64 inRolloff)
{

	if (IsPreferred3DMixerPresent())
		return;     // unnecessary with v2.0 mixer
        
    Float64     maxattenuationDB = 20 * log10(inRefDist / (inRefDist + (inRolloff * (inMaxDist - inRefDist))));
    Float64     maxattenuation = pow(10, (maxattenuationDB/20));                    
    Float64     distAttenuation = (log(1/maxattenuation))/(log(inMaxDist)) - 1.0;

	#if 0
		DebugMessageN1("SetDistanceAttenuation:Reference Distance =  %f", inRefDist);
		DebugMessageN1("SetDistanceAttenuation:Maximum Distance =  %f", inMaxDist);
		DebugMessageN1("SetDistanceAttenuation:Rolloff =  %f", inRolloff);
		DebugMessageN1("SetDistanceAttenuation:Max Attenuation DB =  %f", maxattenuationDB);
		DebugMessageN1("SetDistanceAttenuation:Max Attenuation Scalar =  %f", maxattenuation);
		DebugMessageN1("SetDistanceAttenuation:distAttenuation =  %f", distAttenuation);

	#endif
    
    AudioUnitSetProperty(mMixerUnit, kAudioUnitProperty_3DMixerDistanceAtten, kAudioUnitScope_Input, inBusIndex, &distAttenuation, sizeof(distAttenuation));
    return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UInt32		OALContext::GetAvailableMonoBus ()
{
	// look for a bus already set for mono
	for (UInt32 i = 0; i < mBusCount; i++)
	{
		if (mBusInfo[i].mIsAvailable == true && mBusInfo[i].mNumberChannels == 1) 
		{
			mBusInfo[i].mIsAvailable = false;
#if LOG_BUS_CONNECTIONS
			mMonoSourcesConnected++;
			DebugMessageN2("GetAvailableMonoBus1: Sources Connected, Mono =  %ld, Stereo = %ld", mMonoSourcesConnected, mStereoSourcesConnected);
			DebugMessageN1("GetAvailableMonoBus1: BUS_NUMBER = %ld", i);
#endif
			return (i);
		}
	}

	// do not try and switch a bus to mono if the appliction specified mono and stereo bus counts
	if (!mUserSpecifiedBusCounts)
	{	
		// couldn't find a mono bus, so find any available channel and make it mono
		for (UInt32 i = 0; i < mBusCount; i++)
		{
			if (mBusInfo[i].mIsAvailable == true) 
			{
	#if LOG_BUS_CONNECTIONS
				mMonoSourcesConnected++;
				DebugMessageN2("GetAvailableMonoBus2: Sources Connected, Mono =  %ld, Stereo = %ld", mMonoSourcesConnected, mStereoSourcesConnected);
	#endif
				CAStreamBasicDescription 	theOutFormat;
				theOutFormat.mChannelsPerFrame = 1;
				theOutFormat.mSampleRate = GetMixerRate();          // as a default, set the bus to the mixer's output rate, it should get reset if necessary later on
				theOutFormat.mFormatID = kAudioFormatLinearPCM;
				theOutFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
				theOutFormat.mBytesPerPacket = sizeof (Float32);	
				theOutFormat.mFramesPerPacket = 1;	
				theOutFormat.mBytesPerFrame = sizeof (Float32);
				theOutFormat.mBitsPerChannel = sizeof (Float32) * 8;	
				OSStatus	result = AudioUnitSetProperty (	mMixerUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 
															i, &theOutFormat, sizeof(CAStreamBasicDescription));
					THROW_RESULT

				mBusInfo[i].mIsAvailable = false;
				mBusInfo[i].mNumberChannels = 1; 			
				AudioUnitSetProperty(	mMixerUnit, kAudioUnitProperty_SpatializationAlgorithm, kAudioUnitScope_Input, 
										i, &mSpatialSetting, sizeof(mSpatialSetting));

				UInt32 		render_flags_3d = k3DMixerRenderingFlags_DistanceAttenuation;
				if (mRenderQuality == ALC_SPATIAL_RENDERING_QUALITY_HIGH)
					render_flags_3d += k3DMixerRenderingFlags_InterAuralDelay; // off by default, on if the user sets High Quality rendering

				// Render Flags
				result = AudioUnitSetProperty(	mMixerUnit, kAudioUnitProperty_3DMixerRenderingFlags, kAudioUnitScope_Input, 
												i, &render_flags_3d, sizeof(render_flags_3d));
				
				return (i);
			}
		}
	}
	
	DebugMessage("ERROR: GetAvailableMonoBus: COULD NOT GET A MONO BUS");
	throw (-1); // no inputs available
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UInt32		OALContext::GetAvailableStereoBus ()
{
	for (UInt32 i = 0; i < mBusCount; i++)
	{
		if (mBusInfo[i].mIsAvailable == true && mBusInfo[i].mNumberChannels == 2) 
		{
			mBusInfo[i].mIsAvailable = false;
#if LOG_BUS_CONNECTIONS
			mStereoSourcesConnected++;
			DebugMessageN2("GetAvailableStereoBus1: Sources Connected, Mono =  %ld, Stereo = %ld", mMonoSourcesConnected, mStereoSourcesConnected);
			DebugMessageN1("GetAvailableStereoBus1: BUS_NUMBER = %ld", i);
#endif
			return (i);
		}
	}

	// do not try and switch a bus to stereo if the appliction specified mono and stereo bus counts
	if (!mUserSpecifiedBusCounts)
	{
		// couldn't find one, so look for a mono channel, make it stereo and set to kSpatializationAlgorithm_StereoPassThrough
		for (UInt32 i = 0; i < mBusCount; i++)
		{
			if (mBusInfo[i].mIsAvailable == true) 
			{

	#if LOG_BUS_CONNECTIONS
				mStereoSourcesConnected++;
				DebugMessageN2("GetAvailableStereoBus2: Sources Connected, Mono =  %ld, Stereo = %ld", mMonoSourcesConnected, mStereoSourcesConnected);
				DebugMessageN1("GetAvailableStereoBus2: BUS_NUMBER = %ld", i);
	#endif
				CAStreamBasicDescription 	theOutFormat;
				theOutFormat.mChannelsPerFrame = 2;
				theOutFormat.mSampleRate = GetMixerRate();
				theOutFormat.mFormatID = kAudioFormatLinearPCM;
				theOutFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
				theOutFormat.mBytesPerPacket = sizeof (Float32);	
				theOutFormat.mFramesPerPacket = 1;	
				theOutFormat.mBytesPerFrame = sizeof (Float32);
				theOutFormat.mBitsPerChannel = sizeof (Float32) * 8;	
				OSStatus	result = AudioUnitSetProperty (	mMixerUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 
															i, &theOutFormat, sizeof(CAStreamBasicDescription));
					THROW_RESULT

				mBusInfo[i].mIsAvailable = false;
				mBusInfo[i].mNumberChannels = 2; 

				UInt32		spatAlgo = kSpatializationAlgorithm_StereoPassThrough;
				AudioUnitSetProperty(	mMixerUnit, kAudioUnitProperty_SpatializationAlgorithm, kAudioUnitScope_Input, 
										i, &spatAlgo, sizeof(spatAlgo));

				return (i);
			}
		}
	}
	
	DebugMessage("ERROR: GetAvailableStereoBus: COULD NOT GET A STEREO BUS");
	throw (-1); // no inputs available
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALContext::SetBusAsAvailable (UInt32 inBusIndex)
{	
	mBusInfo[inBusIndex].mIsAvailable = true;

#if LOG_BUS_CONNECTIONS
	if (mBusInfo[inBusIndex].mNumberChannels == 1)
		mMonoSourcesConnected--;
	else
		mStereoSourcesConnected--;

		DebugMessageN2("SetBusAsAvailable: Sources Connected, Mono =  %ld, Stereo = %ld", mMonoSourcesConnected, mStereoSourcesConnected);
#endif
}