/**********************************************************************************************************************************
*
*   OpenAL cross platform audio library
*   Copyright © 2005, Apple Computer, Inc. All rights reserved.
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

#include "oalBuffer.h"
#include "oalSource.h"
#include "oalImp.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark _____Support Methods_____
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// OALBuffers
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ***** OALBuffers *****
OALBuffer::OALBuffer (const ALuint	inSelfToken)
	: 	mSelfToken (inSelfToken),
		mData(NULL),
		mDataSize (0),
		mPreConvertedDataSize(0),
		mDataHasBeenConverted(false),
		mAttachedSourceList(NULL)
{
	mAttachedSourceList = new AttachedSourceList ();			// create a source list map
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OALBuffer::~OALBuffer()
{
	if (mData)
		free (mData);
		
	if (mAttachedSourceList)
		delete mAttachedSourceList;
}
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool		OALBuffer::CanBeRemovedFromBufferMap()
{
	// if the buffer object is not attached to a source, it's ok to remove
	if (mAttachedSourceList->Size() == 0)
		return true;

	// if all attached sources are transitioning to flush, it can be removed from buffer map,
	// but the OALBuffer object must be deleted at a later time
	for (UInt32	i = 0; i < mAttachedSourceList->Size(); i++)
	{
		OALSource	*curSource = mAttachedSourceList->GetSourceByIndex(i);
		if (curSource)
		{
			if (!curSource->IsSourceTransitioningToFlushQ())
				return false;
		}
	}

	return true; // all attached sources are transitioning to flush
}								

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool	OALBuffer::IsPurgable()
{
	if (mAttachedSourceList->Size() == 0)
		return true;
	
	return false;
}
															
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus	OALBuffer::AddAudioData(char*	inAudioData, UInt32	inAudioDataSize, ALenum format, ALsizei freq, bool	inPreConvertToHalFormat)
{
	// creates memory if needed
	// reallocs if needed
	// returns an error if buffer is in use

	try {
        if (!IsFormatSupported(format))
           throw ((OSStatus) AL_INVALID_VALUE);   // this is not a valid buffer token or is an invalid format
	
		// don't allow if the buffer is in a queue
        if (mAttachedSourceList->Size() > 0)
        {
            DebugMessageN1("OALBuffer::AddAudioData: alBufferData ATTACHMENT > 0 - mAttachedSourceList->Size() = %ld", mAttachedSourceList->Size());
            throw ((OSStatus) AL_INVALID_OPERATION);   
        }
		
        mPreConvertedDataSize = (UInt32) inAudioDataSize;
        // do not pre-convert stereo sounds, let the AC do the deinterleaving	
        OSStatus    result = noErr;
        if (!inPreConvertToHalFormat || ((format == AL_FORMAT_STEREO16) || (format == AL_FORMAT_STEREO8)))
        {
            if (mData != NULL)
            {
                if (mDataSize != (UInt32) inAudioDataSize)
                {
                    mDataSize = (UInt32) inAudioDataSize;
                    void *newDataPtr = realloc(mData, mDataSize);
                    mData = (UInt8 *) newDataPtr;
                }		
            }
            else
            {
                mDataSize = (UInt32) inAudioDataSize;
                mData = (UInt8 *) malloc (mDataSize);
            }
            
            if (mData)
            {
                result = FillInASBD(mDataFormat, format, freq);
                    THROW_RESULT
                
                mPreConvertedDataFormat.SetFrom(mDataFormat); //  make sure they are the same so original format info can be returned to caller
                memcpy (mData, inAudioData, mDataSize);		
            }
        }
        else
        {
    #if LOG_EXTRAS		
        DebugMessage("alBufferData called: Converting Data Now");
    #endif
            result = ConvertDataForBuffer(inAudioData, inAudioDataSize, format, freq);	// convert the data to the mixer's format and copy to the buffer
                THROW_RESULT
        }
    }
    catch (OSStatus     result) {
		DebugMessageN1("OALBuffer::AddAudioData Failed - err = %ld\n", result);
        alSetError(result);
		throw result;
    }
    catch (...) {
		DebugMessage("OALBuffer::AddAudioData Failed");
        alSetError(AL_INVALID_OPERATION);
		throw -1;
	}
	
	return noErr;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// This currently will only work for cbr formats
OSStatus OALBuffer::ConvertDataForBuffer(void *inData, UInt32 inDataSize, UInt32	inDataFormat, UInt32 inDataSampleRate)
{
	OSStatus					result = noErr;

    try {

        AudioConverterRef			converter;
        CAStreamBasicDescription	destFormat;
        UInt32						framesOfSource = 0;

        if (inData == NULL)
            throw ((OSStatus) AL_INVALID_OPERATION);
        
        result = FillInASBD(mPreConvertedDataFormat, inDataFormat, inDataSampleRate);
            THROW_RESULT

        // we only should be here for mono sounds for now...
        if (mPreConvertedDataFormat.NumberChannels() == 1)
            mPreConvertedDataFormat.mFormatFlags |= kAudioFormatFlagIsNonInterleaved; 
                    
        destFormat.mChannelsPerFrame = mPreConvertedDataFormat.NumberChannels();
        destFormat.mSampleRate = mPreConvertedDataFormat.mSampleRate;
        destFormat.mFormatID = kAudioFormatLinearPCM;
        
        if (mPreConvertedDataFormat.NumberChannels() == 1)
            destFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
        else
            destFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked; // leave stereo data interleaved, and an AC will be used for deinterleaving later on
        
        destFormat.mFramesPerPacket = 1;	
        destFormat.mBitsPerChannel = sizeof (Float32) * 8;	
        destFormat.mBytesPerPacket = sizeof (Float32) * destFormat.NumberChannels();	
        destFormat.mBytesPerFrame = sizeof (Float32) * destFormat.NumberChannels();
        
        result = FillInASBD(mDataFormat, inDataFormat, UInt32(destFormat.mSampleRate));
            THROW_RESULT
            
        result = AudioConverterNew(&mPreConvertedDataFormat, &destFormat, &converter);
            THROW_RESULT

        framesOfSource = inDataSize / mPreConvertedDataFormat.mBytesPerFrame; // THIS ONLY WORKS FOR CBR FORMATS
        
        UInt32		dataSize = framesOfSource * sizeof(Float32) * destFormat.NumberChannels();
        mDataSize = (UInt32) dataSize;

        if (mData != NULL)
        {
            if (mDataSize != dataSize)
            {
                mDataSize = dataSize;
                void *newDataPtr = realloc(mData, mDataSize);
                if (newDataPtr == NULL)
                    throw ((OSStatus) AL_INVALID_OPERATION);
                    
                mData = (UInt8 *) newDataPtr;
            }		
        }
        else
        {
            mDataSize = dataSize;
            mData = (UInt8 *) malloc (mDataSize);
            if (mData == NULL)
                throw ((OSStatus) AL_INVALID_OPERATION);
        }

        if (mData != NULL)
        {
            result = AudioConverterConvertBuffer(converter, inDataSize, inData, &mDataSize, mData);
            if (result == noErr)
            {
                mDataFormat.SetFrom(destFormat);
                if (mPreConvertedDataFormat.NumberChannels() == 1)
                    mDataHasBeenConverted = true;
                else
                    mDataHasBeenConverted = false;				
            }
        }
        
        AudioConverterDispose(converter);
    }
    catch (OSStatus     result) {
        return (result);
    }
    catch (...) {
        result = (OSStatus) AL_INVALID_OPERATION;
    }
    
	return (result);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Methods called from OALSource objects
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool	OALBuffer::UseThisBuffer(OALSource*	inSource)
{
	// see if this source is in list already
	if (mAttachedSourceList->SourceExists(inSource) == true)
		mAttachedSourceList->IncreaseAttachmentCount(inSource);
	else
	{
		SourceAttachedInfo		nuSourceInfo;
		
		nuSourceInfo.mSource = inSource;
		nuSourceInfo.mAttachedCount = 1;
		
		mAttachedSourceList->Add(nuSourceInfo);
	}

#if LOG_EXTRAS		
	DebugMessageN2("OALBuffer::UseThisBuffer - BufferToken:SourceToken = %ld:%ld", mSelfToken, inSource->GetToken());
#endif
	
	return noErr;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool	OALBuffer::ReleaseBuffer(OALSource*	inSource)
{
#if LOG_EXTRAS		
	DebugMessageN2("OALBuffer::ReleaseBuffer - BufferToken:SourceToken = %ld:%ld", mSelfToken, inSource->GetToken());
#endif
	// see if this source is in list already
	if (mAttachedSourceList->SourceExists(inSource) == true)
	{
		mAttachedSourceList->DecreaseAttachmentCount(inSource);
		if (mAttachedSourceList->GetAttachmentCount(inSource) == 0)
		{
			mAttachedSourceList->Remove(inSource);
			return true;
		}
	}

	return false;
}