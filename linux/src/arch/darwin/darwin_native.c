/* darwin_native.c
 *
 * Mac OS X backend for OpenAL
 * only PowerPC target on MacOS X is supported (Darwin is not supported alone)
 *
 * file originally created on jan 2000 by
 * Guillaume Borios (gborios@free.fr) and Florent Boudet (flobo@ifrance.com)
 * to help the PineApple project (http://ios.free.fr) run on MOSX
 *
 * Version : Alpha 5
 */
 

#include <CoreAudio/CoreAudio.h>

#include "arch/interface/interface_sound.h"
#include "arch/darwin/darwin_native.h"

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "al_main.h"
#include "al_debug.h"
#include "al_rcvar.h"
#include "alc/alc_context.h"

#define maxBuffer 25 /*number of buffers to use (1 buffer = 11.61 ms with my sound card, this may vary) */
/* Adding buffers improves response to temporary heavy CPU loads but increases latency... */

#define buildID 10

typedef struct {
    volatile ALboolean	bufferIsEmpty;
    void *		startOfDataPtr;
} archBuffer;

typedef struct {
    AudioDeviceID	deviceW;		/* the device ID */
    void *		deviceWBuffer;
    UInt32		deviceWBufferSize;	/* Buffer size of the audio device */
    AudioBufferList*	deviceWBufferList;
    AudioStreamBasicDescription	deviceFormat;	/* format of the default device */
    short		bufferToFill;		/* Id of the next buffer to fill */
    short		bufferToRead;		/* Id of the next buffer to read */
    archBuffer		buffer[maxBuffer];	/* Buffers array */
} globalVars, *globalPtr;

/************************************** GLOBALS *********************************/

static globalVars	libGlobals; /* my globals */

static unsigned int	alWriteFormat;	/* format of data from AL*/
static unsigned int	alWriteSpeed;	/* speed of data from AL*/
static unsigned int	nativePreferedBuffSize;

/************************************* PROTOTYPES *********************************/

static void implement_me(const char *fn);
OSStatus deviceFillingProc (AudioDeviceID  inDevice, const AudioTimeStamp*  inNow, const AudioBufferList*  inInputData, const AudioTimeStamp*  inInputTime, AudioBufferList* outOutputData, const AudioTimeStamp* inOutputTime, void* inClientData);

OSStatus GetAudioDevices (void **devices /*Dev IDs*/, short	*devicesAvailable /*Dev number*/);

/************************************** UTILITIES *********************************/

static int NoPrintf( UNUSED(const char *format), ... )
{
	return 0;
}

static int (*DebugPrintf)( const char *format, ... ) = NoPrintf;


static void implement_me(const char *fn)
{
	DebugPrintf("[darwin_native.c] : %s is not implemented.\nPlease contact gborios@free.fr for information or help.\n", fn);
}

/*********************************** OS callback proc *****************************/

OSStatus deviceFillingProc (UNUSED(AudioDeviceID  inDevice), UNUSED(const AudioTimeStamp*  inNow), UNUSED(const AudioBufferList*  inInputData), UNUSED(const AudioTimeStamp*  inInputTime), AudioBufferList*  outOutputData, UNUSED(const AudioTimeStamp* inOutputTime), void* inClientData)
{    
    register SInt16	*inDataPtr16;
    register Float32	*outDataPtr;
    int 		count;
    globalPtr		theGlobals;
    
    theGlobals = inClientData;
    
    if (theGlobals->buffer[theGlobals->bufferToRead].bufferIsEmpty == AL_FALSE)
    {
        inDataPtr16 = (SInt16*)(libGlobals.buffer[libGlobals.bufferToRead].startOfDataPtr);
        outDataPtr = (outOutputData->mBuffers[0]).mData;
        for (count = nativePreferedBuffSize/2; count >0; count--)
        {
            *outDataPtr = ((Float32)(*inDataPtr16))/32767.0;
            outDataPtr++;
            inDataPtr16++;
        }
	theGlobals->buffer[theGlobals->bufferToRead].bufferIsEmpty = AL_TRUE;
	theGlobals->bufferToRead = ++(theGlobals->bufferToRead) % maxBuffer;
    }
    
    return 0;     
}

/************************************** HAL Routines *********************************/

