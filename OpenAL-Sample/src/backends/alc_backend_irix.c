/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * iris.c
 *
 * functions related to the aquisition and management of the DMedia
 * audio on IRIX.
 *
 * This file is in the Public Domain and comes with no warranty.
 * Erik Hofman <erik@ehofman.com>
 *
 * For more information, please visit:
 * http://www.sgi.com/tech/faq/audio/general.html
 *
 */

/*
 * Only Indy, Indogo2 and Onyx Audio/Serial Option (ASO) can
 * natively handle 4 channel (surround sound) audio.
 *
 * Adding 4 channel audio support is possible by adding the following
 * definition to your ~/.openalrc file:
 *     (define speaker-num 4) 
 *
 * It is also possible to specify the default output port by adding
 * the following definition:
 *     (define dmedia-out-device "Analog Out")
 *
 * When your system doesn't support four channel audio by default, but
 * it does contain (at least) two different output ports you could
 * enable four channel audio by difining two separate ports:
 *     (define dmedia-out-device "Analog Out")
 *     (define dmedia-rear-out-device "Analog Out 2")
 *
 * or alternatively by selecting two different interfaces:
 *     (define dmedia-out-device "A3.Speaker")
 *     (define dmedia-rear-out-device "A3.LineOut2")
 *
 * see "man 3dm alResources" for more information.
 */

/*
 * Known problems:
 *
 * 1 channel audio doesn't work as expected.
 */

#include "al_siteconfig.h"
#include <stdlib.h>

/* dmedia complains */
#ifdef USE_BACKEND_DMEDIA
#include <dmedia/audio.h>
#define SGI_AL_CHANNELS	AL_CHANNELS
#define SGI_AL_GAIN	AL_GAIN
#undef AL_CHANNELS
#undef AL_GAIN
#undef AL_VERSION
#undef AL_INVALID_VALUE
#endif

#include "backends/alc_backend.h"

#ifndef USE_BACKEND_DMEDIA

void alcBackendOpenDMedia_ (UNUSED(ALC_OpenMode mode), UNUSED(ALC_BackendOps **ops),
			    ALC_BackendPrivateData **privateData)
{
	*privateData = NULL;
}

#else

#include <stdio.h>
#include <errno.h>
#include <AL/al.h>
#include <AL/alext.h>

#include "config/al_config.h"
#include "al_main.h"
#include "al_debug.h"

#include "alc/alc_context.h"

/*
 * Type definitions
 */
typedef struct {
    ALport port;
    ALconfig config;

    int device;
    char *name;
    unsigned int sampleWidth;
    unsigned int sampleRate;
    unsigned int numChannels;
    stamp_t offset;

    unsigned int _numChannels_init;
    unsigned int _numChannels_avail;
} _ALdevice;

typedef struct {
    _ALdevice input;
    _ALdevice *output;
    unsigned int numOutputPorts;
	ALC_OpenMode mode;
} _ALhandle;

/* Temporary storage buffer size */
#define BUF_SIZE	64
#define MAX_DELTA	8
#define CHECK_FRAMES	4
#define MAX_DEVICES	2

/*
 * Local funtion foreward declaration.
 */
static void check_sync(void *handle);
static void sync_ports(void *handle);
static int grab_device_byname(const char *name);
static void *grab_read_dmedia(void);
static void *grab_write_dmedia(void);
static void release_dmedia(void *handle);

/*
 * Driver functions
 */

static void *grab_read_dmedia(void)
{
    _ALhandle *alh;

    alh = (_ALhandle *)calloc(1, sizeof(_ALhandle));
    if (alh == NULL)
    {
        _alDebug(ALD_MAXIMUS, __FILE__, __LINE__, "Insufficient memory.\n");
        return NULL;
    }


    alh->input.name = "dmedia-in-device";
    alh->input.device = AL_DEFAULT_INPUT;
    alh->input.sampleWidth = 16;
    alh->input._numChannels_avail = 2;
    alh->input.numChannels = 2;

    alh->input.device = grab_device_byname(alh->input.name);
    if (alh->input.device <= 0)
    {
        release_dmedia(alh);
        return NULL;
    }

    alh->mode = ALC_OPEN_INPUT_;
    return NULL; /* (void *)alh; */
}

