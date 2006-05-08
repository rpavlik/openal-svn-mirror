/* darwin_native.c
 *
 * Mac OS X backend for OpenAL
 * only PowerPC target on MacOS X is supported (Darwin is not supported alone)
 *
 * file originally created on jan 2000 by
 * Guillaume Borios (gborios@free.fr) and Florent Boudet (flobo@ifrance.com)
 * to help the PineApple project (http://ios.free.fr) run on MOSX
 *
 * Version : Alpha 6
 */

#include "al_siteconfig.h"
#include <stdlib.h>
#include "backends/alc_backend.h"

#ifndef USE_BACKEND_NATIVE_DARWIN

void alcBackendOpenNative_ (UNUSED(ALC_OpenMode mode), UNUSED(ALC_BackendOps **ops),
			    ALC_BackendPrivateData **privateData)
{
	*privateData = NULL;
}

#else

#include <CoreAudio/CoreAudio.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "al_main.h"
#include "al_debug.h"
#include "al_rcvar.h"
#include "alc/alc_context.h"

/* Adding buffers improves response to temporary heavy CPU loads but increases latency... */

#define buildID 12

typedef struct {
    AudioDeviceID	deviceW;		/* the device ID */
    void *		deviceWBuffer;
    UInt32		deviceWBufferSize;	/* Buffer size of the audio device */
    AudioBufferList*	deviceWBufferList;
    AudioStreamBasicDescription	deviceFormat;	/* format of the default device */
    ALC_OpenMode mode;
} globalVars, *globalPtr;

/************************************** GLOBALS *********************************/

static globalVars	libGlobals; /* my globals */

static unsigned int	alWriteFormat;	/* format of data from AL*/
static unsigned int	alWriteSpeed;	/* speed of data from AL*/
static unsigned int	nativePreferedBuffSize;
static void * coreAudioDestination;
static int ratio;
static int stillToPlay = 0;

/************************************* PROTOTYPES *********************************/

static void implement_me(const char *fn);
static OSStatus deviceFillingProc (AudioDeviceID  inDevice, const AudioTimeStamp*  inNow, const AudioBufferList*  inInputData, const AudioTimeStamp*  inInputTime, AudioBufferList* outOutputData, const AudioTimeStamp* inOutputTime, void* inClientData);

static OSStatus GetAudioDevices (void **devices /*Dev IDs*/, short	*devicesAvailable /*Dev number*/);

int sync_mixer_iterate( void *dummy );

static void playABuffer(void *realdata);

/************************************** UTILITIES *********************************/

static int NoPrintf( UNUSED(const char *format), ... )
{
	return 0;
}

/* ToDo: Use vfprintf and nuke buffer */
static int _alDebugPrintf( const char *format, ... )
{
	static char formatbuf[256];

	va_list ap;
	va_start(ap, format);
	vsnprintf(formatbuf, sizeof formatbuf, format, ap);
	va_end(ap);

	return fprintf(stderr, "%s", formatbuf);
}

static int (*DebugPrintf)( const char *format, ... ) = NoPrintf;


static void implement_me(const char *fn)
{
	DebugPrintf("[darwin_native.c] : %s is not implemented.\nPlease contact gborios@free.fr for information or help.\n", fn);
}

/*********************************** OS callback proc *****************************/

static OSStatus deviceFillingProc (UNUSED(AudioDeviceID  inDevice), UNUSED(const AudioTimeStamp*  inNow), UNUSED(const AudioBufferList*  inInputData), UNUSED(const AudioTimeStamp*  inInputTime), AudioBufferList*  outOutputData, UNUSED(const AudioTimeStamp* inOutputTime), UNUSED(void* inClientData))
{
    coreAudioDestination = (outOutputData->mBuffers[0]).mData;

    if (stillToPlay) playABuffer(NULL);
    else sync_mixer_iterate(NULL);

    return 0;
}

/************************************** HAL Routines *********************************/

static OSStatus GetAudioDevices (void **devices /*Dev IDs*/, short	*devicesAvailable /*Dev number*/)
{
#ifdef DEBUG_MAXIMUS
    int i;
    char	cStr[256];
#endif
    OSStatus	err;
    UInt32 	outSize;
    Boolean	outWritable;

    DebugPrintf("OpenAL MOSX Backend : Build %d\n",buildID);

    /* find out how many audio devices there are, if any */
    err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &outSize, &outWritable);
    if (err) return (err);

    /* calculate the number of device available */
    *devicesAvailable = outSize / sizeof(AudioDeviceID);
    /* Bail if there aren't any devices */
    if (*devicesAvailable < 1) return (-1);

    /* make space for the devices we are about to get */
    if (*devices != NULL) free(*devices);
    *devices = malloc(outSize);
    /* get an array of AudioDeviceIDs */
    err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &outSize, (void *) *devices);
    if (err) free(*devices);
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

static void *grab_read_native(void)
{
    implement_me("void *grab_read_native()");
    return NULL;
}

static void *grab_write_native(void)
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

    error = AudioDeviceAddIOProc(libGlobals.deviceW, deviceFillingProc, (void *) &libGlobals);	/* Creates the callback proc */
    if (error != 0) goto Crash;

    libGlobals.mode = ALC_OPEN_OUTPUT_;
    return &libGlobals.deviceW;

Crash :
    DebugPrintf("An error occured during void *grab_write_native()\n");
    libGlobals.deviceW = 0;
    return NULL;
}