OSStatus GetAudioDevices (void **devices /*Dev IDs*/, short	*devicesAvailable /*Dev number*/)
{
#ifdef DEBUG_MAXIMUS
    int i;
    char	cStr[256];
#endif
    OSStatus	err = NULL;
    UInt32 	outSize;
    Boolean	outWritable;

    DebugPrintf("OpenAL MOSX Backend : Build %d\n",buildID);
    
    // find out how many audio devices there are, if any
    err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &outSize, &outWritable);	
    if (err != NULL) return (err);
   
    // calculate the number of device available
    *devicesAvailable = outSize / sizeof(AudioDeviceID);
    // Bail if there aren't any devices
    if (*devicesAvailable < 1) return (-1);

    // make space for the devices we are about to get
    if (*devices != NULL) free(*devices);
    *devices = malloc(outSize);						
    // get an array of AudioDeviceIDs
    err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &outSize, (void *) *devices);	
    if (err != NULL) free(*devices);
#ifdef DEBUG_MAXIMUS
    DebugPrintf("Found %d Audio Device(s)\n",*devicesAvailable);
    
    for (i=0; i<*devicesAvailable;i++)
    {
        UInt32 ID = ((UInt32*)(*devices))[i];
        err =  AudioDeviceGetPropertyInfo(ID, 0, 0, kAudioDevicePropertyDeviceName,  &outSize, &outWritable);
        err = AudioDeviceGetProperty(ID, 0, 0, kAudioDevicePropertyDeviceName, &outSize, cStr);
        DebugPrintf("Device #%d : %s",ID,cStr);
        err = AudioDeviceGetProperty(ID, 0, 0, kAudioDevicePropertyDeviceManufacturer, &outSize, cStr);
        DebugPrintf(" (%s)\n",cStr);

    }
#endif
    return (err);
}


/************************************** INTERFACE *********************************/

void *grab_read_native(void)
{
    implement_me("void *grab_read_native()");
    return NULL;
}