static void *grab_write_dmedia(void)
{
    _ALhandle *alh;
    ALpv params[2];
    int i, res;

    alh = (_ALhandle *)calloc(1, sizeof(_ALhandle));
    if (alh == NULL)
    {
        _alDebug(ALD_MAXIMUS, __FILE__, __LINE__, "Insufficient memory.\n");
        return NULL;
    }


    /*
     * Read the configuration file and request the proper device(s) or
     * interface(s).
     */
    alh->numOutputPorts = 1;
    alh->output = (_ALdevice *)calloc(MAX_DEVICES, sizeof(_ALdevice));
    alh->output[0].name = "dmedia-out-device";
    alh->output[0].sampleWidth = 16;
    alh->output[0]._numChannels_avail = 2;
    alh->output[0].numChannels = 2;

    alh->output[0].device = grab_device_byname(alh->output[0].name);
    if (alh->output[0].device <= 0)
    {
       if (alh->output[0].device < 0)
       {
           release_dmedia(alh);
           return NULL;
       }

        alh->output[0].device = AL_DEFAULT_OUTPUT;
    }

#if MAX_DEVICES >= 2
    alh->output[1].name = "dmedia-rear-out-device";
    alh->output[1].device = grab_device_byname(alh->output[1].name);
    if (alh->output[1].device < 0)
    {
       return NULL;
    }
    if (alh->output[1].device > 0)
        alh->numOutputPorts++;
#endif

    for (i=0; i < alh->numOutputPorts; i++)
    {
        /*
         * Store the default configuration to restore it when finished.
         */
        params[0].param = AL_CHANNEL_MODE;
        res = alGetParams(alh->output[i].device, params, 1);
        if ((res >= 0) && (params[0].sizeOut >= 0))
            alh->output[i]._numChannels_init = params[0].value.i;

        /*
         * find out how many channels the device is.
         */
        params[0].param = SGI_AL_CHANNELS;
        res = alGetParams(alh->output[i].device, params, 1);
        if ((res >= 0) && (params[0].sizeOut >= 0))
        {
            alh->output[i]._numChannels_avail = params[0].value.i;
            alh->output[i].numChannels = params[0].value.i;
        }
    }


    /*
     * Create a new config that remains available until exit.
     * Only the master port has a configurtion assigned to it.
     * All slave ports share the same config.
     */
    
    alh->output[0].config = alNewConfig();
    if (alh->output[0].config == NULL)
    {
        _alDebug(ALD_MAXIMUS, __FILE__, __LINE__, alGetErrorString(oserror()));
        return NULL;
    }
    alh->mode = ALC_OPEN_OUTPUT_;

    return (void *)alh;
}

static void dmedia_blitbuffer(void *handle,
                      const void *dataptr,
                      int bytes_to_write)
{
    static int check_ = CHECK_FRAMES;
    _ALhandle *alh = (_ALhandle *)handle;
    int sample_width = (alh->output[0].sampleWidth / 8);
    int frame_size = sample_width * alh->output[0].numChannels;
    int frames_to_write, frame_no, i;
    char **buf, *ptr = (void *)dataptr;

    if (alh->numOutputPorts == 1)
    {
        alWriteFrames(alh->output[0].port, ptr, bytes_to_write/frame_size);
        return;
    }

#if MAX_DEVICES >= 2
    /*
     * Setup a buffer for each audio port.
     */
    frames_to_write = bytes_to_write / alh->numOutputPorts;

    buf = (char **)calloc(alh->numOutputPorts, sizeof(char *));
    for (i=0; i < alh->numOutputPorts; i++)
        buf[i] = malloc(frames_to_write);

    /*
     * Fill the buffers in the appropriate format.
     */
    for(frame_no = 0; frame_no < frames_to_write; frame_no += frame_size)
    {
        for (i=0; i < alh->numOutputPorts; i++)
        {
            memcpy(&buf[i][frame_no], ptr, frame_size);
            ptr += frame_size;
        }
        
    }


    /*
     * Sent the data to the appropriate output port and clean up the buffers.
     */
    for (i=0; i < alh->numOutputPorts; i++)
    {
        alWriteFrames(alh->output[i].port, buf[i], frames_to_write/frame_size);
        free(buf[i]);
    }
    free(buf);

    if (! --check_)
    {
        check_ = CHECK_FRAMES;
        check_sync(handle);
    }
#endif
}