static ALboolean set_write_native(UNUSED(void *handle),
				  unsigned int *bufsiz,
				  unsigned int *fmt,
				  unsigned int *speed)
{
    OSStatus		error = 0;

    DebugPrintf("Init Speed : %d\n",*speed);

    /* *fmt = AL_FORMAT_STEREO16; */
    /* *speed = (unsigned int)libGlobals.deviceFormat.mSampleRate; */

    alWriteFormat = *fmt;

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
    *bufsiz /= ((unsigned int)libGlobals.deviceFormat.mSampleRate) / *speed;

    alWriteSpeed = *speed;
    ratio = ((unsigned int)libGlobals.deviceFormat.mSampleRate) / *speed;
    nativePreferedBuffSize = *bufsiz;

    /* start playing sound through the device */
    error = AudioDeviceStart(libGlobals.deviceW, deviceFillingProc);
    if (error != 0) return AL_FALSE;

    return AL_TRUE;
}


static ALboolean set_read_native(UNUSED(void *handle), UNUSED(unsigned int *bufsiz), UNUSED(unsigned int *fmt), UNUSED(unsigned int *speed))
{
    implement_me("ALboolean set_read_native()");
    return AL_FALSE;
}

static ALboolean
alcBackendSetAttributesNative_(void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed)
{
	return libGlobals.mode == ALC_OPEN_INPUT_ ?
		set_read_native(handle, bufsiz, fmt, speed) :
		set_write_native(handle, bufsiz, fmt, speed);
}

static void  native_blitbuffer(void *handle, const void *data, int bytes)
{
    stillToPlay = bytes / nativePreferedBuffSize;

    if (handle == NULL) return;

    if (coreAudioDestination == 0)
    {
        fprintf(stderr,"Something wrong happened between CoreAudio and OpenAL.\n");
        return;
    }

    /* Gyom FIXME: Is this useful? */
    assert(nativePreferedBuffSize <= bytes);

    playABuffer(data);
}

static void playABuffer(void *realdata)
{
    register unsigned int 	count;
    register Float32	*outDataPtr = coreAudioDestination;
    register SInt16	*inDataPtr16;
    register SInt8	*inDataPtr8;
    static void * data;

    if (realdata!=NULL) data = realdata;

    inDataPtr16 = (SInt16*)(data);
    inDataPtr8 = (SInt8*)(data);

    stillToPlay--;

    switch(alWriteFormat)
    {
        int i;
        case AL_FORMAT_STEREO16:
            assert(nativePreferedBuffSize <= libGlobals.deviceWBufferSize/2);
            for (count = nativePreferedBuffSize/2; count > 0; count--)
            {
                for (i = ratio; i>0; i--)
                {
                    *outDataPtr = ((Float32)(*inDataPtr16))/32767.0;
                    outDataPtr++;
                }
                inDataPtr16++;
            }
                data = inDataPtr16;
                break;

        case AL_FORMAT_MONO16:
            assert(nativePreferedBuffSize <= libGlobals.deviceWBufferSize/2);
            for (count = nativePreferedBuffSize/2; count >0; count--)
            {
                for (i = ratio*2; i>0; i--)
                {
                    *outDataPtr = ((Float32)(*inDataPtr16))/32767.0;
                    outDataPtr++;
                }
                inDataPtr16++;
            }
                data = inDataPtr16;
                break;

        case AL_FORMAT_STEREO8:
            assert(nativePreferedBuffSize <= libGlobals.deviceWBufferSize);
            for (count = nativePreferedBuffSize; count >0; count--)
            {
                for (i = ratio; i>0; i--)
                {
                    *outDataPtr = ((Float32)(*inDataPtr8))/32767.0;
                    outDataPtr++;
                }
                inDataPtr8++;
            }
                data = inDataPtr8;
                break;

        case AL_FORMAT_MONO8:
            assert(nativePreferedBuffSize <= libGlobals.deviceWBufferSize);
            for (count = nativePreferedBuffSize; count >0; count--)
            {
                for (i = ratio*2; i>0; i--)
                {
                    *outDataPtr = ((Float32)(*inDataPtr8))/32767.0;
                    outDataPtr++;
                }
                inDataPtr8++;
            }
                data = inDataPtr8;
                break;

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

static ALfloat get_nativechannel(UNUSED(void *handle), UNUSED(ALuint channel))
{
    implement_me("float get_nativechannel()");
    return 0;
}

static int set_nativechannel(UNUSED(void *handle),UNUSED(ALuint channel),UNUSED(ALfloat volume))
{
    implement_me("int set_nativechannel()");
    return 0;
}

static void pause_nativedevice(void *handle) /* Not tested :-( */
{
    if (libGlobals.deviceW == *(AudioDeviceID*)handle)
	AudioDeviceStop(libGlobals.deviceW, deviceFillingProc);
}

static void resume_nativedevice(void *handle) /* Not tested :-( */
{
    if (libGlobals.deviceW == *(AudioDeviceID*)handle)
	AudioDeviceStart(libGlobals.deviceW, deviceFillingProc);
}

static ALsizei capture_nativedevice(UNUSED(void *handle), UNUSED(void *capture_buffer), UNUSED(int bufsiz))
{
    implement_me("void capture_nativedevice()");
    return 0;
}

static ALC_BackendOps nativeOps = {
	release_native,
	pause_nativedevice,
	resume_nativedevice,
	alcBackendSetAttributesNative_,
	native_blitbuffer,
	capture_nativedevice,
	get_nativechannel,
	set_nativechannel
};

void
alcBackendOpenNative_ (ALC_OpenMode mode, ALC_BackendOps **ops, ALC_BackendPrivateData **privateData)
{
	*privateData = (mode == ALC_OPEN_INPUT_) ? grab_read_native() : grab_write_native();
	if (*privateData != NULL) {
		*ops = &nativeOps;
	}
}

#endif /* USE_BACKEND_NATIVE_DARWIN */
