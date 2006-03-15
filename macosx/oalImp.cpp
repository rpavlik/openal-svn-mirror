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
	This source file contains all the entry points into the oal state via the defined OpenAL API set. 
*/

#include "oalImp.h"
#include "oalContext.h"
#include "oalDevice.h"
#include "oalSource.h"
#include "oalBuffer.h"

// ~~~~~~~~~~~~~~~~~~~~~~
// development build flags
#define		LOG_API_USAGE		0
#define		LOG_BUFFER_USAGE	0
#define		LOG_SOURCE_USAGE	0
#define		LOG_EXTRAS			0
#define		LOG_ERRORS			0

// ~~~~~~~~~~~~~~~~~~~~~~
// VERSION
const char *alVersion="1.2";

const char *unknownImplementationError="Unknown Internal Error";

// AL_STATE info
const char *alVendor="Apple Computer Inc.";
const char *alRenderer="Software";

const char *alExtensions="";

const char *alNoError				= "No Error";
const char *alErrInvalidName		= "Invalid Name";
const char *alErrInvalidEnum		= "Invalid Enum";
const char *alErrInvalidValue		= "Invalid Enum Value";
const char *alErrInvalidOp			= "Invalid Operation";
const char *alErrOutOfMemory		= "Out of Memory";

const char *alcErrInvalidDevice		= "ALC Invalid Device";
const char *alcErrInvalidContext	= "ALC Invalid Context";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// globals
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// globals
UInt32		gCurrentError = 0;										// globally stored error code
uintptr_t	gCurrentContext = 0;                                    // token for the current context
uintptr_t	gCurrentDevice = 0;                                     // token for the device of the current context
UInt32      gMaximumMixerBusCount = kDefaultMaximumMixerBusCount;   // use gMaximumMixerBusCount for settinmg the bus count when a device is opened

bool		gPreferred3DMixerPresent = true;						// Is the 3DMixer version 2.0 or greater

// At this time, only mono CBR formats would work - no problem as only pcm formats are currently valid
// The feature is turned on using ALC_CONVERT_DATA_UPON_LOADING and the alEnable()/alDisable() APIs
bool        gConvertBufferNow = false;                              // do not convert data into mixer format by default