static void release_dmedia(void *handle)
{
    _ALhandle *alh = (_ALhandle *)handle;
    ALpv params;
    int i;

    if (alh->output[0].config != NULL) {
        alFreeConfig(alh->output[0].config);
        alh->output[0].config = NULL;
    }

    for (i=0; i < alh->numOutputPorts; i++)
    {
        params.param = AL_CHANNEL_MODE;
        params.value.i = alh->output[i]._numChannels_init;
        if(alh->output[i]._numChannels_init > 0)
            alSetParams(alh->output[i].device, &params, 1);

        if (alh->output[i].port != NULL) {
            alClosePort(alh->output[i].port);
            alh->output[i].port = NULL;
        }
    }

    if (alh->output != NULL) {
        free(alh->output);
        alh->output = NULL;
    }

    if (alh->input.config != NULL) {
        alFreeConfig(alh->input.config);
        alh->input.config = NULL;
    }

    if (alh->input.port != NULL) {
        alClosePort(alh->input.port);
        alh->input.port = NULL;
    }

    if (alh != NULL) {
        free(alh);
        alh = NULL;
    }
}

static int set_dmediachannel(UNUSED(void *handle),
                      UNUSED(ALuint channel),
                      UNUSED(ALfloat volume))
{
    return 0;
}

static void pause_dmedia(void *handle)
{
    _ALhandle *alh = (_ALhandle *)handle;
    ALpv params;
    int i;

    params.param = AL_RATE;
    params.value.i = 0;
    alSetParams(alh->input.device, &params, 1);

    for (i=0; i < alh->numOutputPorts; i++)
        alSetParams(alh->output[i].device, &params, 1);

    if (alh->numOutputPorts > 1)
        sync_ports(handle);

    return;
}

static void resume_dmedia(void *handle)
{
    _ALhandle *alh = (_ALhandle *)handle;
    ALpv params;
    int i;

    if (alh->numOutputPorts > 1)
        sync_ports(handle);

    params.param = AL_INPUT_RATE;
    params.value.i = alh->input.sampleRate;
    alSetParams(alh->input.device, &params, 1);

    params.param = AL_OUTPUT_RATE;
    params.value.i = alh->output[0].sampleRate;
    for (i=0; i < alh->numOutputPorts; i++)
        alSetParams(alh->output[i].device, &params, 1);

    return;
}

static ALfloat get_dmediachannel(UNUSED(void *handle), UNUSED(ALuint channel))
{
    return 0.0;
}

/* capture data from the audio device */
static ALsizei capture_dmedia(UNUSED(void *handle),
                             UNUSED(void *capture_buffer),
                             UNUSED(int bufsiz))
{
    return 0;
}