void *grab_write_native(void)
{
    OSStatus	error = 0;
    UInt32	count;
    void *	devices = 0;
    short	devicesAvailable;
    Boolean	outWritable;

    Rcvar native_debug = rc_lookup("native-backend-debug");
    if((rc_type(native_debug) == ALRC_BOOL) &&
       (rc_tobool(native_debug) == AL_TRUE))
    {
	    DebugPrintf = _alDebugPrintf;
    }
    else
    {
	    DebugPrintf = NoPrintf;
    }

    /* Look for audio devices */
    error = GetAudioDevices (&devices ,&devicesAvailable);
    if (error != 0) goto Crash;
    libGlobals.deviceW = ((AudioDeviceID*)(devices))[0]; /* Selecting first device */
    
    /* Getting buffer size */
    error = AudioDeviceGetPropertyInfo(libGlobals.deviceW, 0, 0, kAudioDevicePropertyBufferSize, &count, &outWritable);
    if (error != 0) goto Crash;
    error = AudioDeviceGetProperty(libGlobals.deviceW, 0, 0, kAudioDevicePropertyBufferSize, &count, &libGlobals.deviceWBufferSize);
    if (error != 0) goto Crash;
    DebugPrintf("IOProperties : Buffersize = %d\n",
		(int) libGlobals.deviceWBufferSize);
    
    /* getting streams configs */
    error = AudioDeviceGetPropertyInfo(libGlobals.deviceW, 0, 0, kAudioDevicePropertyStreamConfiguration,  &count, &outWritable);
    if (error != 0) goto Crash;
    {
        libGlobals.deviceWBufferList = malloc(count);
        
        error = AudioDeviceGetProperty(libGlobals.deviceW, 0, 0, kAudioDevicePropertyStreamConfiguration, &count, libGlobals.deviceWBufferList);
        if (error != 0) goto Crash;
#ifdef DEBUG_MAXIMUS
       {
	    unsigned int i;
            DebugPrintf("IOProperties : Buffer number = %d\n",libGlobals.deviceWBufferList->mNumberBuffers);
            /*device->outStreamsInfo  = malloc(sizeof(StreamInfo) * device->totalOutputStreams);*/
            for (i = 0; i < libGlobals.deviceWBufferList->mNumberBuffers; i++) 
            {
                DebugPrintf("  Buffer %d Properties : DataByteSize = %d\n",i,libGlobals.deviceWBufferList->mBuffers[i].mDataByteSize);
                DebugPrintf("  Buffer %d Properties : NumberChannels = %d\n",i,libGlobals.deviceWBufferList->mBuffers[i].mNumberChannels);
                DebugPrintf("  Buffer %d Properties : Data = %d\n",i,libGlobals.deviceWBufferList->mBuffers[i].mData);
                /*error = GetPhysicalFormatCount(device, i + 1, &device->outStreamsInfo[i].pFormatCount, false);
                device->outStreamsInfo[i].pFormatMenuSelection = 1;

                error = GetActualFormatCount(device, i + 1, &device->outStreamsInfo[i].aFormatCount, false);
                device->outStreamsInfo[i].aFormatMenuSelection = 1;*/
            }
        }
#endif
    }

    error = AudioDeviceGetPropertyInfo(libGlobals.deviceW, 0, 0, kAudioDevicePropertyStreamFormats,  &count, &outWritable);
    if (error != 0) goto Crash;
    error = AudioDeviceGetProperty(libGlobals.deviceW, 0, 0, kAudioDevicePropertyStreamFormats, &count, &libGlobals.deviceFormat);
    if (error != 0) goto Crash;
    
#ifndef DEBUG_MAXIMUS
    DebugPrintf("IOProperties : SampleRate = %f\n",libGlobals.deviceFormat.mSampleRate);
    DebugPrintf("IOProperties : FormatFlags = %d\n",(int)libGlobals.deviceFormat.mFormatFlags);
    DebugPrintf("IOProperties : BytesPerPacket = %d\n",(int)libGlobals.deviceFormat.mBytesPerPacket);
    DebugPrintf("IOProperties : FramesPerPacket = %d\n",(int)libGlobals.deviceFormat.mFramesPerPacket);
    DebugPrintf("IOProperties : BytesPerFrame = %d\n",(int)libGlobals.deviceFormat.mBytesPerFrame);
    DebugPrintf("IOProperties : ChannelsPerFrame = %d\n",(int)libGlobals.deviceFormat.mChannelsPerFrame);
    DebugPrintf("IOProperties : BitsPerChannel = %d\n",(int)libGlobals.deviceFormat.mBitsPerChannel);
#endif

    error = AudioDeviceGetPropertyInfo(libGlobals.deviceW, 0, 0, kAudioDevicePropertyStreamFormat,  &count, &outWritable);
    if (error != 0) goto Crash;
    error = AudioDeviceGetProperty(libGlobals.deviceW, 0, 0, kAudioDevicePropertyStreamFormat, &count, &libGlobals.deviceFormat);
    if (error != 0) goto Crash;

    _alBlitBuffer = native_blitbuffer; /* Defines the blitbuffer function */
    
    error = AudioDeviceAddIOProc(libGlobals.deviceW, deviceFillingProc, (void *) &libGlobals);	/* Creates the callback proc */
    if (error != 0) goto Crash;
    
    return &libGlobals.deviceW;

Crash :
    DebugPrintf("An error occured during void *grab_write_native()\n");
    libGlobals.deviceW = NULL;
    return NULL;
}


ALboolean set_write_native(UNUSED(void *handle),
			   unsigned int *bufsiz,
			   unsigned int *fmt,
			   unsigned int *speed)
{
    OSStatus		error = 0;
    unsigned short 	i;

    DebugPrintf("Init Speed : %d\n",*speed);
    
    *fmt = AL_FORMAT_STEREO16;
    *speed = (unsigned int)libGlobals.deviceFormat.mSampleRate;
    
    alWriteFormat = *fmt;

    /* Set the buffers states to empty */
    for(i=0; i<maxBuffer; i++)
    {
	if (libGlobals.buffer[i].startOfDataPtr != NULL)
	{
		free(libGlobals.buffer[i].startOfDataPtr);
	}

	libGlobals.buffer[i].startOfDataPtr = malloc(libGlobals.deviceWBufferSize/2);
	if(libGlobals.buffer[i].startOfDataPtr == NULL)
	{
		/* JIV FIXME: release other allocations before returning */
		return AL_FALSE;
	}
	libGlobals.buffer[i].bufferIsEmpty = AL_TRUE;
    }

    libGlobals.bufferToFill = 0;
    libGlobals.bufferToRead = 0;
    
    /* defines what the AL buffer size should be */
    switch(alWriteFormat)
    {
	case AL_FORMAT_STEREO8:*bufsiz = libGlobals.deviceWBufferSize/4;
                                DebugPrintf("Init fmt : AL_FORMAT_STEREO8\n");
        break;
	case AL_FORMAT_MONO16: *bufsiz = libGlobals.deviceWBufferSize/4;
                                DebugPrintf("Init fmt : AL_FORMAT_MONO16\n");
	break;
	
	case AL_FORMAT_STEREO16: *bufsiz = libGlobals.deviceWBufferSize/2;
                                DebugPrintf("Init fmt : AL_FORMAT_STEREO16\n");
	break;
	
	case AL_FORMAT_MONO8: *bufsiz = libGlobals.deviceWBufferSize/8;
                                DebugPrintf("Init fmt : AL_FORMAT_MONO8\n");
	break;

	default: break;
    }
    if ( *speed == 22050) *bufsiz/=2;
    alWriteSpeed = *speed;
    nativePreferedBuffSize = *bufsiz;

    /* start playing sound through the device */
    error = AudioDeviceStart(libGlobals.deviceW, deviceFillingProc);
    if (error != 0) return NULL;

    return AL_TRUE;
}


