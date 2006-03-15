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

#include "oalDevice.h"
#include "oalContext.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define LOG_DEVICE_CHANGES  	0
#define PROFILE_IO_USAGE		0

#if PROFILE_IO_USAGE
static int debugCounter         = -1;
static int numCyclesToPrint     = 1000;

static UInt64 lastHostTime;
static UInt64 totalHostTime;
static UInt64 minUsage;
static UInt64 maxUsage;
static UInt64 totalUsage;

#define PROFILE_IO_CYCLE 0
#if PROFILE_IO_CYCLE
static UInt64 maxHT;
static UInt64 minHT;
#endif

#include <CoreAudio/HostTime.h>
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#if GET_OVERLOAD_NOTIFICATIONS
OSStatus	PrintTheOverloadMessage(	AudioDeviceID			inDevice,
										UInt32					inChannel,
										Boolean					isInput,
										AudioDevicePropertyID	inPropertyID,
										void*					inClientData)
{
	DebugMessage("OVERLOAD OCCURRED");
}
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// OALDevices
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ***** OALDevices *****

/*
	If caller wants a specific HAL device (instead of the default output device), a NULL terminated 
	C-String representation of the CFStringRef returned from the HAL APIs for the 
	kAudioDevicePropertyDeviceUID property
*/
OALDevice::OALDevice (const char* 	 inDeviceName, uintptr_t   inSelfToken)
	: 	mSelfToken (inSelfToken),
		mCurrentError(ALC_NO_ERROR),
        mHALDevice (0),
        mDistanceScalingRequired(false),
        mAUGraph(0),
		mOutputNode(0), 
		mOutputUnit(0),
		mConnectedContext(NULL),
		mMixerOutputRate(kDefaultMixerRate),
		mRenderChannelCount(0),
        mRenderChannelSetting(ALC_RENDER_CHANNEL_COUNT_MULTICHANNEL),
		mFramesPerSlice(512)
{
	OSStatus	result = noErr;
	UInt32		size = 0;
	CFStringRef	cfString = NULL;
    char        *useThisDevice = (char *) inDeviceName;
	
	try {
        // until the ALC_ENUMERATION_EXT extension is supported only use the default output device
        useThisDevice = NULL;
        
		// first, get the requested HAL device's ID
		if (useThisDevice)
		{
			// turn the inDeviceName into a CFString
			cfString = CFStringCreateWithCString(NULL, useThisDevice, kCFStringEncodingUTF8);
			if (cfString)
			{
				AudioValueTranslation	translation;
				
				translation.mInputData = &cfString;
				translation.mInputDataSize = sizeof(cfString);
				translation.mOutputData = &mHALDevice;
				translation.mOutputDataSize = sizeof(mHALDevice);
				
				size = sizeof(AudioValueTranslation);
				result = AudioHardwareGetProperty(kAudioHardwarePropertyDeviceForUID, &size, &translation);
                CFRelease (cfString);
			}
			else
				result = -1; // couldn't get string ref
				
			THROW_RESULT
		}
		else
		{
			size = sizeof(AudioDeviceID);
			result = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &size, &mHALDevice);
				THROW_RESULT
		}
		
		InitializeGraph(useThisDevice);
		
#if PROFILE_IO_USAGE
		debugCounter = -1;
#endif

	}
	catch (...) {
        throw;
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OALDevice::~OALDevice()
{		
	TeardownGraph();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALDevice::SetError(ALenum errorCode)
{
	if (mCurrentError == ALC_NO_ERROR)
		return;
	
	mCurrentError = errorCode;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALenum	OALDevice::GetError()
{
	ALenum	latestError = mCurrentError;
	mCurrentError = ALC_NO_ERROR;
	
	return latestError;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void OALDevice::TeardownGraph()
{
#if LOG_DEVICE_CHANGES
	DebugMessageN1("OALDevice::TeardownGraph - OALDevice: %ld\n", mSelfToken);
#endif

#if GET_OVERLOAD_NOTIFICATIONS
	AudioDeviceID	device = 0;
	UInt32			size = sizeof(device);
	
	AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &size, &device);
	if (device != 0)
	{
		DebugMessage("********** Removing Overload Notification ***********");
		AudioDeviceRemovePropertyListener(	device, 0, false, kAudioDeviceProcessorOverload, PrintTheOverloadMessage);	
	}
#endif

	if (mAUGraph) 
	{
		AUGraphStop (mAUGraph);

		DisposeAUGraph (mAUGraph);
		mAUGraph = 0;
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALDevice::ResetChannelLayout()
{
#if LOG_DEVICE_CHANGES
	DebugMessageN1("OALDevice::ResetChannelLayout - OALDevice: %ld\n", mSelfToken);
#endif

	// verify that the channel count has actually changed before doing all this work...
	UInt32	channelCount = GetDesiredRenderChannelCount();

	if (mRenderChannelCount == channelCount)
		return; // only reset the graph if the channel count has changed

	mRenderChannelCount = channelCount;

	AUGraphStop (mAUGraph);
	Boolean flag;
	do {
		AUGraphIsRunning (mAUGraph, &flag);
	} while (flag);

	// disconnect the mixer from the  output au
	OSStatus	result = AUGraphDisconnectNodeInput (mAUGraph, mOutputNode, 0);
	// update the graph
	for (;;) {
		result = AUGraphUpdate (mAUGraph, NULL);
		if (result == noErr)
			break;
	}
	
	ConfigureGraphAUs();
	
	// connect mixer to output unit
	result = AUGraphConnectNodeInput (mAUGraph, mConnectedContext->GetMixerNode() /*mConnectedMixerNode*/, 0, mOutputNode, 0);
		THROW_RESULT
		
	// update the graph
	for (;;) {
		result = AUGraphUpdate (mAUGraph, NULL);
		if (result == noErr)
			break;
	}

	mConnectedContext->InitRenderQualityOnBusses ();

	AUGraphStart(mAUGraph);
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void OALDevice::GraphFormatPropertyListener (	void				*inRefCon, 
												AudioUnit			ci, 
												AudioUnitPropertyID inID, 
												AudioUnitScope		inScope, 
												AudioUnitElement	inElement)
{
#if LOG_DEVICE_CHANGES
	DebugMessageN1("OALDevice::GraphFormatPropertyListener - OALDevice: %ld\n", ((OALDevice*)inRefCon)->mSelfToken);
#endif

	try {
		if (inScope == kAudioUnitScope_Output)
		{
			((OALDevice*)inRefCon)->ResetChannelLayout ();
		}		
	} 
	catch (...) {
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void OALDevice::SetMixerRate (Float64	inSampleRate)
{
#if LOG_DEVICE_CHANGES
	DebugMessageN1("OALDevice::SetMixerRate - OALDevice: %ld\n", mSelfToken);
#endif
	
	if (mMixerOutputRate == inSampleRate)
		return;		// nothing to do	
		
	AUGraphDisconnectNodeInput (mAUGraph, mOutputNode, 0);
	OSStatus result;
	for (;;) 
    {
		result = AUGraphUpdate (mAUGraph, NULL);
		if (result == noErr)
			break;
	}

	AudioUnitUninitialize (mConnectedContext->GetMixerUnit());
	
	// reconfigure the graph's mixer...
	CAStreamBasicDescription outFormat;
	UInt32 outSize = sizeof(outFormat);
	result = AudioUnitGetProperty (mConnectedContext->GetMixerUnit(), kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &outFormat, &outSize);
		THROW_RESULT
		
	outFormat.mSampleRate = inSampleRate;
	result = AudioUnitSetProperty (mConnectedContext->GetMixerUnit(), kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &outFormat, sizeof(outFormat));
		THROW_RESULT
		
    // lets just do the output unit format
	result = AudioUnitSetProperty (mOutputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &outFormat, sizeof(outFormat));
		THROW_RESULT

	AudioUnitInitialize(mConnectedContext->GetMixerUnit());

	result = AUGraphConnectNodeInput (mAUGraph, mConnectedContext->GetMixerNode(), 0, mOutputNode, 0);
		THROW_RESULT

	for (;;) 
    {
		result = AUGraphUpdate (mAUGraph, NULL);
		if (result == noErr)
			break;
	}

	mMixerOutputRate = inSampleRate;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void OALDevice::SetRenderChannelCount (UInt32 inRenderChannelCount)
{
#if LOG_DEVICE_CHANGES
	DebugMessageN1("OALDevice::SetRenderChannelCount - OALDevice: %ld\n", mSelfToken);
#endif

    if ((inRenderChannelCount != ALC_RENDER_CHANNEL_COUNT_MULTICHANNEL) && (inRenderChannelCount != ALC_RENDER_CHANNEL_COUNT_STEREO))
		throw (OSStatus) AL_INVALID_VALUE;
    
    if (inRenderChannelCount == mRenderChannelSetting)
        return; //nothing to do
        
    mRenderChannelSetting = inRenderChannelCount;
    
    if (inRenderChannelCount == ALC_RENDER_CHANNEL_COUNT_STEREO)
    {
        // clamping to stereo
        if (mRenderChannelCount == 2)
            return; // already rendering to stereo, so there's nothing to do
    }
    else
    {
        // allowing multi channel now
        if (mRenderChannelCount > 2)
            return; // already rendering to mc, so there's nothing to do
    }    

    // work to be done now, it is necessary to change the channel layout and stream format from multi channel to stereo
    // this requires the graph to be stopped and reconfigured
    ResetChannelLayout ();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void OALDevice::InitializeGraph (const char* 		inDeviceName)
{	
#if LOG_DEVICE_CHANGES
	DebugMessageN1("OALDevice::InitializeGraph - OALDevice: %ld\n", mSelfToken);
#endif

	if (mAUGraph)
		throw static_cast<OSStatus>('init');
	
    OSStatus result = noErr;
    
    // ~~~~~~~~~~~~~~~~~~~~ CREATE GRAPH

	result = NewAUGraph(&mAUGraph);
		THROW_RESULT
	 
    // ~~~~~~~~~~~~~~~~~~~~ SET UP OUTPUT NODE

	ComponentDescription	cd;
	cd.componentFlags = 0;        
	cd.componentFlagsMask = 0;     

	// At this time, only allow the default output device to be used and ignore the inDeviceName parameter
	cd.componentType = kAudioUnitType_Output;          
	cd.componentSubType = kAudioUnitSubType_DefaultOutput;       	
	cd.componentManufacturer = kAudioUnitManufacturer_Apple;  
	result = AUGraphNewNode (mAUGraph, &cd, 0, NULL, &mOutputNode);
		THROW_RESULT
		        		
	// ~~~~~~~~~~~~~~~~~~~~ OPEN GRAPH
	
	result = AUGraphOpen (mAUGraph);
		THROW_RESULT
	
	result = AUGraphGetNodeInfo (mAUGraph, mOutputNode, 0, 0, 0, &mOutputUnit);
		THROW_RESULT   
	
 	result = AudioUnitInitialize (mOutputUnit);
		THROW_RESULT

	result = AudioUnitAddPropertyListener (mOutputUnit, kAudioUnitProperty_StreamFormat, GraphFormatPropertyListener, this);
		THROW_RESULT

	// Frame Per Slice
	// get the device's frame count and set the AUs to match, will be set to 512 if this fails
	AudioDeviceID	device = 0;
	UInt32	dataSize = sizeof(device);
	result = AudioUnitGetProperty(mOutputUnit, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, 0, &device, &dataSize);
	if (result == noErr)
	{
		dataSize = sizeof(mFramesPerSlice);
		result = AudioDeviceGetProperty(device, 0, false, kAudioDevicePropertyBufferFrameSize, &dataSize, &mFramesPerSlice);
		if (result == noErr)
		{
			result = AudioUnitSetProperty(  mOutputUnit, kAudioUnitProperty_MaximumFramesPerSlice, 
											kAudioUnitScope_Global, 0, &mFramesPerSlice, sizeof(mFramesPerSlice));
		}
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// set format channels. channel layout and frames per slice if necessary
void	OALDevice::ConfigureGraphAUs()
{
#if LOG_DEVICE_CHANGES
	DebugMessageN1("OALDevice::ConfigureGraphAUs - OALDevice: %ld\n", mSelfToken);
#endif

	OSStatus	result = noErr;

	try {
		// set the stream format
		CAStreamBasicDescription	format;
		UInt32                      outSize = sizeof(format);
		result = AudioUnitGetProperty(mOutputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &format, &outSize);
			THROW_RESULT

		if (mRenderChannelCount == 0)
		{
			if (mRenderChannelSetting == ALC_RENDER_CHANNEL_COUNT_STEREO)
				mRenderChannelCount = 2;
			else
				mRenderChannelCount = GetDesiredRenderChannelsFor3DMixer(format.NumberChannels());
		}

		format.SetCanonical (mRenderChannelCount, false);     // not interleaved
		format.mSampleRate = mMixerOutputRate;
		outSize = sizeof(format);
		result = AudioUnitSetProperty (mOutputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &format, outSize);
			THROW_RESULT

		result = AudioUnitSetProperty (mConnectedContext->GetMixerUnit(), kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &format, outSize);
			THROW_RESULT

		// explicitly set the channel layout, pre 2.0 mixer is just 5.0 and stereo
		// the output AU will then be configured correctly when the mixer is connected to it later
		AudioChannelLayout		layout;
		 
		if (mRenderChannelCount == 5)
			layout.mChannelLayoutTag = kAudioChannelLayoutTag_AudioUnit_5_0;
		else if (mRenderChannelCount == 4)
			layout.mChannelLayoutTag = (IsPreferred3DMixerPresent() == true) ? kAudioChannelLayoutTag_AudioUnit_4 : kAudioChannelLayoutTag_Stereo;
		else
			layout.mChannelLayoutTag = kAudioChannelLayoutTag_Stereo;
		
		layout.mChannelBitmap = 0;			
		layout.mNumberChannelDescriptions = 0;

		// it isn't currently necessary to explicitly set the mixer's output channel layout but it might in the future if there were more
		// than one layout usable by the current channel count. It doesn't hurt to set this property.
		outSize = sizeof(layout);
		result = AudioUnitSetProperty (mConnectedContext->GetMixerUnit(), kAudioUnitProperty_AudioChannelLayout, kAudioUnitScope_Output, 0, &layout, outSize);
			THROW_RESULT

		result = AudioUnitSetProperty (mOutputUnit, kAudioUnitProperty_AudioChannelLayout, kAudioUnitScope_Input, 0, &layout, outSize);
			THROW_RESULT

		
		// Frames Per Slice
		UInt32		mixerFPS = 0;
		UInt32		dataSize = sizeof(mixerFPS);
		result = AudioUnitGetProperty(mConnectedContext->GetMixerUnit(), kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &mixerFPS, &dataSize);
		if (mixerFPS != mFramesPerSlice)
		{
			result = AudioUnitSetProperty(  mConnectedContext->GetMixerUnit(), kAudioUnitProperty_MaximumFramesPerSlice, 
											kAudioUnitScope_Global, 0, &mFramesPerSlice, sizeof(mFramesPerSlice));
				THROW_RESULT
		}
	}
	catch(OSStatus	result) {
		throw result;
	}
	catch(...) {
		throw -1;
	}

	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UInt32 OALDevice::GetDesiredRenderChannelCount ()
{
#if LOG_DEVICE_CHANGES
	DebugMessageN1("OALDevice::GetDesiredRenderChannelCount - OALDevice: %ld\n", mSelfToken);
#endif

	UInt32			returnValue = 2;	// return stereo by default
	
    // observe the mRenderChannelSetting flag and clamp to stereo if necessary
    // This allows the user to request the libary to render to stereo in the case where only 2 speakers
    // are connected to multichannel hardware
    if (mRenderChannelSetting == ALC_RENDER_CHANNEL_COUNT_STEREO)
        return (returnValue);

	// get the HAL device id form the output AU
	AudioDeviceID	deviceID;
	UInt32			outSize =  sizeof(deviceID);
	OSStatus	result = AudioUnitGetProperty(mOutputUnit, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Output, 1, &deviceID, &outSize);
		THROW_RESULT
	
	// get the channel layout set by the user in AMS		
	result = AudioDeviceGetPropertyInfo(deviceID, 0, false, kAudioDevicePropertyPreferredChannelLayout, &outSize, NULL);
    if (result != noErr)
        return (returnValue);   // default to stereo since channel layout could not be obtained

	AudioChannelLayout	*layout = NULL;
	layout = (AudioChannelLayout *) calloc(1, outSize);
	if (layout != NULL)
	{
		result = AudioDeviceGetProperty(deviceID, 0, false, kAudioDevicePropertyPreferredChannelLayout, &outSize, layout);
		if (layout->mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelDescriptions)
		{
			// no channel layout tag is returned, so walk through the channel descriptions and count 
            // the channels that are associated with a speaker
			if (layout->mNumberChannelDescriptions == 2)
			{	
                returnValue = 2;        // there is no channel info for stereo
			}
			else
			{
				returnValue = 0;
				for (UInt32 i = 0; i < layout->mNumberChannelDescriptions; i++)
				{
					if (layout->mChannelDescriptions[i].mChannelLabel != kAudioChannelLabel_Unknown)
						returnValue++;
				}
			}
		}
		else
		{
			switch (layout->mChannelLayoutTag)
			{
				case kAudioChannelLayoutTag_AudioUnit_5_0:
				case kAudioChannelLayoutTag_AudioUnit_5_1:
				case kAudioChannelLayoutTag_AudioUnit_6:
					returnValue = 5;
					break;
				case kAudioChannelLayoutTag_AudioUnit_4:
					returnValue = 4;
					break;
				default:
					returnValue = 2;
					break;
			}
		}
	
		free(layout);
	}
    
	// pass in num channels on the hw, 
	// how many channels the user has requested, and which 3DMixer is present
	returnValue	= GetDesiredRenderChannelsFor3DMixer(returnValue);
        
	return (returnValue);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALDevice::ConnectContext (OALContext*	inContext)
{
#if LOG_DEVICE_CHANGES
	DebugMessageN1("OALDevice::ConnectContext - OALDevice: %ld\n", mSelfToken);
#endif

	OSStatus	result = noErr;
	OALContext*	oldContext = mConnectedContext;
	
	if (mConnectedContext == NULL)
		mConnectedContext = inContext;
	else if (inContext == mConnectedContext)
		return;	// already connected
			
	try {
			result = AUGraphUninitialize (mAUGraph);
				THROW_RESULT
	
			if (inContext->GetMixerNode() != mConnectedContext->GetMixerNode())
			{
				// disconnect the previously connected mixer now
				result = AUGraphDisconnectNodeInput(mAUGraph, mOutputNode, 0);
					THROW_RESULT

				mConnectedContext = NULL;
			}
						
			ConfigureGraphAUs();	// set AU format and channel layout and frames per slice if necessary
		
			// connect mixer to output unit
			OSStatus	result = AUGraphConnectNodeInput (mAUGraph, inContext->GetMixerNode(), 0, mOutputNode, 0);
				THROW_RESULT

			result = AUGraphInitialize (mAUGraph);
				THROW_RESULT

			// update the graph
			for (;;) {
				result = AUGraphUpdate (mAUGraph, NULL);
				if (result == noErr)
					break;
			}
				
			mConnectedContext = inContext;
			AUGraphStart(mAUGraph);
	}
	catch (OSStatus		result) {
		mConnectedContext = oldContext;
		throw result;
	}
	catch (...) {
		mConnectedContext = oldContext;
		throw -1;
	}
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	OALDevice::DisconnectContext (OALContext*	inContext)
{
#if LOG_DEVICE_CHANGES
	DebugMessageN1("OALDevice::DisconnectContext - OALDevice: %ld\n", mSelfToken);
#endif

	if (inContext != mConnectedContext)
		return; // this context is not currently connected
	
	try {
		
		AUGraphStop (mAUGraph);
		Boolean flag;
		do {
			AUGraphIsRunning (mAUGraph, &flag);
		} while (flag);
		
		// disconnect the mixer from the  output au
		OSStatus	result = AUGraphDisconnectNodeInput (mAUGraph, mOutputNode, 0);
		// update the graph
		for (;;) {
			result = AUGraphUpdate (mAUGraph, NULL);
			if (result == noErr)
				break;
		}
		
		mConnectedContext = NULL;
	}
	catch (OSStatus		result) {
		throw result;
	}
	catch (...) {
		throw -1;
	}
}