static ALboolean set_write_dmedia(void *handle,
				  unsigned int *bufsiz,
				  ALenum *fmt,
				  unsigned int *speed)
{
    _ALhandle *alh = (_ALhandle *)handle;
    ALuint channels = _alGetChannelsFromFormat(*fmt);
    unsigned int data_format;
    ALpv params[2];
    int i, result;

    if (alh->output[0]._numChannels_avail >= channels)
        alh->numOutputPorts = 1;

    switch(*fmt) {
    case AL_FORMAT_MONO8:
    case AL_FORMAT_STEREO8:
    case AL_FORMAT_QUAD8_LOKI:
        data_format = AL_SAMPLE_8;
        alh->output[0].sampleWidth = 8;
        break;
    case AL_FORMAT_MONO16:
    case AL_FORMAT_STEREO16:
    case AL_FORMAT_QUAD16_LOKI:
        data_format = AL_SAMPLE_16;
        alh->output[0].sampleWidth = 16;
        break;
    default:
        _alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
                 "Unsuported audio format: %d\n", *fmt);
        return AL_FALSE;
    }

    /*
     * Change the playback sample rate
     */
    alh->output[0].sampleRate = *speed;
    params[0].param = AL_MASTER_CLOCK;
    params[0].value.i = AL_MCLK_TYPE;
    params[1].param = AL_RATE;
    params[1].value.i = alh->output[0].sampleRate;
    for(i=0; i < alh->numOutputPorts; i++)
        result = alSetParams(alh->output[i].device, params, 2);


    if (alh->output[0].config != NULL)
    {
        alFreeConfig(alh->output[0].config);
        alh->output[0].config = alNewConfig();
        if (alh->output[0].config == NULL)
        {
            _alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
                     alGetErrorString(oserror()));
            return AL_FALSE;
        }
    }


    if (alh->output[0].port == NULL) 	/* Only if port not initialized */
    {
        alh->output[0].numChannels = channels/alh->numOutputPorts;
        if (alh->output[0].numChannels <= 1)
            alh->output[0].numChannels = 2;

        for (i=0; i < alh->numOutputPorts; i++)
        {
            params[0].param = AL_CHANNEL_MODE;
            params[0].value.i = alh->output[0].numChannels;
            alSetParams(alh->output[i].device,params, 1);
        }

        /*
         * Change the size of the audio queue
         * and the number of audio channels.
         */
        result = alSetChannels(alh->output[0].config, alh->output[0].numChannels);
        alSetQueueSize(alh->output[0].config, alh->output[0].sampleRate*alh->output[0].numChannels/10);
    }

    alSetSampFmt(alh->output[0].config, AL_SAMPFMT_TWOSCOMP);
    alSetWidth(alh->output[0].config, data_format);

    /*
     * Alter configuration parameters, if possible
     */
    if (alh->output[0].port == NULL)
    {

        for (i=0; i < alh->numOutputPorts; i++)
        {
            alSetDevice(alh->output[0].config, alh->output[i].device);
            /*
             * Now attempt to open an audio output port using this config
             */
            alh->output[i].port = alOpenPort(alh->output[i].name, "w",
                                             alh->output[0].config);

            if (alh->output[i].port == NULL)
            {
                _alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
                         alGetErrorString(oserror()));
                return AL_FALSE;
            }
        }
    }
    else
    {

        for (i=0; i < alh->numOutputPorts; i++)
        {
            alSetDevice(alh->output[0].config, alh->output[i].device);
            result = alSetConfig(alh->output[i].port, alh->output[0].config);
            if (result == NULL)
            {
                _alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
                         alGetErrorString(oserror()));
                return AL_FALSE;
            }
        }
    }

    if (alh->numOutputPorts > 1)
        sync_ports(alh);

    return AL_TRUE;
}

static ALboolean set_read_dmedia(UNUSED(void *handle),
				 UNUSED(unsigned int *bufsiz),
				 UNUSED(ALenum *fmt),
				 UNUSED(unsigned int *speed))
{
    return AL_FALSE;
}

static ALboolean
alcBackendSetAttributesDMedia_(void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed)
{
	return ((_ALhandle*)handle)->mode == ALC_OPEN_INPUT_ ?
		set_read_dmedia(handle, bufsiz, fmt, speed) :
		set_write_dmedia(handle, bufsiz, fmt, speed);
}

/*
 * Local function definitions
 */
static void check_sync(void *handle)
{
    _ALhandle *alh = (_ALhandle *)handle;
    stamp_t msc[MAX_DEVICES];
    int need_sync = 0;
    stamp_t buf_delta_msc;
    stamp_t ref_delta_msc;
    int i;

    if (alh->numOutputPorts == 1)
        return;

    /*
     * Get the sample frame number associated with
     * each port.
     */
    for (i = 0; i < alh->numOutputPorts; i++) {
        alGetFrameNumber(alh->output[i].port, &msc[i]);
    }

    ref_delta_msc = msc[0];

    for (i = 0; i < alh->numOutputPorts; i++) {
        /*
         * For each port, see how far ahead or behind
         * the first port we are, and keep track of the
         * maximum. in a moment, we'll bring all the ports to this
         * number.
         */
        buf_delta_msc = msc[i] - alh->output[i].offset;
        if (abs(buf_delta_msc - ref_delta_msc) > MAX_DELTA) {
            need_sync++;
        }
    }

    if (need_sync) {
        sync_ports(handle);
    }
}