ALboolean set_read_native(UNUSED(void *handle), UNUSED(unsigned int *bufsiz), UNUSED(unsigned int *fmt), UNUSED(unsigned int *speed))
{
    implement_me("ALboolean set_read_native()");
    return AL_FALSE;
}


int   grab_mixerfd(void)
{
    implement_me("int grab_mixerfd()");
    return NULL;
}


void  native_blitbuffer(void *handle, void *data, int bytes)
{
    unsigned int i;   
   
    if (handle == NULL) return;

    switch(alWriteFormat)
    {
	case AL_FORMAT_STEREO16:
	    for (i = 0; i < bytes/nativePreferedBuffSize; i++)
	    {
		    assert(nativePreferedBuffSize <= bytes);
		    assert(nativePreferedBuffSize <= libGlobals.deviceWBufferSize/2);

		    /* JIV FIXME

                      seems incorrect

		    assert(nativePreferedBuffSize <= libGlobals.deviceWBufferSize/2 - i * nativePreferedBuffSize);
		      memcpy(libGlobals.buffer[libGlobals.bufferToFill].startOfDataPtr + i * nativePreferedBuffSize, data, nativePreferedBuffSize);
                     */

		    memcpy(libGlobals.buffer[libGlobals.bufferToFill].startOfDataPtr, data, nativePreferedBuffSize);

		
		    libGlobals.buffer[libGlobals.bufferToFill].bufferIsEmpty = AL_FALSE;
		    libGlobals.bufferToFill = ++libGlobals.bufferToFill % maxBuffer;
		    while (libGlobals.buffer[libGlobals.bufferToFill].bufferIsEmpty != AL_TRUE)
			    sleep(0.02);
	    }
            return;

	default:
                DebugPrintf("Format not recognized... Try again ;-)");
                break;
    }
}


void  release_native(void *handle)
{
    if (libGlobals.deviceW == *(AudioDeviceID*)handle)
    {
        AudioDeviceStop(libGlobals.deviceW, deviceFillingProc);
	AudioDeviceRemoveIOProc(libGlobals.deviceW, deviceFillingProc);
    }
}

float get_nativechannel(UNUSED(void *handle), UNUSED(ALCenum channel))
{
    implement_me("float get_nativechannel()");
    return NULL;
}

int   set_nativechannel(UNUSED(void *handle),UNUSED( ALCenum channel),UNUSED( float volume))
{
    implement_me("int set_nativechannel()");
    return NULL;
}

void pause_nativedevice(void *handle) /* Not tested :-( */
{
    if (libGlobals.deviceW == *(AudioDeviceID*)handle)
	AudioDeviceStop(libGlobals.deviceW, deviceFillingProc);
}

void resume_nativedevice(void *handle) /* Not tested :-( */
{
    if (libGlobals.deviceW == *(AudioDeviceID*)handle)
	AudioDeviceStart(libGlobals.deviceW, deviceFillingProc);
}

ALsizei capture_nativedevice(UNUSED(void *handle), UNUSED(void *capture_buffer), UNUSED(int bufsiz))
{
    implement_me("void capture_nativedevice()");
    return NULL;
}