// global object maps
OALDeviceMap*				gOALDeviceMap = NULL;					// this map will be created upon the first call to alcOpenDevice()
OALContextMap*				gOALContextMap = NULL;					// this map will be created upon the first call to alcCreateContext()
OALBufferMap*				gOALBufferMap = NULL;					// this map will be created upon the first call to alcGenBuffers()
OALBufferMap*				gDeadOALBufferMap = NULL;				// this map will be created upon the first call to alcGenBuffers()

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ***** Support Methods *****
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SetPreferred3DMixerPresent (bool inPreferredMixerIsPresent)
{
    gPreferred3DMixerPresent = inPreferredMixerIsPresent;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool IsPreferred3DMixerPresent ()
{
    return gPreferred3DMixerPresent;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void alSetError (ALenum errorCode)
{
    // only set an error if we are in a no error state
    if (gCurrentError == AL_NO_ERROR)
		gCurrentError = errorCode;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	CleanUpTheDeadBufferList()
{
	if (gDeadOALBufferMap)
	{
		UInt32	index = 0;
		for (UInt32 i = 0; i < gDeadOALBufferMap->Size(); i++)
		{
			OALBuffer*		buffer = gDeadOALBufferMap->GetBufferByIndex(index);
			if (buffer)
			{
				if (buffer->IsPurgable())
				{
					gDeadOALBufferMap->Remove(buffer->GetToken());
					delete (buffer);
				}
				else
					index++;
			}
			else
				index++;
		}
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OALContext*		GetContextObjectByToken (uintptr_t	inContextToken)
{
	if (gOALContextMap == NULL)
		throw ((OSStatus) AL_INVALID_OPERATION);

	OALContext		*oalContext = gOALContextMap->Get(inContextToken);
	if (oalContext == NULL)
		throw ((OSStatus) ALC_INVALID_CONTEXT);
	
	return oalContext;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	SetDeviceError(uintptr_t inDeviceToken, UInt32	inError)
{
	if (gOALDeviceMap != NULL)
	{
		OALDevice			*oalDevice = gOALDeviceMap->Get(inDeviceToken);	// get the requested oal device
		if (oalDevice)
			oalDevice->SetError(inError);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OALDevice*		GetDeviceObjectByToken (uintptr_t inDeviceToken)
{
	if (gOALDeviceMap == NULL)
		throw AL_INVALID_OPERATION;

	OALDevice			*oalDevice = gOALDeviceMap->Get(inDeviceToken);	// get the requested oal device
	if (oalDevice == NULL)
		throw ((OSStatus) AL_INVALID_VALUE);
	
	return oalDevice;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OALSource*	GetSourceObjectFromCurrentContext(ALuint	inSID)
{
	OALContext		*oalContext = GetContextObjectByToken(gCurrentContext);
	OALSource		*oalSource = oalContext->GetSource(inSID);
	if (oalSource == NULL)
		throw ((OSStatus) AL_INVALID_VALUE);
	return oalSource;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OALBuffer*	GetBufferObjectFromToken(ALuint	inBID)
{
	if (gOALBufferMap == NULL)
		throw ((OSStatus) AL_INVALID_OPERATION);

	CleanUpTheDeadBufferList();	

	OALBuffer	*oalBuffer = gOALBufferMap->Get(inBID);
	if (oalBuffer == NULL)
		throw ((OSStatus) AL_INVALID_VALUE);

	return oalBuffer;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void	InitializeBufferMap()
{
	if (gOALBufferMap == NULL)
	{
		gOALBufferMap = new OALBufferMap ();						// create the buffer map since there isn't one yet
		gDeadOALBufferMap = new OALBufferMap ();					// create the buffer map since there isn't one yet

		OALBuffer	*newBuffer = new OALBuffer (AL_NONE);
												
		gOALBufferMap->Add(0, &newBuffer);							// add the new buffer to the buffer map
		gDeadOALBufferMap->Add(0, &newBuffer);						// add the new buffer to the buffer map
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// walk through the context map and find the ones that are linked to this device
void	DeleteContextsOfThisDevice(uintptr_t inDeviceToken)
{
	try {
        if (gOALContextMap == NULL)
            throw ((OSStatus) AL_INVALID_OPERATION);

		for (UInt32	i = 0; i < gOALContextMap->Size(); i++)
		{
			uintptr_t		contextToken = 0;
			OALContext		*oalContext = gOALContextMap->GetContextByIndex(i, contextToken);
			
            if (oalContext == NULL)
                throw ((OSStatus) AL_INVALID_OPERATION);

			if (oalContext->GetDeviceToken() == inDeviceToken)
			{
				// delete this context, it belongs to the device that is going away
				if (contextToken == gCurrentContext)
				{
					// this context is the current context, so remove it as the current context first
					alcMakeContextCurrent(NULL);
				}
				
				delete (oalContext);
				i--; //try this index again since it was just deleted
			}
		}
	}
	catch (OSStatus     result) {
		alSetError(result);
	}
	catch (...) {
		alSetError(AL_INVALID_VALUE);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ALC Methods
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ***** ALC - METHODS *****

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Device APIs
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ***** Devices *****
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALCAPI ALCdevice* ALCAPIENTRY alcOpenDevice(ALCubyte *deviceName)
{
	uintptr_t	newDeviceToken = 0;
	OALDevice	*newDevice = NULL;

#if LOG_API_USAGE
	DebugMessage("alcOpenDevice");
#endif
	
	try {

		if (gOALDeviceMap == NULL)
			gOALDeviceMap = new OALDeviceMap ();                                // create the device map if there isn't one yet
		        
        newDeviceToken = GetNewPtrToken();                                      // get a unique token
        newDevice = new OALDevice((const char *) deviceName, newDeviceToken);	// create a new device object
        if (newDevice != NULL)
            gOALDeviceMap->Add(newDeviceToken, &newDevice);                     // add the new device to the device map
	}
	catch (OSStatus result) {
		DebugMessageN1("ERROR: alcOpenDevice FAILED = %s\n", alcGetString(NULL, result));
		if (newDevice) delete (newDevice);
		SetDeviceError(gCurrentDevice, result);
		return NULL;
    }
	catch (...) {
		DebugMessage("ERROR: alcOpenDevice FAILED");
		if (newDevice) delete (newDevice);
		SetDeviceError(gCurrentDevice, AL_INVALID_OPERATION);
		return NULL;
	}
	
	return ((ALCdevice *) newDeviceToken);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALCAPI ALCvoid    ALCAPIENTRY alcCloseDevice(ALCdevice *device)
{
#if LOG_API_USAGE
	DebugMessage("alcCloseDevice");
#endif

	try {										
		OALDevice	*oalDevice = GetDeviceObjectByToken((uintptr_t) device);	// get the requested oal device

        gOALDeviceMap->Remove((uintptr_t) device);								// remove the device from the map
        
        DeleteContextsOfThisDevice((uintptr_t) device);
        delete (oalDevice);														// destruct the device object
        
        if (gOALDeviceMap->Empty())
        {
            // there are no more devices in the map, so delete the map and create again later if needed
            delete (gOALDeviceMap);
            gOALDeviceMap = NULL;
        }
	}
	catch (OSStatus   result) {
		DebugMessageN1("ERROR: alcCloseDevice FAILED = %s\n", alcGetString(NULL, result));
		SetDeviceError(gCurrentDevice, result);
		return;
	}
    catch (...) {
		DebugMessage("ERROR: alcCloseDevice FAILED");
		SetDeviceError(gCurrentDevice, AL_INVALID_OPERATION);
		return;
	}

	return; // success
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALCAPI ALCenum  ALCAPIENTRY alcGetError(ALCdevice *device)
{
#if LOG_API_USAGE
	DebugMessage("alcGetError");
#endif

	try {
			if (gOALDeviceMap)
			{
				OALDevice*		oalDevice = GetDeviceObjectByToken ((uintptr_t) device);
				if (oalDevice)
					return (oalDevice->GetError());
			}
			
			return ALC_INVALID_DEVICE;
	} 
	catch (OSStatus	result) {
		DebugMessage("ERROR: alcCloseDevice FAILED: ALC_INVALID_DEVICE");
		return ALC_INVALID_DEVICE;
	}
	catch (...) {
		DebugMessage("ERROR: alcCloseDevice FAILED: ALC_INVALID_DEVICE");
		return ALC_INVALID_DEVICE;
	}
	return noErr;	// don't know about this device
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Context APIs
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ***** Contexts *****

// There is no attribute support yet
ALCAPI ALCcontext*	ALCAPIENTRY alcCreateContext(ALCdevice *device, ALCint *attrList)
{
	uintptr_t		newContextToken = 0;
	OALContext		*newContext = NULL;

#if LOG_API_USAGE
	DebugMessageN1("alcCreateContext--> device = %ld", (long int) device);
#endif
	
	try {
		OALDevice	*oalDevice = GetDeviceObjectByToken ((uintptr_t) device);

		// create the context map if there isn't one yet
		if (gOALContextMap == NULL)
			gOALContextMap = new OALContextMap();
	
		newContextToken = GetNewPtrToken();
				
		newContext = new OALContext(newContextToken, oalDevice, gMaximumMixerBusCount);
		
		gOALContextMap->Add(newContextToken, &newContext);	
	}
	catch (OSStatus     result){
		DebugMessageN1("ERROR: alcCreateContext FAILED = %s\n", alcGetString(NULL, result));
		if (newContext) delete (newContext);
		SetDeviceError(gCurrentDevice, result);
		return (NULL);
	}
    catch (...) {
		DebugMessage("ERROR: alcCreateContext FAILED");
		SetDeviceError(gCurrentDevice, AL_INVALID_OPERATION);
		return (NULL);
	}
	
	return ((ALCcontext *) newContextToken);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALCAPI ALCboolean  ALCAPIENTRY alcMakeContextCurrent(ALCcontext *context)
{
#if LOG_API_USAGE
	DebugMessageN1("alcMakeContextCurrent--> context = %ld", (long int) context);
#endif

	OALContext		*newContext = NULL;
	OALContext		*oldContext = NULL;

	if ((uintptr_t) context == gCurrentContext)
		return AL_TRUE;								// no change necessary, already using this context
	
	try {	
		if ((gOALContextMap == NULL) || (gOALDeviceMap == NULL))
            throw ((OSStatus) AL_INVALID_OPERATION);

		// get the current context if there is one
		if (gCurrentContext != 0)
			oldContext = gOALContextMap->Get(gCurrentContext);
			
		if (context == 0)
		{
			// caller passed NULL, which means no context should be current
			gCurrentDevice = 0;
			gCurrentContext = 0;
			// disable the current context if there is one
			if (oldContext != NULL)
				oldContext->DeviceDisconnect();
		}
		else
		{
			newContext = gOALContextMap->Get((uintptr_t) context);
			if (newContext == NULL)
				throw ((OSStatus) ALC_INVALID_CONTEXT);    // fail because the context is invalid

			// find the device that owns this context
			uintptr_t		newCurrentDeviceToken = gOALContextMap->GetDeviceTokenForContext((uintptr_t) context);
			
			// new context is obtained so disable the old one now
			if (oldContext)
				oldContext->DeviceDisconnect();
            
			// store the new current context and device
			gCurrentDevice = newCurrentDeviceToken;
			gCurrentContext = (uintptr_t) context;
				
			newContext->DeviceConnect();
		}
		
		return AL_TRUE;
	}
	catch (OSStatus result) {
		DebugMessageN1("ERROR: alcMakeContextCurrent FAILED = %s\n", alcGetString(NULL, result));
		// get the Device object for this context so we can set an error code
		SetDeviceError(gCurrentDevice, result);
	}
    catch (...) {
		DebugMessage("ERROR: alcMakeContextCurrent FAILED");
		SetDeviceError(gCurrentDevice, AL_INVALID_OPERATION);
	}
	
	return AL_FALSE;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALCAPI ALCvoid	  ALCAPIENTRY alcProcessContext(ALCcontext *context)
{
#if LOG_API_USAGE
	DebugMessageN1("alcProcessContext--> context = %ld", (long int) context);
#endif

	try {
		OALContext		*oalContext = GetContextObjectByToken((uintptr_t) context);
        oalContext->ProcessContext();
    }
    catch (OSStatus result) {
		DebugMessageN1("ERROR: alcProcessContext FAILED = %s\n", alcGetString(NULL, result));
		SetDeviceError(gCurrentDevice, result);
    }
    catch (...) {
		DebugMessage("ERROR: alcProcessContext FAILED");
		SetDeviceError(gCurrentDevice, AL_INVALID_OPERATION);
	}
	
    return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALCAPI ALCcontext* ALCAPIENTRY alcGetCurrentContext(ALCvoid)
{
#if LOG_API_USAGE
	DebugMessage("alcGetCurrentContext");
#endif

	return ((ALCcontext *) gCurrentContext);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// find out what device the context uses
ALCAPI ALCdevice*  ALCAPIENTRY alcGetContextsDevice(ALCcontext *context)
{
	UInt32	returnValue = 0;

#if LOG_API_USAGE
	DebugMessageN1("alcGetContextsDevice--> context = %ld", (long int) context);
#endif
	
	try {
        if (gOALContextMap == NULL)
            throw ((OSStatus) AL_INVALID_OPERATION);
        
        returnValue = gOALContextMap->GetDeviceTokenForContext((uintptr_t) context);
    }
    catch (OSStatus result) {
		DebugMessageN1("ERROR: alcGetContextsDevice FAILED = %s\n", alcGetString(NULL, result));
		SetDeviceError(gCurrentDevice, result);
    }
    catch (...) {
		DebugMessage("ERROR: alcGetContextsDevice FAILED");
		SetDeviceError(gCurrentDevice, AL_INVALID_OPERATION);
	}
    
	return ((ALCdevice*) returnValue);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALCAPI ALCvoid	  ALCAPIENTRY alcSuspendContext(ALCcontext *context)
{
#if LOG_API_USAGE
	DebugMessageN1("alcSuspendContext--> context = %ld", (long int) context);
#endif

    try {
		OALContext		*oalContext = GetContextObjectByToken((uintptr_t) context);
        oalContext->SuspendContext();
    }
    catch (OSStatus     result) {
		DebugMessageN1("ERROR: alcSuspendContext FAILED = %s\n", alcGetString(NULL, result));
		SetDeviceError(gCurrentDevice, result);
    }
    catch (...) {
		DebugMessage("ERROR: alcSuspendContext FAILED");
		SetDeviceError(gCurrentDevice, AL_INVALID_OPERATION);
	}
    
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALCAPI ALCvoid	ALCAPIENTRY alcDestroyContext (ALCcontext *context)
{
#if LOG_API_USAGE
	DebugMessageN1("alcDestroyContext--> context = %ld", (long int) context);
#endif

	try {
        // if this was the current context, set current context to NULL
        if (gCurrentContext == (uintptr_t) context)
            alcMakeContextCurrent(NULL);

		if (gOALContextMap)
		{
			OALContext		*deleteThis = gOALContextMap->Get((uintptr_t) context);
			if (!gOALContextMap->Remove((uintptr_t) context))	// remove from the map
				throw ((OSStatus) ALC_INVALID_CONTEXT);
				
			delete(deleteThis);
		}
		else
            throw ((OSStatus) AL_INVALID_OPERATION);
	}
	catch (OSStatus     result) {
		DebugMessageN1("ERROR: alcDestroyContext FAILED = %s\n", alcGetString(NULL, result));
		SetDeviceError(gCurrentDevice, result);
	}
    catch (...) {
		DebugMessage("ERROR: alcDestroyContext FAILED");
		SetDeviceError(gCurrentDevice, AL_INVALID_OPERATION);
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Other APIs
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ***** Other *****

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALCAPI ALCubyte*  ALCAPIENTRY alcGetString(ALCdevice *device,ALCenum param)
{
#if LOG_API_USAGE
	DebugMessageN1("alcGetString-->  %s", GetALCAttributeString(param));
#endif

	try {

		switch (param)
		{			
			case ALC_NO_ERROR:
				return (ALCubyte *) alNoError;
				break;
				
			case ALC_INVALID_DEVICE:
				return (ALCubyte *) alcErrInvalidDevice;
				break;
				
			case ALC_INVALID_CONTEXT:
				return (ALCubyte *) alcErrInvalidContext;
				break;
				
			case ALC_INVALID_ENUM:
				return (ALCubyte *) alErrInvalidEnum;
				break;
				
			case ALC_INVALID_VALUE:
				return (ALCubyte *) alErrInvalidValue;
				break;

			default:
				throw (OSStatus) AL_INVALID_VALUE;
				break;
		}
	}
	catch (OSStatus		result) {
		alSetError(result);
 		DebugMessageN2("ERROR: alcGetString FAILED - attribute = %s error = %s\n", GetALCAttributeString(param), alcGetString(NULL, result));
    }
	catch (...) {
 		DebugMessageN1("ERROR: alcGetString FAILED - attribute = %s", GetALCAttributeString(param));
	}

	return NULL;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALCAPI ALCboolean ALCAPIENTRY alcIsExtensionPresent(ALCdevice *device,ALCubyte *extName)
{
#if LOG_API_USAGE
	DebugMessage("alcIsExtensionPresent called");
#endif

	return AL_FALSE;    // no extensions present in this implementation
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALCAPI ALCvoid *  ALCAPIENTRY alcGetProcAddress(ALCdevice *device,ALCubyte *funcName)
{
#if LOG_API_USAGE
	DebugMessageN1("alcGetProcAddress--> function name = %s", funcName);
#endif

	return (alGetProcAddress(funcName));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALCAPI ALCenum	  ALCAPIENTRY alcGetEnumValue(ALCdevice *device,ALCubyte *enumName)
{
#if LOG_API_USAGE
	DebugMessageN1("alcGetEnumValue--> enum name = %s", enumName);
#endif

	return (alGetEnumValue (enumName));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALint ALAPIENTRY alcGetInteger (ALCdevice *device, ALenum pname)
{
#if LOG_API_USAGE
	DebugMessageN2("alcGetInteger--> device = %ld attribute name = %s", (long int) device, GetALCAttributeString(pname));
#endif

	UInt32				returnValue	= 0;
	OALDevice			*oalDevice = gOALDeviceMap->Get((uintptr_t) device);	// get the requested oal device

	try {

		switch (pname)
		{
			default:
				throw (OSStatus) AL_INVALID_VALUE;
				break;
		}
	}
	catch (OSStatus		result) {
		if (oalDevice)
			oalDevice->SetError(result);
		else 
			alSetError(result);

		DebugMessageN3("ERROR: alcGetInteger FAILED: device = %ld attribute name = %s error = %s", (long int) device, GetALCAttributeString(pname), alcGetString(NULL, result));
    }
	catch (...) {
		DebugMessageN3("ERROR: alcGetInteger FAILED: device = %ld attribute name = %s error = %s", (long int) device, GetALCAttributeString(pname), "Unknown Error");
	}
	
	return (returnValue);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALCAPI ALCvoid     ALCAPIENTRY alcGetIntegerv(ALCdevice *device, ALCenum pname, ALCsizei size, ALCint *data)
{	
#if LOG_API_USAGE
	DebugMessageN2("alcGetIntegerv--> device = %ld attribute name = %s", (long int) device, GetALCAttributeString(pname));
#endif

	// get the device
	OALDevice			*oalDevice = gOALDeviceMap->Get((uintptr_t) device);	// get the requested oal device

	try {

		if ((data == NULL) || (size == 0))
			throw ALC_INVALID_VALUE;

		switch (pname)
		{			
			default:
				throw (OSStatus) AL_INVALID_VALUE;
				break;
		}
	}
	catch (OSStatus		result) {
		if (oalDevice)
			oalDevice->SetError(result);
		else 
			alSetError(result);

		DebugMessageN3("ERROR: alcGetInteger FAILED: device = %ld attribute name = %s error = %s", (long int) device, GetALCAttributeString(pname), alcGetString(NULL, result));
    }
	catch (...) {
		DebugMessageN3("ERROR: alcGetInteger FAILED: device = %ld attribute name = %s error = %s", (long int) device, GetALCAttributeString(pname), "Unknown Error");
	}

	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// AL Methods
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ***** AL - METHODS *****

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALenum ALAPIENTRY alGetError ()
{
    ALenum	error = AL_NO_ERROR;

#if LOG_API_USAGE
	DebugMessage("alGetError");
#endif

	if (gCurrentError != AL_NO_ERROR)
    {
#if LOG_ERRORS
		DebugMessageN1("alGetError: error = 0x%X\n", (uint) gCurrentError);
#endif
		error = gCurrentError;
		gCurrentError = AL_NO_ERROR;

		// this call should also clear the error on the current device as well
		OALDevice		*device = gOALDeviceMap->Get((UInt32) gCurrentDevice);
		if (device)
			device->SetError(AL_NO_ERROR);
	}
	
	return (error);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Buffer APIs
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ***** Buffers *****

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alGenBuffers(ALsizei n, ALuint *bids)
{
#if LOG_BUFFER_USAGE
	DebugMessageN1("alGenBuffers--> requested buffers = %ld", (long int) n);
#endif
	
	try {
		if (n < 0)
            throw ((OSStatus) AL_INVALID_VALUE);

        InitializeBufferMap();
        if (gOALBufferMap == NULL)
            throw ((OSStatus) AL_INVALID_OPERATION);

		CleanUpTheDeadBufferList();	
		
		if ((n + gOALBufferMap->Size() > AL_MAXBUFFERS) || (n > AL_MAXBUFFERS))
            throw ((OSStatus) AL_INVALID_VALUE);
		
        for (UInt32	i = 0; i < (UInt32) n; i++)
        {
			ALuint	newBufferToken = GetNewToken();		// get a unique token

			OALBuffer	*newBuffer = new OALBuffer (newBufferToken);
			
            gOALBufferMap->Add(newBufferToken, &newBuffer);			// add the new buffer to the buffer map
            bids[i] = newBufferToken;
        }
	}
	catch (OSStatus     result) {
		DebugMessageN2("ERROR: alGenBuffers FAILED: requested buffers = %ld error %s", (long int) n, alGetString(result));
        alSetError (result);
	}
    catch (...) {
		DebugMessageN1("ERROR: alGenBuffers FAILED: requested buffers = %ld", (long int) n);
        alSetError(AL_INVALID_OPERATION);
	}

#if LOG_BUFFER_USAGE
	printf("alGenBuffers--> (%ld) ", (long int) n);
	for (UInt32	i = 0; i < (UInt32) n; i++) {
		printf("%ld, ", (long int) bids[i]);
	}
	printf("\n");
#endif
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid	ALAPIENTRY alDeleteBuffers( ALsizei n, ALuint* buffers )
{
#if LOG_BUFFER_USAGE
	printf("alDeleteBuffers--> (%ld) ", (long int) n);
	for (UInt32	i = 0; i < (UInt32) n; i++) {
		printf("%ld, ", (long int) buffers[i]);
	}
	printf("\n");
#endif
	
	try {
        if (gOALBufferMap == NULL)
        {
            DebugMessage("alDeleteBuffers: gOALBufferMap == NULL");
            throw ((OSStatus) AL_INVALID_OPERATION);   
        }

		CleanUpTheDeadBufferList();	
        
        if (gOALBufferMap->Empty())
            return; // nothing to do
            
        if ((UInt32) n > gOALBufferMap->Size() - 1)
        {
            DebugMessage("alDeleteBuffers: n > gOALBufferMap->Size() - 1");
            throw ((OSStatus) AL_INVALID_VALUE);   
        }
        else
        {
            UInt32	i;
            // see if any of the buffers are attached to a source or are invalid names
            for (i = 0; i < (UInt32) n; i++)
            {
                // don't bother verifying the NONE buffer, it won't be deleted anyway
                if (buffers[i] != AL_NONE)
                {
					OALBuffer	*oalBuffer = gOALBufferMap->Get(buffers[i]);
                    if (oalBuffer == NULL)
                    {
                        DebugMessage("alDeleteBuffers: oalBuffer == NULL");
                        throw ((OSStatus) AL_INVALID_VALUE);    // the buffer is invalid
                    }
                    else if (!oalBuffer->CanBeRemovedFromBufferMap())
                    {
                        throw ((OSStatus) AL_INVALID_VALUE);    // the buffer is attached to a source so set an error and bail
                    }
                }
            }
        
            // All the buffers are OK'd for deletion, so delete them now
            for (i = 0; i < (UInt32) n; i++)
            {
                // do not delete the NONE buffer at the beginning of the map
                if (buffers[i] != AL_NONE)
                {
                    OALBuffer	*buffer = gOALBufferMap->Get((UInt32) buffers[i]);
					if (buffer->IsPurgable())
					{
						gOALBufferMap->Remove((UInt32) buffers[i]);
						delete (buffer);
					}
					else
					{
						gDeadOALBufferMap->Add(buffer->GetToken(), &buffer);	// add this buffer object to a dead list that can be cleaned up later
						gOALBufferMap->Remove((UInt32) buffers[i]);				// remove it from the good list so it won't be used again
					}
				}
            }
        }
    }
    catch (OSStatus     result) {
		DebugMessageN1("ERROR: alDeleteBuffers FAILED = %s", alGetString(result));
        alSetError(result);
    }
    catch (...) {
		DebugMessage("ERROR: alDeleteBuffers FAILED");
        alSetError(AL_INVALID_OPERATION);
	}
    
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALboolean ALAPIENTRY alIsBuffer(ALuint bid)
{
#if LOG_BUFFER_USAGE
	DebugMessageN1("alIsBuffer--> buffer %ld", (long int) bid);
#endif
	if (bid == 0)
		return true;	// 0 == AL_NONE which is valid

    try {
        if (gOALBufferMap == NULL)
            throw ((OSStatus) AL_INVALID_OPERATION);   

		CleanUpTheDeadBufferList();	

        OALBuffer	*oalBuffer = gOALBufferMap->Get((UInt32) bid);

        if (oalBuffer != NULL)
            return AL_TRUE;
        else
            return AL_FALSE;
    }
    catch (OSStatus     result) {
		DebugMessageN2("ERROR: alIsBuffer FAILED: buffer = %ld error = %s", (long int) bid, alGetString(result));
        alSetError(result);
    }
    catch (...) {
		DebugMessageN1("ERROR: alIsBuffer FAILED: buffer = %ld", (long int) bid);
        alSetError(AL_INVALID_OPERATION);
	}

    return AL_FALSE;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid	ALAPIENTRY alBufferData( ALuint   buffer,
										 ALenum   format,
										 ALvoid*  data,
										 ALsizei  size,
										 ALsizei  freq )
{
#if LOG_BUFFER_USAGE
	DebugMessageN4("alBufferData-->  buffer %ld : %s : %ld bytes : %ldHz", (long int) buffer, GetFormatString(format), (long int) size, (long int) freq);
#endif

	try {
			OALBuffer	*oalBuffer = GetBufferObjectFromToken(buffer);
			oalBuffer->AddAudioData((char*)data, size, format, freq, gConvertBufferNow); // should also check for a valid format IsFormatSupported()
    }
    catch (OSStatus     result) {
		DebugMessageN5("ERROR: alBufferData FAILED: buffer %ld : %s : %ld bytes : %ldHz error = %s", (long int) buffer, GetFormatString(format), (long int) size, (long int) freq,  alGetString(result));
        alSetError(result);
    }
    catch (...) {
		DebugMessageN4("ERROR: alBufferData FAILED: buffer %ld : %s : %ld bytes : %ldHz", (long int) buffer, GetFormatString(format), (long int) size, (long int) freq);
        alSetError(AL_INVALID_OPERATION);
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alGetBufferf (ALuint bid, ALenum pname, ALfloat *value)
{
#if LOG_BUFFER_USAGE
	DebugMessageN2("alGetBufferf--> buffer %ld : property = %s", (long int) bid, GetALAttributeString(pname));
#endif

    try {
		OALBuffer	*oalBuffer = GetBufferObjectFromToken(bid);
        switch (pname)
        {
            case AL_FREQUENCY:
                *value = oalBuffer->GetSampleRate();
                break;
            default:
                alSetError(AL_INVALID_ENUM);
                break;
        }
    }
    catch (OSStatus result) {
		DebugMessageN3("ERROR: alGetBufferf FAILED: buffer %ld : property = %s error = %s", (long int) bid, GetALAttributeString(pname), alGetString(result));
        alSetError(result);
    }
    catch (...) {
		DebugMessageN2("ERROR: alGetBufferf FAILED: buffer %ld : property = %s", (long int) bid, GetALAttributeString(pname));
        alSetError(AL_INVALID_OPERATION);
	}
    
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alGetBufferi(ALuint bid, ALenum pname, ALint *value)
{
#if LOG_BUFFER_USAGE
	DebugMessageN2("alGetBufferi--> buffer %ld : property = %s", (long int) bid, GetALAttributeString(pname));
#endif

    try {
		OALBuffer	*oalBuffer = GetBufferObjectFromToken(bid);
        switch (pname)
        {
            case AL_FREQUENCY:
                *value = (UInt32) oalBuffer->GetSampleRate();
                break;
            case AL_BITS:
                *value = oalBuffer->GetPreConvertedBitsPerChannel();
                break;
            case AL_CHANNELS:
                *value = oalBuffer->GetNumberChannels();
                break;
            case AL_SIZE:
                *value = oalBuffer->GetPreConvertedDataSize();
                break;
            default:
                *value = 0;
				alSetError(AL_INVALID_ENUM);
                break;
        }        
    }
    catch (OSStatus result) {
		DebugMessageN3("ERROR: alGetBufferi FAILED: buffer = %ld property = %s error = %s", (long int) bid, GetALAttributeString(pname), alGetString(result));
		*value = 0;
		alSetError(result);
    }
    catch (...) {
		DebugMessageN2("ERROR: alGetBufferi FAILED: buffer = %ld property = %s", (long int) bid, GetALAttributeString(pname));
		*value = 0;
        alSetError(AL_INVALID_OPERATION);
	}
    
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Source APIs
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ***** Sources *****

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALCAPI ALvoid  ALAPIENTRY alGenSources(ALsizei n, ALuint *sources)
{
#if LOG_SOURCE_USAGE
	DebugMessageN1("alGenSources--> requested sources = %ld", (long int) n);
#endif

	if (n == 0)
		return; // NOP

	UInt32      i = 0,
                count = 0;
	try {
		if (n < 0)
            throw ((OSStatus) AL_INVALID_VALUE);

        if ((n > AL_MAXSOURCES) || (sources == NULL))
            throw ((OSStatus) AL_INVALID_VALUE);
        
		OALContext		*oalContext = GetContextObjectByToken(gCurrentContext);
        for (i = 0; i < (UInt32) n; i++)
        {
            ALuint	newToken = GetNewToken();		// get a unique token
            
            oalContext->AddSource(newToken);		// add this source to the context
            sources[i] = newToken; 					// return the source token
            count++;
        }
	}
	catch (OSStatus     result){
		DebugMessageN2("ERROR: alGenSources FAILED: source count = %ld error = %s", (long int) n, alGetString(result));
		// some of the sources could not be created, so delete the ones that were and return none
		alSetError(result);
		alDeleteSources(i, sources);
		for (i = 0; i < count; i++)
			sources[i] = 0;
	}
    catch (...) {
		DebugMessageN1("ERROR: alGenSources FAILED: source count = %ld", (long int) n);
        alSetError(AL_INVALID_OPERATION);
		// some of the sources could not be created, so delete the ones that were and return none
		alDeleteSources(i, sources);
		for (i = 0; i < count; i++)
			sources[i] = 0;
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid	ALAPIENTRY alDeleteSources( ALsizei n, ALuint* sources )
{
#if LOG_SOURCE_USAGE
	//DebugMessageN1("alDeleteSources: count = %ld", (long int) n);
	printf("alDeleteSources--> (%ld) ", (long int) n);
	for (UInt32	i = 0; i < (UInt32) n; i++) {
		printf("%ld, ", (long int) sources[i]);
	}
	printf("\n");
#endif

	if (n == 0)
		return; // NOP

	try {
		OALContext		*oalContext = GetContextObjectByToken(gCurrentContext);

        if ((UInt32) n > oalContext->GetSourceCount())
            throw ((OSStatus) AL_INVALID_VALUE);
        
        for (UInt32 i = 0; i < (UInt32) n; i++)
        {
            oalContext->RemoveSource(sources[i]);
        }
	}
	catch (OSStatus     result) {
		DebugMessageN2("ERROR: alDeleteSources FAILED: source count = %ld error = %s", (long int) n, alGetString(result));
		alSetError(result);
	}
    catch (...) {
		DebugMessageN1("ERROR: alDeleteSources FAILED: source count = %ld", (long int) n);
        alSetError(AL_INVALID_OPERATION);
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALCAPI ALboolean  ALAPIENTRY alIsSource(ALuint sid)
{
#if LOG_SOURCE_USAGE
	DebugMessageN1("alIsSource--> source %ld", (long int) sid);
#endif

	try {
		OALSource		*oalSource = GetSourceObjectFromCurrentContext(sid);
        if (oalSource != NULL)
            return AL_TRUE;
        else
            return AL_FALSE;
	}
	catch (OSStatus     result) {
		DebugMessageN2("ERROR: alIsSource FAILED: source = %ld error = %s", (long int) sid, alGetString(result));
		alSetError(result);
	}
    catch (...) {
		DebugMessageN1("ERROR: alIsSource FAILED: source = %ld", (long int) sid);
        alSetError(AL_INVALID_OPERATION);
	}

	return AL_FALSE;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alSourcef (ALuint sid, ALenum pname, ALfloat value)
{
#if LOG_SOURCE_USAGE
	DebugMessageN3("alSourcef--> source %ld : %s : value = %.f2", (long int) sid, GetALAttributeString(pname), value);
#endif

	try {
		OALSource		*oalSource = GetSourceObjectFromCurrentContext(sid);
        switch (pname) 
        {
			// Source ONLY Attributes
            case AL_MIN_GAIN:
                oalSource->SetMinGain(value);
                break;
            case AL_MAX_GAIN:
                oalSource->SetMaxGain(value);
                break;
            case AL_REFERENCE_DISTANCE:
                if (value <= 0.0f)
                    throw ((OSStatus) AL_INVALID_VALUE);
                oalSource->SetReferenceDistance(value);
                break;
            case AL_ROLLOFF_FACTOR:
                oalSource->SetRollOffFactor(value);
                break;
            case AL_MAX_DISTANCE:
                oalSource->SetMaxDistance(value);
                break;
            case AL_PITCH:
                oalSource->SetPitch(value);
                break;
            case AL_CONE_INNER_ANGLE:
                oalSource->SetConeInnerAngle(value);
                break;
            case AL_CONE_OUTER_ANGLE:
                oalSource->SetConeOuterAngle(value);
                break;
            case AL_CONE_OUTER_GAIN:
				oalSource->SetConeOuterGain(value);
                break;

			// Source & Listener Attributes
            case AL_GAIN:
                oalSource->SetGain(value);
                break;
			
            default:
                alSetError(AL_INVALID_OPERATION);
                break;
        }
	}
	catch (OSStatus     result) {
		DebugMessageN4("ERROR alSourcef FAILED: source %ld : property = %s : value = %.f2 : error = %s", (long int) sid, GetALAttributeString(pname), value, alGetString(result));
		alSetError(result);
	}
    catch (...) {
		DebugMessageN4("ERROR alSourcef FAILED: source %ld : property = %s : value = %.f2 : error = %s", (long int) sid, GetALAttributeString(pname), value, alGetString(-1));
        alSetError(AL_INVALID_OPERATION);
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alSourcefv( ALuint source, ALenum param, ALfloat* values )
{
#if LOG_SOURCE_USAGE
	DebugMessageN2("alSourcefv--> source %ld : %s", (long int) source, GetALAttributeString(param));
#endif

	try {
		OALSource		*oalSource = GetSourceObjectFromCurrentContext(source);
        switch(param) 
        {
			// Source ONLY Attributes
            case AL_MIN_GAIN:
                oalSource->SetMinGain(*values);
                break;
            case AL_MAX_GAIN:
                oalSource->SetMaxGain(*values);
                break;
            case AL_REFERENCE_DISTANCE:
                oalSource->SetReferenceDistance(*values);
                break;
            case AL_ROLLOFF_FACTOR:
                if (*values < 0.0f) 
                    throw ((OSStatus) AL_INVALID_VALUE);
                oalSource->SetRollOffFactor(*values);
                break;
            case AL_MAX_DISTANCE:
                oalSource->SetMaxDistance(*values);
                break;
            case AL_PITCH:
                oalSource->SetPitch(*values);
                break;
            case AL_DIRECTION:
                oalSource->SetDirection(values[0], values[1], values[2]);
                break;
            case AL_CONE_INNER_ANGLE:
                oalSource->SetConeInnerAngle(*values);
                break;
            case AL_CONE_OUTER_ANGLE:
                oalSource->SetConeOuterAngle(*values);
                break;
            case AL_CONE_OUTER_GAIN:
				oalSource->SetConeOuterGain(*values);
                break;
			
			// Source & Listener Attributes
            case AL_POSITION:
                oalSource->SetPosition(values[0], values[1], values[2]);
                break;
            case AL_VELOCITY:
                oalSource->SetVelocity(values[0], values[1], values[2]);
                break;
            case AL_GAIN:
                oalSource->SetGain(*values);
                break;

            default:
                alSetError(AL_INVALID_ENUM);
                break;
        }
	}
	catch(OSStatus      result) {
		DebugMessageN3("ERROR alSourcefv FAILED: source = %ld property = %s result = %s\n", (long int) source, GetALAttributeString(param), alGetString(result));
		alSetError(result);
	}
    catch (...) {
		DebugMessageN3("ERROR alSourcefv: FAILED : property : result %ld : %s : %s\n", (long int) source, GetALAttributeString(param), alGetString(-1));
        alSetError(AL_INVALID_OPERATION);
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alSource3f (ALuint sid, ALenum pname, ALfloat v1, ALfloat v2, ALfloat v3)
{
#if LOG_SOURCE_USAGE
	DebugMessageN5("alSource3f--> source %ld : %s : values = %.f2:%.f2:%.f2", (long int) sid, GetALAttributeString(pname), v1, v2, v3);
#endif

	try {
		OALSource		*oalSource = GetSourceObjectFromCurrentContext(sid);
        switch (pname) 
        {
			// Source ONLY Attributes
            case AL_DIRECTION:
                oalSource->SetDirection(v1, v2, v3);
                break;

			// Source & Listener Attributes
            case AL_POSITION:
                oalSource->SetPosition(v1, v2, v3);
                break;
            case AL_VELOCITY:
                oalSource->SetVelocity(v1, v2, v3);
                break;

            default:
                alSetError(AL_INVALID_ENUM);
                break;
        }
	}
	catch (OSStatus      result) {
		DebugMessageN6("ERROR: alSource3f FAILED: source %ld : %s : values = %.f2:%.f2:%.f2 error = %s", (long int) sid, GetALAttributeString(pname), v1, v2, v3, alGetString(result));
		alSetError(result);
	}
    catch (...) {
		DebugMessageN5("ERROR: alSource3f FAILED: source %ld : %s : values = %.f2:%.f2:%.f2", (long int) sid, GetALAttributeString(pname), v1, v2, v3);
        alSetError(AL_INVALID_OPERATION);
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alSourcei (ALuint sid, ALenum pname, ALint value)
{
#if LOG_SOURCE_USAGE
	DebugMessageN3("alSourcei--> source %ld : %s : value = %ld", (long int) sid, GetALAttributeString(pname), (long int)value);
#endif

	try {
		OALSource		*oalSource = GetSourceObjectFromCurrentContext(sid);
        switch (pname) 
        {
			// Source ONLY Attributes
            case AL_SOURCE_RELATIVE:
				oalSource->SetSourceRelative(value);
                break;
            case AL_LOOPING:
                oalSource->SetLooping(value);
                break;
            case AL_BUFFER:
                {
					// The source type must first be checked for static or streaming
					////if (oalSource->GetSourceType() == AL_STREAMING && value != 0)
					////	throw ((OSStatus) AL_INVALID_OPERATION);

                    if (gOALBufferMap == NULL)
                        throw ((OSStatus) AL_INVALID_OPERATION);   

                    if (alIsBuffer(value))
					    oalSource->SetBuffer(value, gOALBufferMap->Get((UInt32) value));
					else
                        throw ((OSStatus) AL_INVALID_OPERATION);
                } 
                break;
            case AL_REFERENCE_DISTANCE:
                oalSource->SetReferenceDistance(value);
                break;
            case AL_ROLLOFF_FACTOR:
                oalSource->SetRollOffFactor(value);
                break;
            case AL_MAX_DISTANCE:
                oalSource->SetMaxDistance(value);
                break;
			case AL_CONE_INNER_ANGLE:
                oalSource->SetConeInnerAngle(value);
                break;
            case AL_CONE_OUTER_ANGLE:
                oalSource->SetConeOuterAngle(value);
                break;
            case AL_CONE_OUTER_GAIN:
				oalSource->SetConeOuterGain(value);
                break;
            
            default:
                alSetError(AL_INVALID_ENUM);
                break;
        }
	}
	catch (OSStatus      result) {
		DebugMessageN4("ERROR: alSourcei FAILED - sid:pname:value:result %ld:%s:%ld:%s", (long int) sid, GetALAttributeString( pname), (long int) value, alGetString(result));
		alSetError(result);
	}
    catch (...) {
		DebugMessageN3("ERROR: alSourcei FAILED - sid:pname:value %ld:%s:%ld", (long int) sid, GetALAttributeString( pname), (long int) value);
        alSetError(AL_INVALID_OPERATION);
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alGetSourcef (ALuint sid, ALenum pname, ALfloat *value)
{
#if LOG_SOURCE_USAGE
	DebugMessageN2("alGetSourcef--> source %ld : %s", (long int) sid, GetALAttributeString(pname));
#endif

	try {
		OALSource		*oalSource = GetSourceObjectFromCurrentContext(sid);
        switch (pname) 
        {
			// Source ONLY Attributes
            case AL_MIN_GAIN:
                *value = oalSource->GetMinGain();
                break;
            case AL_MAX_GAIN:
                *value = oalSource->GetMaxGain();
                break;
            case AL_REFERENCE_DISTANCE:
                *value = oalSource->GetReferenceDistance();
                break;
            case AL_ROLLOFF_FACTOR:
                *value = oalSource->GetRollOffFactor();
                break;
            case AL_MAX_DISTANCE:
                *value = oalSource->GetMaxDistance();
                break;
            case AL_PITCH:
                *value = oalSource->GetPitch();
                break;
            case AL_CONE_INNER_ANGLE:
                *value = oalSource->GetConeInnerAngle();
                break;
            case AL_CONE_OUTER_ANGLE:
                *value = oalSource->GetConeOuterAngle();
                break;
            case AL_CONE_OUTER_GAIN:
                *value = oalSource->GetConeOuterGain();
                break;

			// Source & Listener Attributes
            case AL_GAIN:
                *value = oalSource->GetGain();
                break;

            default:
                alSetError(AL_INVALID_ENUM);
                break;
        }
	}
	catch (OSStatus      result) {
		DebugMessageN1("ERROR: alGetSourcef FAILED = %s\n", alGetString(result));
		alSetError(result);
	}
    catch (...) {
		DebugMessage("ERROR: alGetSourcef FAILED");
        alSetError(AL_INVALID_OPERATION);
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alGetSourcefv (ALuint sid, ALenum pname, ALfloat *values)
{
#if LOG_SOURCE_USAGE
	DebugMessageN2("alGetSourcefv--> source %ld : %s", (long int) sid, GetALAttributeString(pname));
#endif

	try {
		OALSource		*oalSource = GetSourceObjectFromCurrentContext(sid);
        switch(pname) 
        {
			// Source ONLY Attributes
            case AL_MIN_GAIN:
                *values = oalSource->GetMinGain();
                break;
            case AL_MAX_GAIN:
                *values = oalSource->GetMaxGain();
                break;
            case AL_REFERENCE_DISTANCE:
                *values = oalSource->GetReferenceDistance();
                break;
            case AL_ROLLOFF_FACTOR:
                *values = oalSource->GetRollOffFactor();
                break;
            case AL_MAX_DISTANCE:
                *values = oalSource->GetMaxDistance();
                break;
            case AL_PITCH:
                *values = oalSource->GetPitch();
                break;
            case AL_DIRECTION:
                oalSource->GetDirection(values[0], values[1], values[2]);
                break;
            case AL_CONE_INNER_ANGLE:
                *values = oalSource->GetConeInnerAngle();
                break;
            case AL_CONE_OUTER_ANGLE:
                *values = oalSource->GetConeOuterAngle();
                break;
            case AL_CONE_OUTER_GAIN:
                *values = oalSource->GetConeOuterGain();
                break;

			// Source & Listener Attributes
            case AL_POSITION:
                oalSource->GetPosition(values[0], values[1], values[2]);
                break;
            case AL_VELOCITY:
                oalSource->GetVelocity(values[0], values[1], values[2]);
                break;
            case AL_GAIN:
                *values = oalSource->GetGain();
                break;

            default:
                alSetError(AL_INVALID_ENUM);
                break;
        }
	}
	catch (OSStatus      result) {
		DebugMessageN1("ERROR: alGetSourcefv FAILED = %s\n", alGetString(result));
		alSetError(result);
	}
    catch (...) {
		DebugMessage("ERROR: alGetSourcefv FAILED");
        alSetError(AL_INVALID_OPERATION);
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alGetSource3f (ALuint sid, ALenum pname, ALfloat *v1, ALfloat *v2, ALfloat *v3)
{
#if LOG_SOURCE_USAGE
	DebugMessageN2("alGetSource3f--> source %ld : %s", (long int) sid, GetALAttributeString(pname));
#endif

	try {
		OALSource		*oalSource = GetSourceObjectFromCurrentContext(sid);
        switch (pname) 
        {
			// Source ONLY Attributes
            case AL_DIRECTION:
                oalSource->GetDirection(*v1, *v2, *v3);
                break;

			// Source & Listener Attributes
            case AL_POSITION:
                oalSource->GetPosition(*v1, *v2, *v3);
                break;
            case AL_VELOCITY:
                oalSource->GetVelocity(*v1, *v2, *v3);
                break;

            default:
                alSetError(AL_INVALID_ENUM);
                break;
        }
	}
	catch (OSStatus      result) {
		DebugMessageN1("ERROR: alGetSource3f FAILED = %s\n", alGetString(result));
		alSetError(result);
	}
    catch (...) {
		DebugMessage("ERROR: alGetSource3f FAILED");
        alSetError(AL_INVALID_OPERATION);
	}
	
	return;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alGetSourcei (ALuint sid, ALenum pname, ALint *value)
{
#if LOG_SOURCE_USAGE
	DebugMessageN2("alGetSourcei--> source %ld : %s", (long int) sid, GetALAttributeString(pname));
#endif

	try {
		OALSource		*oalSource = GetSourceObjectFromCurrentContext(sid);
        switch (pname) 
        {
			// Source ONLY Attributes
            case AL_SOURCE_RELATIVE:
                *value = oalSource->GetSourceRelative();
                break;
            case AL_LOOPING:
                *value = oalSource->GetLooping();
                break;
            case AL_BUFFER:
                *value = oalSource->GetBuffer();
                break;
            case AL_BUFFERS_QUEUED:
                *value = oalSource->GetQLength();
                break;
            case AL_BUFFERS_PROCESSED:
                *value = oalSource->GetBuffersProcessed();
                break;
            case AL_REFERENCE_DISTANCE:
                *value = (ALint) oalSource->GetReferenceDistance();
                break;
            case AL_ROLLOFF_FACTOR:
                *value = (ALint) oalSource->GetRollOffFactor();
                break;
            case AL_MAX_DISTANCE:
                *value = (ALint) oalSource->GetMaxDistance();
                break;
            case AL_CONE_INNER_ANGLE:
                *value = (UInt32) oalSource->GetConeInnerAngle();
                break;
            case AL_CONE_OUTER_ANGLE:
                *value = (UInt32) oalSource->GetConeOuterAngle();
                break;
            case AL_CONE_OUTER_GAIN:
                *value = (UInt32) oalSource->GetConeOuterGain();
                break;
            case AL_SOURCE_STATE:
                *value = oalSource->GetState();
                break;
            
			default:
                alSetError(AL_INVALID_ENUM);
                break;
        }
	}
	catch (OSStatus      result) {
		DebugMessageN1("ERROR: alGetSourcei FAILED = %s\n", alGetString(result));
		alSetError(result);
	}
    catch (...) {
		DebugMessage("ERROR: alGetSourcei FAILED");
        alSetError(AL_INVALID_OPERATION);
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alSourcePlay(ALuint sid)
{	
#if LOG_SOURCE_USAGE
	DebugMessageN1("alSourcePlay--> source %ld", (long int) sid);
#endif

	try {
		OALSource		*oalSource = GetSourceObjectFromCurrentContext(sid);
		oalSource->Play();					// start playing the queue
	}
	catch (OSStatus      result) {
		DebugMessageN1("ERROR: alSourcePlay FAILED = %s\n", alGetString(result));
		alSetError(result);
	}
    catch (...) {
		DebugMessage("ERROR: alSourcePlay FAILED");
        alSetError(AL_INVALID_OPERATION);
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alSourcePause (ALuint sid)
{
#if LOG_SOURCE_USAGE
	DebugMessageN1("alSourcePause--> source %ld", (long int) sid);
#endif

	try {
		OALSource		*oalSource = GetSourceObjectFromCurrentContext(sid);
        oalSource->Pause();
	}
	catch (OSStatus      result) {
		DebugMessageN1("ERROR: alSourcePause FAILED = %s\n", alGetString(result));
		alSetError(result);
	}
    catch (...) {
		DebugMessage("ERROR: alSourcePause FAILED");
        alSetError(AL_INVALID_OPERATION);
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alSourceStop (ALuint sid)
{
#if LOG_SOURCE_USAGE
	DebugMessageN1("alSourceStop--> source %ld", (long int) sid);
#endif

	try {
		OALSource		*oalSource = GetSourceObjectFromCurrentContext(sid);
        oalSource->Stop();
	}
	catch (OSStatus      result) {
		DebugMessageN1("ERROR: alSourceStop FAILED = %s\n", alGetString(result));
		alSetError(result);
	}
    catch (...) {
		DebugMessage("ERROR: alSourceStop FAILED");
        alSetError(AL_INVALID_OPERATION);
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alSourceRewind (ALuint sid)
{
#if LOG_SOURCE_USAGE
	DebugMessageN1("alSourceRewind--> source %ld", (long int) sid);
#endif

	try {
		OALSource		*oalSource = GetSourceObjectFromCurrentContext(sid);
        oalSource->Rewind();
	}
	catch (OSStatus      result) {
		DebugMessageN1("ERROR: alSourceRewind FAILED = %s\n", alGetString(result));
		alSetError(result);
	}
    catch (...) {
		DebugMessage("ERROR: alSourceRewind FAILED");
        alSetError(AL_INVALID_OPERATION);
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid	ALAPIENTRY alSourcePlayv( ALsizei n, ALuint *sources )
{
#if LOG_SOURCE_USAGE
	DebugMessage("alSourcePlayv--> sources = ");
	for (UInt32	i = 0; i < (UInt32) n; i++) {
		printf("%ld ", (long int) sources[i]);
	}
	printf("\n");
#endif

	try {
        for (UInt32	i = 0; i < (UInt32) n; i++)
            alSourcePlay(sources[i]);
    }
	catch (OSStatus      result) {
		DebugMessageN1("ERROR: alSourcePlayv FAILED = %s\n", alGetString(result));
		alSetError(result);
	}
    catch (...) {
		DebugMessage("ERROR: alSourcePlayv FAILED");
        alSetError(AL_INVALID_OPERATION);
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid	ALAPIENTRY alSourcePausev( ALsizei n, ALuint *sources )
{
#if LOG_SOURCE_USAGE
	DebugMessage("alSourcePausev--> sources = ");
	for (UInt32	i = 0; i < (UInt32) n; i++) {
		printf("%ld ", (long int) sources[i]);
	}
	printf("\n");
#endif

	try {
        for (UInt32	i = 0; i < (UInt32) n; i++)
            alSourcePause(sources[i]);
    }
	catch (OSStatus      result) {
		DebugMessageN1("ERROR: alSourcePausev FAILED = %s\n", alGetString(result));
		alSetError(result);
	}
    catch (...) {
		DebugMessage("ERROR: alSourcePausev FAILED");
        alSetError(AL_INVALID_OPERATION);
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid	ALAPIENTRY alSourceStopv( ALsizei n, ALuint *sources )
{
#if LOG_SOURCE_USAGE
	DebugMessage("alSourceStopv--> sources = ");
	for (UInt32	i = 0; i < (UInt32) n; i++) {
		printf("%ld ", (long int) sources[i]);
	}
	printf("\n");
#endif

	try {
        for (UInt32	i = 0; i < (UInt32) n; i++)
            alSourceStop(sources[i]);
    }
	catch (OSStatus      result) {
		DebugMessageN1("ERROR: alSourceStopv FAILED = %s\n", alGetString(result));
		alSetError(result);
	}
    catch (...) {
		DebugMessage("ERROR: alSourceStopv FAILED");
        alSetError(AL_INVALID_OPERATION);
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid	ALAPIENTRY alSourceRewindv(ALsizei n, ALuint *sources)
{
#if LOG_SOURCE_USAGE
	DebugMessage("alSourceRewindv--> sources = ");
	for (UInt32	i = 0; i < (UInt32) n; i++) {
		printf("%ld ", (long int) sources[i]);
	}
	printf("\n");
#endif

	try {
        for (UInt32	i = 0; i < (UInt32) n; i++)
            alSourceRewind(sources[i]);
    }
	catch (OSStatus      result) {
		DebugMessageN1("ERROR: alSourceRewindv FAILED = %s\n", alGetString(result));
		alSetError(result);
	}
    catch (...) {
		DebugMessage("ERROR: alSourceRewindv FAILED");
        alSetError(AL_INVALID_OPERATION);
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid	ALAPIENTRY alSourceQueueBuffers( ALuint source, ALsizei n, ALuint* buffers )
{
#if LOG_SOURCE_USAGE
	DebugMessageN2("alSourceQueueBuffers--> source %ld : numEntries = %ld", (long int) source, (long int) n);
#endif

#if LOG_BUFFER_USAGE
	printf("alSourceQueueBuffers--> (%ld) ", (long int) n);
	for (UInt32	i = 0; i < (UInt32) n; i++) {
		printf("%ld, ", (long int) buffers[i]);
	}
	printf("\n");
#endif
	

	if (n == 0)
		return;	// no buffers were actually requested for queueing
		
	try {
        if (gOALBufferMap == NULL)
            throw ((OSStatus) AL_INVALID_OPERATION);   

		OALSource		*oalSource = GetSourceObjectFromCurrentContext(source);
                        
		// The source type must first be checked for static or streaming
		////if (oalSource->GetSourceType() == AL_STATIC)
        ////    throw ((OSStatus) AL_INVALID_OPERATION);
		
        // verify that buffers provided are valid before queueing them.
        for (UInt32	i = 0; i < (UInt32) n; i++)
        {
            if (buffers[i] != AL_NONE)
            {
                // verify that this is a valid buffer
                OALBuffer *oalBuffer = gOALBufferMap->Get(buffers[i]);
                if (oalBuffer == NULL)
                    throw ((OSStatus) AL_INVALID_VALUE);				// an invalid buffer id has been provided
            }
            else
            {
                // NONE is a valid buffer parameter - do nothing it's valid
            }
        }

        // all valid buffers, so append them to the queue
        for (UInt32	i = 0; i < (UInt32) n; i++)
        {
			oalSource->AddToQueue(buffers[i], gOALBufferMap->Get(buffers[i]));
        }
	}
	catch (OSStatus		result) {
		DebugMessageN1("ERROR: alSourceQueueBuffers FAILED = %s\n", alGetString(result));
		alSetError(result);
	}
	catch (...) {
		DebugMessage("ERROR: alSourceQueueBuffers FAILED");
        alSetError(AL_INVALID_OPERATION);
	}

	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alSourceUnqueueBuffers (ALuint sid, ALsizei n, ALuint *buffers)
{
#if LOG_SOURCE_USAGE
	DebugMessageN2("alSourceUnqueueBuffers--> source %ld : count = %ld", (long int) sid, (long int) n);
#endif

	if (n == 0)
		return;

	try {
        if (buffers == NULL)
            throw ((OSStatus) AL_INVALID_VALUE);   

        if (gOALBufferMap == NULL)
            throw ((OSStatus) AL_INVALID_OPERATION);   

		OALSource		*oalSource = GetSourceObjectFromCurrentContext(sid);

        if (oalSource->GetQLength() < (UInt32) n)
            throw (OSStatus) AL_INVALID_VALUE;				// n is greater than the source's Q length
        
        oalSource->RemoveBuffersFromQueue(n, buffers);

	}
	catch (OSStatus		result) {
		DebugMessageN1("ERROR: alSourceUnqueueBuffers FAILED = %s\n", alGetString(result));
		// reinitialize the elements in the buffers array
		if (buffers)
		{
			for (UInt32	i = 0; i < (UInt32) n; i++)
				buffers[i] = 0;
		}
		// this would be real bad, as now we have a buffer queue in an unknown state
		alSetError(result);
	}
	catch (...){
		DebugMessage("ERROR: alSourceUnqueueBuffers FAILED");
        alSetError(AL_INVALID_OPERATION);
	}

#if LOG_BUFFER_USAGE
	printf("alSourceUnqueueBuffers--> (%ld) ", (long int) n);
	for (UInt32	i = 0; i < (UInt32) n; i++) {
		printf("%ld, ", (long int) buffers[i]);
	}
	printf("\n");
#endif

	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ***** Listeners *****
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alListenerf (ALenum pname, ALfloat value)
{
#if LOG_API_USAGE
	DebugMessageN2("alListenerf--> attribute = %s : value %.f2", GetALAttributeString(pname), value);
#endif

	try {
        OALContext		*oalContext = GetContextObjectByToken(gCurrentContext);
        switch (pname) 
        {
            case AL_GAIN:
				oalContext->SetListenerGain((Float32) value);     //gListener.Gain=value;
                break;
            
			default:
                alSetError(AL_INVALID_ENUM);
                break;
        }
	}
	catch (OSStatus		result) {
 		DebugMessageN1("ERROR: alListenerf FAILED = %s\n", alGetString(result));
        alSetError(result);
    }
	catch (...) {
 		DebugMessage("ERROR: alListenerf FAILED");
        alSetError(AL_INVALID_OPERATION);   // by default
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alListenerfv( ALenum param, ALfloat* values )
{
#if LOG_API_USAGE
	DebugMessageN1("alListenerfv--> attribute = %s", GetALAttributeString(param));
#endif

	try {
        OALContext		*oalContext = GetContextObjectByToken(gCurrentContext);
        switch(param) 
        {
            case AL_POSITION:
                oalContext->SetListenerPosition(values[0], values[1], values[2]);
                break;
            case AL_VELOCITY:
                oalContext->SetListenerVelocity(values[0], values[1], values[2]);
                break;
            case AL_GAIN:
                oalContext->SetListenerGain((Float32) *values);     //gListener.Gain=value;
                break;
            case AL_ORIENTATION:
                oalContext->SetListenerOrientation(	values[0], values[1], values[2],
                                                    values[3], values[4], values[5]);
                break;
            default:
                alSetError(AL_INVALID_ENUM);
                break;
        }
	}
	catch (OSStatus		result) {
 		DebugMessageN1("ERROR: alListenerfv FAILED = %s\n", alGetString(result));
       alSetError(result);
    }
	catch (...) {
 		DebugMessage("ERROR: alListenerfv FAILED");
        alSetError(AL_INVALID_OPERATION);   // by default
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alListener3f (ALenum pname, ALfloat v1, ALfloat v2, ALfloat v3)
{
#if LOG_API_USAGE
	DebugMessageN4("alListener3f--> attribute = %s : %.f2 : %.f2 : %.f2", GetALAttributeString(pname), v1, v2, v3);
#endif

	try {
        OALContext		*oalContext = GetContextObjectByToken(gCurrentContext);
        switch (pname) 
        {
            case AL_POSITION:
                oalContext->SetListenerPosition(v1, v2, v3);
                break;
            case AL_VELOCITY:
                oalContext->SetListenerVelocity(v1, v2, v3);
                break;
            default:
                alSetError(AL_INVALID_ENUM);
                break;
        }
	}
	catch (OSStatus		result) {
		DebugMessageN1("ERROR: alListener3f FAILED = %s\n", alGetString(result));
        alSetError(result);
    }
	catch (...) {
		DebugMessage("ERROR: alListener3f FAILED");
        alSetError(AL_INVALID_OPERATION);   // by default
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alListeneri (ALenum pname, ALint value)
{		
#if LOG_API_USAGE
	DebugMessage("***** alListeneri");
#endif
	alSetError(AL_INVALID_ENUM);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alGetListenerf( ALenum pname, ALfloat* value )
{
#if LOG_API_USAGE
	DebugMessageN1("alGetListenerf--> attribute = %s", GetALAttributeString(pname));
#endif

	try {
        OALContext		*oalContext = GetContextObjectByToken(gCurrentContext);
        switch(pname) 
        {
            case AL_GAIN:
                *value = oalContext->GetListenerGain();
                break;
            default:
                alSetError(AL_INVALID_ENUM);
                break;
        }
	}
	catch (OSStatus		result) {
		DebugMessageN1("ERROR: alGetListenerf FAILED = %s\n", alGetString(result));
        alSetError(result);
    }
	catch (...) {
		DebugMessage("ERROR: alGetListenerf FAILED");
        alSetError(AL_INVALID_OPERATION);   // by default
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alGetListenerfv( ALenum pname, ALfloat* values )
{
#if LOG_API_USAGE
	DebugMessageN1("alGetListenerfv--> attribute = %s", GetALAttributeString(pname));
#endif

	try {
        OALContext		*oalContext = GetContextObjectByToken(gCurrentContext);
        switch (pname) 
        {
            case AL_POSITION:
                oalContext->GetListenerPosition(&values[0], &values[1], &values[2]);
                break;
            case AL_VELOCITY:
                oalContext->GetListenerVelocity(&values[0], &values[1], &values[2]);
                break;
            case AL_GAIN:
                *values = oalContext->GetListenerGain();
                break;
            case AL_ORIENTATION:
                oalContext->GetListenerOrientation( &values[0], &values[1], &values[2],
                                                    &values[3], &values[4], &values[5]);
                break;
            default:
                alSetError(AL_INVALID_ENUM);
                break;
        }
	}
	catch (OSStatus		result) {
		DebugMessageN1("ERROR: alGetListenerfv FAILED = %s\n", alGetString(result));
        alSetError(result);
    }
	catch (...) {
		DebugMessage("ERROR: alGetListenerfv FAILED");
        alSetError(AL_INVALID_OPERATION);   // by default
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alGetListener3f( ALenum pname, ALfloat* v1, ALfloat* v2, ALfloat* v3 )
{
#if LOG_API_USAGE
	DebugMessageN1("alGetListener3f--> attribute = %s", GetALAttributeString(pname));
#endif

	try {
        OALContext		*oalContext = GetContextObjectByToken(gCurrentContext);
        switch(pname) 
        {
            case AL_POSITION:
                oalContext->GetListenerPosition(v1, v2, v3);
                break;
            case AL_VELOCITY:
                oalContext->GetListenerVelocity(v1, v2, v3);
                break;
            default:
                alSetError(AL_INVALID_ENUM);
                break;
        }
	}
	catch (OSStatus		result) {
		DebugMessageN1("ERROR: alGetListener3f FAILED = %s\n", alGetString(result));
        alSetError(result);
    }
	catch (...) {
		DebugMessage("ERROR: alGetListener3f FAILED");
        alSetError(AL_INVALID_OPERATION);   // by default
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alGetListeneri( ALenum pname, ALint* value )
{
#if LOG_API_USAGE
	DebugMessageN1("alGetListeneri--> attribute = %s", GetALAttributeString(pname));
#endif

	*value = 0;

	try {
		switch (pname)
		{
			default:
				alSetError(AL_INVALID_VALUE);
				break;
		}
	}
	catch (...) {
 		DebugMessage("ERROR: alGetListeneri FAILED");
        alSetError(AL_INVALID_OPERATION); // not available yet as the device is not setup
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark ***** Global Settings *****
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY	alDistanceModel (ALenum value)
{
#if LOG_API_USAGE
	DebugMessageN1("alDistanceModel--> model = %s", GetALAttributeString(value));
#endif

	try {
        switch (value)
        {
            case AL_NONE:			
            case AL_INVERSE_DISTANCE:
            case AL_INVERSE_DISTANCE_CLAMPED:
            {
				OALContext		*oalContext = GetContextObjectByToken(gCurrentContext);
				oalContext->SetDistanceModel(value);
            } 
            break;
            
            default:
                alSetError(AL_INVALID_VALUE);
                break;
        }
    }
	catch (OSStatus		result) {
		DebugMessageN1("ERROR: alDistanceModel FAILED = %s\n", alGetString(result));
        alSetError(result);
    }
    catch (...) {
		DebugMessage("ERROR: alDistanceModel FAILED");
        alSetError(AL_INVALID_OPERATION);   // by default
    }
    return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alDopplerFactor (ALfloat value)
{	
#if LOG_API_USAGE
	DebugMessageN1("alDopplerFactor---> setting = %.f2", value);
#endif

	try {
        if (value < 0.0f)
            throw ((OSStatus) AL_INVALID_VALUE);

        OALContext		*oalContext = GetContextObjectByToken(gCurrentContext);
		oalContext->SetDopplerFactor(value);
	}
	catch (OSStatus		result) {
		DebugMessageN1("ERROR: alDopplerFactor FAILED = %s\n", alGetString(result));
        alSetError(result);
    }
    catch (...) {
		DebugMessage("ERROR: alDopplerFactor FAILED");
        alSetError(AL_INVALID_OPERATION);   // by default
    }
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alDopplerVelocity (ALfloat value)
{
#if LOG_API_USAGE
	DebugMessageN1("alDopplerVelocity---> setting = %.f2", value);
#endif

	try {
        if (value <= 0.0f)
            throw ((OSStatus) AL_INVALID_VALUE);

        OALContext		*oalContext = GetContextObjectByToken(gCurrentContext);
		oalContext->SetDopplerVelocity(value);
	}
	catch (OSStatus		result) {
		DebugMessageN1("ERROR: alDopplerVelocity FAILED = %s\n", alGetString(result));
        alSetError(result);
    }
    catch (...) {
		DebugMessage("ERROR: alDopplerVelocity FAILED");
        alSetError(AL_INVALID_OPERATION);   // by default
    }
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALubyte*	ALAPIENTRY alGetString( ALenum param )
{
#if LOG_API_USAGE
	DebugMessageN1("alGetString = %s", GetALAttributeString(param));
#endif

	switch (param)
	{
		case AL_VENDOR:
			return (ALubyte *)alVendor;
		case AL_VERSION:
			return (ALubyte *)alVersion;
		case AL_RENDERER:
			return (ALubyte *)alRenderer;
		case AL_EXTENSIONS:
			return (ALubyte *)alExtensions;
		case AL_NO_ERROR:
			return (ALubyte *)alNoError;
		case AL_INVALID_NAME:
			return (ALubyte *)alErrInvalidName;
		case AL_INVALID_ENUM:
			return (ALubyte *)alErrInvalidEnum;
		case AL_INVALID_VALUE:
			return (ALubyte *)alErrInvalidValue;
		case AL_INVALID_OPERATION:
			return (ALubyte *)alErrInvalidOp;
		case AL_OUT_OF_MEMORY:
			return (ALubyte *)alErrOutOfMemory;
		case -1:
			return (ALubyte *)unknownImplementationError;
		default:
			alSetError(AL_INVALID_ENUM);
			break;
	}
	return NULL;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALenum	ALAPIENTRY alGetEnumValue( ALubyte* ename )
{
#if LOG_API_USAGE
	DebugMessageN1("alGetEnumValue: %s", ename);
#endif

	// AL
	if (strcmp("AL_INVALID", (const char *)ename) == 0) { return AL_INVALID; }
	if (strcmp("AL_NONE", (const char *)ename) == 0) { return AL_NONE; }
	if (strcmp("AL_FALSE", (const char *)ename) == 0) { return AL_FALSE; }
	if (strcmp("AL_TRUE", (const char *)ename) == 0) { return AL_TRUE; }
	if (strcmp("AL_SOURCE_RELATIVE", (const char *)ename) == 0) { return AL_SOURCE_RELATIVE; }
	if (strcmp("AL_CONE_INNER_ANGLE", (const char *)ename) == 0) { return AL_CONE_INNER_ANGLE; }
	if (strcmp("AL_CONE_OUTER_ANGLE", (const char *)ename) == 0) { return AL_CONE_OUTER_ANGLE; }
	if (strcmp("AL_CONE_OUTER_GAIN", (const char *)ename) == 0) { return AL_CONE_OUTER_GAIN; }
	if (strcmp("AL_PITCH", (const char *)ename) == 0) { return AL_PITCH; }
	if (strcmp("AL_POSITION", (const char *)ename) == 0) { return AL_POSITION; }
	if (strcmp("AL_DIRECTION", (const char *)ename) == 0) { return AL_DIRECTION; }
	if (strcmp("AL_VELOCITY", (const char *)ename) == 0) { return AL_VELOCITY; }
	if (strcmp("AL_LOOPING", (const char *)ename) == 0) { return AL_LOOPING; }
	if (strcmp("AL_BUFFER", (const char *)ename) == 0) { return AL_BUFFER; }
	if (strcmp("AL_GAIN", (const char *)ename) == 0) { return AL_GAIN; }
	if (strcmp("AL_MIN_GAIN", (const char *)ename) == 0) { return AL_MIN_GAIN; }
	if (strcmp("AL_MAX_GAIN", (const char *)ename) == 0) { return AL_MAX_GAIN; }
	if (strcmp("AL_ORIENTATION", (const char *)ename) == 0) { return AL_ORIENTATION; }
	if (strcmp("AL_REFERENCE_DISTANCE", (const char *)ename) == 0) { return AL_REFERENCE_DISTANCE; }
	if (strcmp("AL_ROLLOFF_FACTOR", (const char *)ename) == 0) { return AL_ROLLOFF_FACTOR; }
	if (strcmp("AL_MAX_DISTANCE", (const char *)ename) == 0) { return AL_MAX_DISTANCE; }
	if (strcmp("AL_SOURCE_STATE", (const char *)ename) == 0) { return AL_SOURCE_STATE; }
	if (strcmp("AL_INITIAL", (const char *)ename) == 0) { return AL_INITIAL; }
	if (strcmp("AL_PLAYING", (const char *)ename) == 0) { return AL_PLAYING; }
	if (strcmp("AL_PAUSED", (const char *)ename) == 0) { return AL_PAUSED; }
	if (strcmp("AL_STOPPED", (const char *)ename) == 0) { return AL_STOPPED; }
	if (strcmp("AL_BUFFERS_QUEUED", (const char *)ename) == 0) { return AL_BUFFERS_QUEUED; }
	if (strcmp("AL_BUFFERS_PROCESSED", (const char *)ename) == 0) { return AL_BUFFERS_PROCESSED; }
	if (strcmp("AL_FORMAT_MONO8", (const char *)ename) == 0) { return AL_FORMAT_MONO8; }
	if (strcmp("AL_FORMAT_MONO16", (const char *)ename) == 0) { return AL_FORMAT_MONO16; }
	if (strcmp("AL_FORMAT_STEREO8", (const char *)ename) == 0) { return AL_FORMAT_STEREO8; }
	if (strcmp("AL_FORMAT_STEREO16", (const char *)ename) == 0) { return AL_FORMAT_STEREO16; }
	if (strcmp("AL_FREQUENCY", (const char *)ename) == 0) { return AL_FREQUENCY; }
	if (strcmp("AL_SIZE", (const char *)ename) == 0) { return AL_SIZE; }
	if (strcmp("AL_UNUSED", (const char *)ename) == 0) { return AL_UNUSED; }
	if (strcmp("AL_PENDING", (const char *)ename) == 0) { return AL_PENDING; }
	if (strcmp("AL_PROCESSED", (const char *)ename) == 0) { return AL_PROCESSED; }
	if (strcmp("AL_NO_ERROR", (const char *)ename) == 0) { return AL_NO_ERROR; }
	if (strcmp("AL_INVALID_NAME", (const char *)ename) == 0) { return AL_INVALID_NAME; }
	if (strcmp("AL_INVALID_ENUM", (const char *)ename) == 0) { return AL_INVALID_ENUM; }
	if (strcmp("AL_INVALID_VALUE", (const char *)ename) == 0) { return AL_INVALID_VALUE; }
	if (strcmp("AL_INVALID_OPERATION", (const char *)ename) == 0) { return AL_INVALID_OPERATION; }
	if (strcmp("AL_OUT_OF_MEMORY", (const char *)ename) == 0) { return AL_OUT_OF_MEMORY; }
	if (strcmp("AL_VENDOR", (const char *)ename) == 0) { return AL_VENDOR; }
	if (strcmp("AL_VERSION", (const char *)ename) == 0) { return AL_VERSION; }
	if (strcmp("AL_RENDERER", (const char *)ename) == 0) { return AL_RENDERER; }
	if (strcmp("AL_EXTENSIONS", (const char *)ename) == 0) { return AL_EXTENSIONS; }
	if (strcmp("AL_DOPPLER_FACTOR", (const char *)ename) == 0) { return AL_DOPPLER_FACTOR; }
	if (strcmp("AL_DOPPLER_VELOCITY", (const char *)ename) == 0) { return AL_DOPPLER_VELOCITY; }
	if (strcmp("AL_DISTANCE_MODEL", (const char *)ename) == 0) { return AL_DISTANCE_MODEL; }
	if (strcmp("AL_INVERSE_DISTANCE", (const char *)ename) == 0) { return AL_INVERSE_DISTANCE; }
	if (strcmp("AL_INVERSE_DISTANCE_CLAMPED", (const char *)ename) == 0) { return AL_INVERSE_DISTANCE_CLAMPED; }
	// ALC
	if (strcmp("ALC_INVALID", (const char *)ename) == 0) { return ALC_INVALID; }
	if (strcmp("ALC_FALSE", (const char *)ename) == 0) { return ALC_FALSE; }
	if (strcmp("ALC_TRUE", (const char *)ename) == 0) { return ALC_TRUE; }
	if (strcmp("ALC_MAJOR_VERSION", (const char *)ename) == 0) { return ALC_MAJOR_VERSION; }
	if (strcmp("ALC_MINOR_VERSION", (const char *)ename) == 0) { return ALC_MINOR_VERSION; }
	if (strcmp("ALC_ATTRIBUTES_SIZE", (const char *)ename) == 0) { return ALC_ATTRIBUTES_SIZE; }
	if (strcmp("ALC_ALL_ATTRIBUTES", (const char *)ename) == 0) { return ALC_ALL_ATTRIBUTES; }
	if (strcmp("ALC_DEFAULT_DEVICE_SPECIFIER", (const char *)ename) == 0) { return ALC_DEFAULT_DEVICE_SPECIFIER; }
	if (strcmp("ALC_DEVICE_SPECIFIER", (const char *)ename) == 0) { return ALC_DEVICE_SPECIFIER; }
	if (strcmp("ALC_EXTENSIONS", (const char *)ename) == 0) { return ALC_EXTENSIONS; }
	if (strcmp("ALC_FREQUENCY", (const char *)ename) == 0) { return ALC_FREQUENCY; }
	if (strcmp("ALC_REFRESH", (const char *)ename) == 0) { return ALC_REFRESH; }
	if (strcmp("ALC_SYNC", (const char *)ename) == 0) { return ALC_SYNC; }
	if (strcmp("ALC_NO_ERROR", (const char *)ename) == 0) { return ALC_NO_ERROR; }
	if (strcmp("ALC_INVALID_DEVICE", (const char *)ename) == 0) { return ALC_INVALID_DEVICE; }
	if (strcmp("ALC_INVALID_CONTEXT", (const char *)ename) == 0) { return ALC_INVALID_CONTEXT; }
	if (strcmp("ALC_INVALID_ENUM", (const char *)ename) == 0) { return ALC_INVALID_ENUM; }
	if (strcmp("ALC_INVALID_VALUE", (const char *)ename) == 0) { return ALC_INVALID_VALUE; }
	if (strcmp("ALC_OUT_OF_MEMORY", (const char *)ename) == 0) { return ALC_OUT_OF_MEMORY; }
	
	return -1;
}
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALboolean ALAPIENTRY alGetBoolean (ALenum pname)
{
#if LOG_API_USAGE
	DebugMessage("***** alGetBoolean");
#endif
	return AL_FALSE;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alGetBooleanv (ALenum pname, ALboolean *data)
{
#if LOG_API_USAGE
	DebugMessage("***** alGetBooleanv");
#endif
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALfloat ALAPIENTRY alGetFloat (ALenum pname)
{
	Float32			returnValue = 0.0f;

#if LOG_API_USAGE
	DebugMessageN1("alGetFloat ---> attribute = %s", GetALAttributeString(pname));
#endif

	try {
        OALContext		*oalContext = GetContextObjectByToken(gCurrentContext);
        switch (pname)
        {
            case AL_DOPPLER_FACTOR:
                returnValue = oalContext->GetDopplerFactor();
                break;
            case AL_DOPPLER_VELOCITY:
                returnValue = oalContext->GetDopplerVelocity();
                break;
            default:
                break;
        }
	}
	catch (OSStatus		result) {
 		DebugMessageN1("ERROR: alGetFloat FAILED = %s\n", alGetString(result));
        alSetError(result);
    }
	catch (...) {
 		DebugMessage("ERROR: alGetFloat FAILED");
        alSetError(AL_INVALID_OPERATION);   // by default
	}
	
	return (returnValue);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alGetFloatv (ALenum pname, ALfloat *data)
{
#if LOG_API_USAGE
	DebugMessageN1("alGetFloatv ---> attribute = %s", GetALAttributeString(pname));
#endif

	try {
        OALContext		*oalContext = GetContextObjectByToken(gCurrentContext);
        switch(pname)
        {
            case AL_DOPPLER_FACTOR:
                *data = oalContext->GetDopplerFactor();
                break;
            case AL_DOPPLER_VELOCITY:
                *data = oalContext->GetDopplerVelocity();
                break;
            default:
                alSetError(AL_INVALID_ENUM);
                break;
        }
	}
	catch (OSStatus		result) {
 		DebugMessageN1("ERROR: alGetFloatv FAILED = %s\n", alGetString(result));
        alSetError(result);
    }
	catch (...) {
 		DebugMessage("ERROR: alGetFloatv FAILED");
        alSetError(AL_INVALID_OPERATION);   // by default
	}
	
    return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALdouble ALAPIENTRY alGetDouble (ALenum pname)
{
#if LOG_API_USAGE
	DebugMessageN1("alGetDouble ---> attribute = %s", GetALCAttributeString(pname));
#endif

    double      returnValue = 0.0;

	try {
		switch (pname)
		{			
			case ALC_MIXER_OUTPUT_RATE:
				OALDevice	*oalDevice = GetDeviceObjectByToken (gCurrentDevice);
				returnValue = (ALdouble) oalDevice->GetMixerRate();
				break;
			
			default:
				alSetError(AL_INVALID_VALUE);
				break;
		}
	}
	catch (OSStatus		result) {
 		DebugMessageN1("ERROR: alGetDouble FAILED = %s\n", alGetString(result));
        alSetError(result);
    }
	catch (...) {
 		DebugMessage("ERROR: alGetDouble FAILED");
        alSetError(AL_INVALID_OPERATION);   // by default
	}
	
	return (returnValue);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alGetDoublev (ALenum pname, ALdouble *data)
{
#if LOG_API_USAGE
	DebugMessageN1("alGetDoublev ---> attribute = %s", GetALCAttributeString(pname));
#endif

	try {
		switch (pname)
		{			
			case ALC_MIXER_OUTPUT_RATE:
				OALDevice	*oalDevice = GetDeviceObjectByToken (gCurrentDevice);
				*data = (ALdouble) oalDevice->GetMixerRate();
				break;
			
			default:
				alSetError(AL_INVALID_VALUE);
				break;
		}
	}
	catch (OSStatus		result) {
 		DebugMessageN1("ERROR: alGetDoublev FAILED = %s\n", alGetString(result));
        alSetError(result);
    }
	catch (...) {
 		DebugMessage("ERROR: alGetDoublev FAILED");
        alSetError(AL_INVALID_OPERATION);   // by default
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALint ALAPIENTRY alGetInteger (ALenum pname)
{
#if LOG_API_USAGE
	DebugMessageN1("alGetInteger ---> attribute = 0x%X", pname);
#endif

	UInt32			returnValue	= 0;
	OALContext		*oalContext = NULL;
	
	try {
		switch (pname)
		{
            case AL_DISTANCE_MODEL:
				oalContext = GetContextObjectByToken(gCurrentContext);
				returnValue = oalContext->GetDistanceModel();
                break;

			case ALC_SPATIAL_RENDERING_QUALITY:
				oalContext = GetContextObjectByToken(gCurrentContext);
				returnValue = oalContext->GetRenderQuality();
				break;

			case ALC_RENDER_CHANNEL_COUNT:
				OALDevice* oalDevice = GetDeviceObjectByToken (gCurrentDevice);
				returnValue = oalDevice->GetRenderChannelCount();
				break;
                
            case ALC_MIXER_MAXIMUM_BUSSES:
				oalContext = GetContextObjectByToken(gCurrentContext);
				if (oalContext)
					returnValue = oalContext->GetBusCount();
				else
					returnValue = gMaximumMixerBusCount;
                break;
			
			default:
				alSetError(AL_INVALID_VALUE);
				break;
		}
	}
	catch (OSStatus		result) {
 		DebugMessageN1("ERROR: alGetInteger FAILED = %s\n", alGetString(result));
    }
	catch (...) {
 		DebugMessage("ERROR: alGetInteger FAILED");
	}
	
	return (returnValue);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alGetIntegerv (ALenum pname, ALint *data)
{
#if LOG_API_USAGE
	DebugMessageN1("alGetIntegerv ---> attribute = 0x%X", pname);
#endif

	try {
		OALContext		*oalContext = NULL;
        switch (pname)
        {
            case AL_DISTANCE_MODEL:
				oalContext = GetContextObjectByToken(gCurrentContext);
				*data = oalContext->GetDistanceModel();
                break;
                
			case ALC_SPATIAL_RENDERING_QUALITY:
				oalContext = GetContextObjectByToken(gCurrentContext);
				*data = oalContext->GetRenderQuality();
				break;

			case ALC_RENDER_CHANNEL_COUNT:
				OALDevice* oalDevice = GetDeviceObjectByToken (gCurrentDevice);
				*data = oalDevice->GetRenderChannelCount();
				break;
                
            case ALC_MIXER_MAXIMUM_BUSSES:
				oalContext = GetContextObjectByToken(gCurrentContext);
				if (oalContext)
					*data = oalContext->GetBusCount();
				else
					*data = gMaximumMixerBusCount;
                break;

            default:
                alSetError(AL_INVALID_ENUM);
                break;
        }
	}
	catch (OSStatus     result) {
		DebugMessageN1("ERROR: alGetIntegerv FAILED = %s\n", alGetString(result));
        alSetError(result);
    }
    catch (...) {
		DebugMessage("ERROR: alGetIntegerv FAILED");
        alSetError(AL_INVALID_OPERATION);
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid*	ALAPIENTRY alGetProcAddress( ALubyte* fname )
{
#if LOG_API_USAGE
	DebugMessageN1("alGetProcAddress function name = %s", fname);
#endif

	if (fname == NULL)
		SetDeviceError(gCurrentDevice, ALC_INVALID_VALUE);
	else
	{
	}

	return NULL;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALboolean ALAPIENTRY alIsEnvironmentIASIG (ALuint environment)
{
#if LOG_API_USAGE
	DebugMessage("***** alIsEnvironmentIASIG");
#endif
	return AL_FALSE;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALboolean ALAPIENTRY alIsExtensionPresent( ALubyte* fname )
{
#if LOG_API_USAGE
	DebugMessage("alIsExtensionPresent called");
#endif

    return AL_FALSE;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alDisable (ALenum capability)
{
#if LOG_API_USAGE
	DebugMessageN1("alDisable--> capability = 0x%X", capability);
#endif
	switch (capability)
	{
		case ALC_CONVERT_DATA_UPON_LOADING:
			gConvertBufferNow = false;
			break;
		default:
			alSetError(AL_INVALID_VALUE);
			break;
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alEnable (ALenum capability)
{
#if LOG_API_USAGE
	DebugMessageN1("alEnable--> capability = 0x%X", capability);
#endif
	switch(capability)
	{
		case ALC_CONVERT_DATA_UPON_LOADING:
			gConvertBufferNow = true;
			break;
		default:
			alSetError(AL_INVALID_VALUE);
			break;
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALboolean ALAPIENTRY alIsEnabled(ALenum capability)
{
#if LOG_API_USAGE
	DebugMessageN1("alIsEnabled--> capability = 0x%X", capability);
#endif
	switch(capability)
	{
		case ALC_CONVERT_DATA_UPON_LOADING:
			return (gConvertBufferNow);
			break;
		default:
			break;
	}
	return (false);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI void	ALAPIENTRY alHint( ALenum target, ALenum mode )
{
	// Removed from headers for 1.1 but left in to avoid runtime link errors
	// Prototype has been removed from the public headers
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Never actually in official OpenAL headers because no tokens were defined to use it
// keep building for 1.0 released binary compatibility
ALAPI ALvoid ALAPIENTRY alSetInteger (ALenum pname, const ALint value)
{	
#if LOG_API_USAGE
	DebugMessage("***** alSetIntegeri");
#endif
	try {
		switch(pname)
		{			
			case ALC_SPATIAL_RENDERING_QUALITY:
				if (IsValidRenderQuality ((UInt32) value))
				{
					OALContext		*oalContext = gOALContextMap->Get(gCurrentContext);
					if (oalContext)
						oalContext->SetRenderQuality ((UInt32) value);
				}
				break;

			case ALC_RENDER_CHANNEL_COUNT:
				OALDevice	*oalDevice = GetDeviceObjectByToken (gCurrentDevice);
				oalDevice->SetRenderChannelCount ((UInt32) value);
				break;

            case ALC_MIXER_MAXIMUM_BUSSES:
                gMaximumMixerBusCount = value;
                break;
			
			default:
				alSetError(AL_INVALID_VALUE);
				break;
		}
	}
	catch (OSStatus		result) {
 		DebugMessageN1("ERROR: alSetInteger FAILED = %s\n", alGetString(result));
        alSetError(result);
    }
	catch (...) {
 		DebugMessage("ERROR: alSetInteger FAILED");
		alSetError(AL_INVALID_OPERATION); 
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Never actually in official OpenAL headers because no tokens were defined to use it
// deprecated, keep building for 1.0 released binary compatibility
ALAPI ALvoid ALAPIENTRY alSetDouble (ALenum pname, const ALdouble value)
{	
#if LOG_API_USAGE
	DebugMessage("***** alSetDouble");
#endif
	try {
		switch (pname)
		{			
			case ALC_MIXER_OUTPUT_RATE:
				OALDevice	*oalDevice = GetDeviceObjectByToken (gCurrentDevice);
				oalDevice->SetMixerRate ((Float64) value);
				break;
			
			default:
				alSetError(AL_INVALID_VALUE);
				break;
		}
	}
	catch (OSStatus		result) {
 		DebugMessageN1("ERROR: alSetDouble FAILED = %s\n", alGetString(result));
        alSetError(result);
    }
	catch (...) {
 		DebugMessage("ERROR: alSetDouble FAILED");
		alSetError(AL_INVALID_OPERATION); 
	}
	
	return;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alPropagationSpeed (ALfloat value)
{
#if LOG_API_USAGE
	DebugMessage("***** alPropagationSpeed");
#endif
	if (value > 0.0f)
	{
	} 
    else
	{
        alSetError(AL_INVALID_VALUE);
	}
}
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alDistanceScale (ALfloat value)
{
#if LOG_API_USAGE
	DebugMessage("***** alDistanceScale");
#endif

	if (value > 0.0f)
	{
		// gDistanceScale = value;
	} 
    else
	{
		alSetError(AL_INVALID_VALUE);
	}
}
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alEnviromentiIASIG (ALuint environment, ALenum pname, ALint value)
{
#if LOG_API_USAGE
	DebugMessage("***** alEnviromentiIASIG");
#endif
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alEnvironmentfIASIG (ALuint environment, ALenum pname, ALfloat value)
{
#if LOG_API_USAGE
	DebugMessage("***** alEnvironmentfIASIG");
#endif
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALsizei ALAPIENTRY alGenEnvironmentIASIG (ALsizei n, ALuint *environments)
{
#if LOG_API_USAGE
	DebugMessage("***** alGenEnvironmentIASIG");
#endif
	return 0;
}
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ALAPI ALvoid ALAPIENTRY alDeleteEnvironmentIASIG (ALsizei n, ALuint *environments)
{
#if LOG_API_USAGE
	DebugMessage("***** alDeleteEnvironmentIASIG");
#endif
}