static void sync_ports(void *handle)
{
    _ALhandle *alh = (_ALhandle *)handle;
    double nanosec_per_frame = (1000000000.0)/alh->output[0].sampleRate;
    stamp_t buf_delta_msc;
    stamp_t msc[MAX_DEVICES];
    stamp_t ust[MAX_DEVICES];
    int corrected;
    int i;

    /*
     * Get UST/MSC (time & sample frame number) pairs for the
     * device associated with each port.
     */
    for (i = 0; i < alh->numOutputPorts; i++) {
        alGetFrameTime(alh->output[i].port, &msc[i], &ust[i]);
    }

    /*
     * We consider the first device to be the "master" and we
     * calculate, for each other device, the sample frame number
     * on that device which has the same time as ust[0] (the time
     * at which sample frame msc[0] went out on device 0).
     *
     * From that, we calculate the sample frame offset between
     * contemporaneous samples on the two devices. This remains
     * constant as long as the devices don't drift.
     *
     * If the port i is connected to the same device as port 0, you should
     * see offset[i] = 0.
     */

    /* alh->output[0].offset = 0;         / * by definition */
    for (i = 0; i < alh->numOutputPorts; i++) {
        stamp_t msc0 = 
           msc[i] + (stamp_t)((double) (ust[0] - ust[i]) / (nanosec_per_frame));
        alh->output[i].offset = msc0 - msc[0];
    }

    do {
        stamp_t max_delta_msc;
        corrected = 0;

        /*
         * Now get the sample frame number associated with
         * each port.
         */
        for (i = 0; i < alh->numOutputPorts; i++) {
            alGetFrameNumber(alh->output[i].port, &msc[i]);
        }

        max_delta_msc = msc[0];
        for (i = 0; i < alh->numOutputPorts; i++) {
            /*
             * For each port, see how far ahead or behind
             * the furthest port we are, and keep track of the
             * minimum. in a moment, we'll bring all the ports to this
             * number.
             */
            buf_delta_msc = msc[i] - alh->output[i].offset;
            if (max_delta_msc < buf_delta_msc) {
                max_delta_msc = buf_delta_msc;
            }
        }

        for (i = 0; i < alh->numOutputPorts; i++) {
            buf_delta_msc = msc[i] - alh->output[i].offset;
            if (abs(buf_delta_msc - max_delta_msc) > MAX_DELTA ) {
                alDiscardFrames(alh->output[i].port,
                                (int)(max_delta_msc-buf_delta_msc));
                corrected++;
            }
        }
    } while (corrected);
}

static int grab_device_byname(const char *name)
{
    int device = 0;
    Rcvar rcv;

    if ((rcv = rc_lookup(name)) != NULL)
    {
        char buf[BUF_SIZE];
        ALpv params[2];

        rc_tostr0(rcv, buf, BUF_SIZE);
        device = alGetResourceByName(AL_SYSTEM, buf, AL_DEVICE_TYPE);
        if (device)
        {
            int res;
            /*
             * if the name refers to an interface, select that interface
             * on the device
             */
            if (res = alGetResourceByName(AL_SYSTEM, buf, AL_INTERFACE_TYPE))
            {
                params[0].param = AL_INTERFACE;
                params[0].value.i = res;
                alSetParams(device, params, 1);
            }
        }
        else
        {
            device = -1;
            _alDebug(ALD_MAXIMUS, __FILE__, __LINE__,
                     alGetErrorString(oserror()));
        }

    }

    return device;
}

static ALC_BackendOps dmediaOps = {
	release_dmedia,
	pause_dmedia,
	resume_dmedia,
	alcBackendSetAttributesDMedia_,
	dmedia_blitbuffer,
	capture_dmedia,
	get_dmediachannel,
	set_dmediachannel
};

void
alcBackendOpenDMedia_ (ALC_OpenMode mode, ALC_BackendOps **ops, ALC_BackendPrivateData **privateData)
{
	*privateData = (mode == ALC_OPEN_INPUT_) ? grab_read_dmedia() : grab_write_dmedia();
	if (*privateData != NULL) {
		*ops = &dmediaOps;
	}
}

#endif /* USE_BACKEND_DMEDIA */
