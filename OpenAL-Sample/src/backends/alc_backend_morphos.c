/*
 * morphos_native.c
 *
 * MorphOS backend for OpenAL
 * using AHI for sound output
 */

#include "al_siteconfig.h"
#include <stdlib.h>
#include "backends/alc_backend.h"

#ifndef USE_BACKEND_NATIVE_MORPHOS

void alcBackendOpenNative_ (UNUSED(ALC_OpenMode mode), UNUSED(ALC_BackendOps **ops),
			    ALC_BackendPrivateData **privateData)
{
	*privateData = NULL;
}

#else

#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "al_main.h"
#include "al_debug.h"
#include "al_rcvar.h"
#include "alc/alc_context.h"

#include <clib/ddebug_protos.h>
#include <dos/dostags.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <devices/ahi.h>
#include <proto/ahi.h>

#include <exec/lists.h>
#include <exec/ports.h>
#include <exec/semaphores.h>

struct DispatcherStartupMsg
{
	struct Message dsm_Msg;
	LONG				dsm_Result;
};

struct Buffer
{
	struct AHISampleInfo    bn_SampleInfo;
	struct SignalSemaphore  bn_Semaphore;
	ULONG						   bn_SampleNo;
	ULONG						   bn_FillSize;
};

struct MOSWriteHandle
{
	ULONG				 wh_SampleType;
	ULONG				 wh_Frequency;
	ULONG				 wh_Channels;
	struct Buffer*  wh_Buffers;
	ULONG				 wh_BufSize;
	UWORD				 wh_WriteBuf;
	UWORD				 wh_ReadBuf;
	BOOL				 wh_FirstBufSent;
	struct DispatcherStartupMsg *wh_StartupMsg;
	struct MsgPort *wh_StartupPort;
	struct Process *wh_DispatcherThread;
	struct MsgPort *wh_DispatcherPort;
	ULONG				 wh_SwitchSignal;
        ALC_OpenMode mode;
};

#define Handle(h)		((struct MOSWriteHandle*) h)

struct DispatcherInitReport
{
	struct Message dir_Msg;
	ULONG				dir_RealFrequency;
	ULONG				dir_RealBufSize;
};

#define DISPATCHER_CMD_BREAK		1
#define DISPATCHER_CMD_START		2
#define DISPATCHER_CMD_PAUSE		3
#define DISPATCHER_CMD_RESUME		4

struct DispatcherMsg
{
	struct Message dm_Msg;
	ULONG				dm_Command;
};

#define AHI_BUFFERS		4
#define AHI_BUF_SIZE    _ALC_DEF_BUFSIZ*4

static VOID DispatcherThread(struct MOSWriteHandle*);
static BOOL start_sound_thread(struct MOSWriteHandle* h, ALuint *bufsiz, ALuint *speed);
static void stop_sound_thread(struct MOSWriteHandle* h);

static void do_command(struct MOSWriteHandle* h, ULONG cmd)
{
	struct DispatcherMsg msg;
	struct MsgPort *reply_port = CreateMsgPort();

	if (reply_port)
	{
		msg.dm_Msg.mn_Node.ln_Type = NT_MESSAGE;
		msg.dm_Msg.mn_ReplyPort = reply_port;
		msg.dm_Msg.mn_Length = sizeof (struct DispatcherMsg);
		msg.dm_Command = cmd;
		PutMsg(h->wh_DispatcherPort, (struct Message*) &msg);
		WaitPort(reply_port);
		GetMsg(reply_port);
		DeleteMsgPort(reply_port);
	}
}

static void *grab_read_native(void)
{
	/* Not implemented yet */
	return NULL;
}

static void *grab_write_native(void)
{
	struct MOSWriteHandle* h;

	h = Handle(AllocVec(sizeof (struct MOSWriteHandle), MEMF_PUBLIC | MEMF_CLEAR));

	if (h == NULL)
	{
		/*dprintf("No handle\n");*/
		return NULL;
	}

	h->wh_SampleType = AHIST_S16S;
	h->wh_Channels = 2;
	h->wh_Frequency = _ALC_CANON_SPEED;
	h->wh_Buffers = (struct Buffer*) AllocVec(sizeof (struct Buffer)*AHI_BUFFERS, MEMF_PUBLIC | MEMF_CLEAR);
	h->wh_BufSize = AHI_BUF_SIZE;
	h->wh_WriteBuf = 0;
	h->wh_DispatcherThread = NULL;
	h->wh_DispatcherPort = NULL;
	h->wh_FirstBufSent = FALSE;
	h->wh_StartupMsg = (struct DispatcherStartupMsg *) AllocVec(sizeof (struct DispatcherStartupMsg), MEMF_PUBLIC | MEMF_CLEAR);
	h->wh_StartupPort = CreateMsgPort();
	h->mode = ALC_OPEN_OUTPUT_;

	if (h->wh_Buffers == NULL || h->wh_StartupMsg == NULL || h->wh_StartupPort == NULL)
	{
		/*dprintf("No resources\n");*/
		release_native(h);
		return NULL;
	}

	if (start_sound_thread(h, NULL, NULL))
		return h;
	/*dprintf("No sound thread\n");*/

	return NULL;
}

static ALboolean set_write_native(void *h,
				  ALuint *bufsiz,
				  ALenum *fmt,
				  ALuint *speed)
{
	ULONG sample_type;
	ULONG channels;

	if (h == NULL)
		return AL_FALSE;

	/*dprintf("set_write_native(%d, %d, %d)\n", *bufsiz, *fmt, *speed);*/

	switch(*fmt)
	{
		case AL_FORMAT_MONO16:
			sample_type = AHIST_M16S;
			channels = 1;
			break;
		case AL_FORMAT_STEREO16:
		case AL_FORMAT_QUAD16_LOKI:
			sample_type = AHIST_S16S;
			channels = 2;
			break;
		case AL_FORMAT_MONO8:
			sample_type = AHIST_M8S;
			channels = 1;
			break;
		case AL_FORMAT_STEREO8:
		case AL_FORMAT_QUAD8_LOKI:
			sample_type = AHIST_S8S;
			channels = 2;
			break;
		default:
#ifdef DEBUG_MAXIMUS
			dprintf("OpenAL: unknown format 0x%x\n", fmt);
#endif
			sample_type = AHIST_S16S;
			channels = 2;
			*fmt = AL_FORMAT_STEREO16;
			break;
	}

	stop_sound_thread(Handle(h));

	Handle(h)->wh_SampleType = sample_type;
	Handle(h)->wh_Frequency = *speed;
	Handle(h)->wh_BufSize = *bufsiz;
	Handle(h)->wh_Channels = channels;
	Handle(h)->wh_ReadBuf = 0;
	Handle(h)->wh_WriteBuf = 0;
	Handle(h)->wh_FirstBufSent = FALSE;

	return start_sound_thread(Handle(h), bufsiz, speed);
}

static BOOL start_sound_thread(struct MOSWriteHandle* h, ALuint *bufsiz, ALuint *speed)
{
	h->wh_StartupMsg->dsm_Msg.mn_Node.ln_Type = NT_MESSAGE;
	h->wh_StartupMsg->dsm_Msg.mn_ReplyPort = h->wh_StartupPort;
	h->wh_StartupMsg->dsm_Msg.mn_Length = sizeof (struct DispatcherStartupMsg);

	if ((h->wh_DispatcherThread = CreateNewProcTags(NP_CodeType,  CODETYPE_PPC,
																	NP_Entry, 	  (ULONG) &DispatcherThread,
																	NP_Name,      (ULONG) "OpenAL Sound Thread",
																	NP_StackSize, 32000,
																	NP_StartupMsg,(ULONG) h->wh_StartupMsg,
																	NP_TaskMsgPort,(ULONG) &h->wh_DispatcherPort,
																	NP_PPC_Arg1,  (ULONG) h,
																	TAG_DONE)))
	{
		struct DispatcherInitReport *init_report;
		WaitPort(h->wh_StartupPort);
		init_report = (struct DispatcherInitReport *) GetMsg(h->wh_StartupPort);
		if (init_report->dir_Msg.mn_Length == sizeof (struct DispatcherInitReport))
		{
			if (bufsiz)	*bufsiz = init_report->dir_RealBufSize;
			if (speed)	*speed  = init_report->dir_RealFrequency;
			ReplyMsg((struct Message*) init_report);
			return AL_TRUE;
		}

		/*
		 * Otherwise we got the startup message back.
		 * This means the thread has exited already because something went wrong
		 * during initialisation.
		 */
		h->wh_DispatcherThread = NULL;
	}

	release_native(h);

	return AL_FALSE;
}

static void stop_sound_thread(struct MOSWriteHandle* h)
{
	if (h && h->wh_DispatcherThread)
	{
		do_command(h, DISPATCHER_CMD_BREAK);

		/* Wait until thread has quit */
		WaitPort(h->wh_StartupPort);
		GetMsg(h->wh_StartupPort);
		h->wh_DispatcherThread = NULL;
		h->wh_DispatcherPort = NULL;
	}
}

static void release_native(void *h)
{
	if (h)
	{
		stop_sound_thread(Handle(h));

		if (Handle(h)->wh_Buffers)
		{
			FreeVec(Handle(h)->wh_Buffers);
			Handle(h)->wh_Buffers = NULL;
		}
		if (Handle(h)->wh_StartupMsg)
		{
			FreeVec(Handle(h)->wh_StartupMsg);
			Handle(h)->wh_StartupMsg = NULL;
		}
		if (Handle(h)->wh_StartupPort)
		{
			DeleteMsgPort(Handle(h)->wh_StartupPort);
			Handle(h)->wh_StartupPort = NULL;
		}
		FreeVec(h);
	}
}

static ALboolean set_read_native(UNUSED(void *handle),
				 UNUSED(ALuint *bufsiz),
				 UNUSED(ALenum *fmt),
				 UNUSED(ALuint *speed))
{
	/* Not yet implemented */
	return AL_FALSE;
}

static ALboolean
alcBackendSetAttributesNative_(void *handle, ALuint *bufsiz, ALenum *fmt, ALuint *speed)
{
  return ((MOSWriteHandle *)handle)->mode == ALC_OPEN_INPUT_ ?
		set_read_native(handle, bufsiz, fmt, speed) :
		set_write_native(handle, bufsiz, fmt, speed);
}

static void native_blitbuffer(void *h, const void *data, int bytes)
{
	UWORD next_buf;

	/* Prepare buffer */
	next_buf = Handle(h)->wh_WriteBuf;
	ObtainSemaphore(&Handle(h)->wh_Buffers[next_buf].bn_Semaphore);
	CopyMem(data, Handle(h)->wh_Buffers[next_buf].bn_SampleInfo.ahisi_Address, bytes);
	Handle(h)->wh_Buffers[next_buf].bn_FillSize = bytes;
	ReleaseSemaphore(&Handle(h)->wh_Buffers[next_buf].bn_Semaphore);

	if (!Handle(h)->wh_FirstBufSent)
	{
		do_command(Handle(h), DISPATCHER_CMD_START);
		Handle(h)->wh_FirstBufSent = TRUE;
	}
	next_buf++;
	if (next_buf >= AHI_BUFFERS)
		next_buf = 0;
	Handle(h)->wh_WriteBuf = next_buf;
}

static ALfloat get_nativechannel(UNUSED(void *h), UNUSED(ALuint channel))
{
	/* Not yet implemented */
	return 0.0;
}

static int set_nativechannel(UNUSED(void *h),UNUSED(ALuint channel),UNUSED(ALfloat volume))
{
	/* Not yet implemented */
	return 0;
}

static void pause_nativedevice(UNUSED(void *h))
{
	/* Not tested */
	dprintf("pause_nativedevice\n");
//	  do_command(Handle(h), DISPATCHER_CMD_PAUSE);
}

static void resume_nativedevice(UNUSED(void *h))
{
	/* Not tested */
	dprintf("resume_nativedevice\n");
//	  do_command(Handle(h), DISPATCHER_CMD_RESUME);
}

static ALsizei capture_nativedevice(UNUSED(void *h), UNUSED(void *capture_buffer), UNUSED(int bufsiz))
{
	/* Not yet implemented */
	return NULL;
}

/******************************************************************************/

static ULONG OpenAL_SoundFunc(VOID)
{
	struct AHIAudioCtrl* ctrl = (struct AHIAudioCtrl*) REG_A2;
	struct AHISoundMessage* sm = (struct AHISoundMessage*) REG_A1;
	struct MOSWriteHandle* h = Handle(ctrl->ahiac_UserData);
	struct ExecBase *SysBase = *(struct ExecBase **) 4;

	AHI_SetSound(sm->ahism_Channel, h->wh_ReadBuf, 0, 0, ctrl, 0);

	/* When last channel has been started, request switching to next buffer */
	if (sm->ahism_Channel == h->wh_Channels-1)
		Signal((struct Task*) h->wh_DispatcherThread, h->wh_SwitchSignal);

	return 0;
}

static struct EmulLibEntry OpenAL_SoundGate = { TRAP_LIB, 0, (void (*)(void)) OpenAL_SoundFunc };
static struct Hook OpenAL_SoundHook = { { NULL, NULL }, (ULONG (*)(void)) &OpenAL_SoundGate, NULL, NULL }; \

struct Library*	  AHIBase = NULL;

static VOID DispatcherThread(struct MOSWriteHandle* h)
{
	struct MsgPort*	  ahi_port = NULL;
	struct AHIRequest*  ahi_request = NULL;
	struct AHIAudioCtrl*ahi_control = NULL;
	BYTE*					  sample_bufs = NULL;
	ULONG					  sample_size = 2;
	BYTE					  ahi_device = -1;
	LONG					  switch_sig_bit = -1;
	LONG				     locked_buf = -1;
	struct DispatcherStartupMsg *startup_msg;
	struct DispatcherInitReport *init_report;
	struct MsgPort     *task_port = NULL;
	struct ExecBase *SysBase = *(struct ExecBase **) 4;

	init_report = (struct DispatcherInitReport *) AllocVec(sizeof (struct DispatcherInitReport), MEMF_PUBLIC | MEMF_CLEAR);
	if (init_report == NULL)
		return;

	NewGetTaskAttrsA(NULL, &task_port, sizeof (task_port), TASKINFOTYPE_TASKMSGPORT, NULL);
	NewGetTaskAttrsA(NULL, &startup_msg, sizeof (startup_msg), TASKINFOTYPE_STARTUPMSG, NULL);

	if (task_port == NULL || startup_msg == NULL)
	{
		FreeVec(init_report);
		return;
	}

	startup_msg->dsm_Result = -1;
	switch_sig_bit = AllocSignal(-1);

	if (switch_sig_bit != -1 && (ahi_port = CreateMsgPort()))
	{
		if ((ahi_request = (struct AHIRequest *) CreateIORequest(ahi_port, sizeof (struct AHIRequest))))
		{
			ahi_request->ahir_Version = 4;
			if ((ahi_device = OpenDevice(AHINAME, AHI_NO_UNIT, (struct IORequest *) ahi_request, 0)) == 0)
			{
				AHIBase = (struct Library*) ahi_request->ahir_Std.io_Device;

				/*dprintf("AllocAudio with %d channels, %d sounds and frequency %d\n", h->wh_Channels, AHI_BUFFERS, h->wh_Frequency);*/
				ahi_control = AHI_AllocAudio(AHIA_AudioID,   AHI_DEFAULT_ID,
													  AHIA_Channels,  h->wh_Channels,
													  AHIA_Sounds,	   AHI_BUFFERS,
													  AHIA_MixFreq,   h->wh_Frequency,
													  AHIA_SoundFunc, (ULONG) &OpenAL_SoundHook,
													  AHIA_UserData,	(ULONG) h,
													  TAG_DONE
													 );
				if (ahi_control)
				{
					ULONG buf_size;
					ULONG samples, fs, fm;

					AHI_GetAudioAttrs(AHI_INVALID_ID, ahi_control, AHIDB_MaxPlaySamples, (ULONG) &samples, TAG_DONE);
					AHI_ControlAudio(ahi_control, AHIC_MixFreq_Query, (ULONG) &fm, TAG_DONE);
					fs = h->wh_Frequency;

					buf_size = samples*fs/fm;
					/*dprintf("OpenAL: Minimum buffer size is %d, requested buffer size is %d\n", buf_size, h->wh_BufSize);*/
					if (buf_size > h->wh_BufSize)
						h->wh_BufSize = buf_size;

					sample_bufs = AllocVec(h->wh_BufSize*AHI_BUFFERS, MEMF_PUBLIC | MEMF_CLEAR);
					if (sample_bufs)
					{
						struct Buffer* bn;
						ULONG	buf;
						LONG err = AHIE_OK;

						sample_size = AHI_SampleFrameSize(h->wh_SampleType);

						for (buf = 0; buf < AHI_BUFFERS && err == AHIE_OK; buf++)
						{
							bn = &h->wh_Buffers[buf];
							bn->bn_SampleNo = buf;
							bn->bn_SampleInfo.ahisi_Type = h->wh_SampleType;
							bn->bn_SampleInfo.ahisi_Address = &sample_bufs[buf*h->wh_BufSize];
							bn->bn_SampleInfo.ahisi_Length = h->wh_BufSize/sample_size;
							InitSemaphore(&bn->bn_Semaphore);
							bn->bn_FillSize = 0;
							err = AHI_LoadSound(buf, AHIST_DYNAMICSAMPLE, &bn->bn_SampleInfo, ahi_control);
						}

						if (err != AHIE_OK)
						{
							FreeVec(sample_bufs);
							sample_bufs = NULL;
						}
					}
				}
			}
		}
	}

	if (sample_bufs)
	{
		BOOL dispatcher_running = TRUE;
		ULONG signal_mask = 1 << task_port->mp_SigBit;
		ULONG signal_set;
		struct MsgPort *reply_port;

		reply_port = CreateMsgPort();
		if (reply_port == NULL)
			reply_port = task_port;

		if (startup_msg)
			startup_msg->dsm_Result = 0;
		init_report->dir_Msg.mn_Node.ln_Type = NT_MESSAGE;
		init_report->dir_Msg.mn_ReplyPort = reply_port;
		init_report->dir_Msg.mn_Length = sizeof (struct DispatcherInitReport);
		AHI_ControlAudio(ahi_control, AHIC_MixFreq_Query, (ULONG) &init_report->dir_RealFrequency, TAG_DONE);
		init_report->dir_RealBufSize = h->wh_BufSize;
		PutMsg(startup_msg->dsm_Msg.mn_ReplyPort, (struct Message*) init_report);
		WaitPort(reply_port);
		GetMsg(reply_port);
		FreeVec(init_report);
		init_report = NULL;

		if (reply_port != task_port)
			DeleteMsgPort(reply_port);

		h->wh_SwitchSignal = 1UL << switch_sig_bit;
		h->wh_ReadBuf = 0;
		while (dispatcher_running)
		{
			signal_set = Wait(signal_mask);

			if (signal_set & (1 << task_port->mp_SigBit))
			{
				struct DispatcherMsg *msg;

				while ((msg = (struct DispatcherMsg *) GetMsg(task_port)))
				{
					if (msg->dm_Msg.mn_Length == sizeof (struct DispatcherMsg))
					{
						switch (msg->dm_Command)
						{
							case DISPATCHER_CMD_START:
							{
								/*
								 * First buffer has been filled and we were previously not
								 * playing any sound yet
								 */
								ULONG chan;
								ULONG cur_buf;

								cur_buf = h->wh_ReadBuf;
								AHI_ControlAudio(ahi_control, AHIC_Play, TRUE, TAG_DONE);

								/* Lock first audio buffer */
								ObtainSemaphore(&h->wh_Buffers[cur_buf].bn_Semaphore);
								locked_buf = cur_buf;

								for (chan = 0; chan < h->wh_Channels; chan++)
								{
									AHI_SetFreq(chan, h->wh_Frequency, ahi_control, AHISF_IMM);
									AHI_SetVol(chan, 0x10000L, -0x8000L, ahi_control, AHISF_IMM);
									AHI_SetSound(chan, cur_buf, 0, 0, ahi_control, AHISF_IMM);
								}

								Wait(1 << switch_sig_bit);
								cur_buf++;
								if (cur_buf >= AHI_BUFFERS)
									cur_buf = 0;
								h->wh_ReadBuf = cur_buf;

								signal_mask |= 1UL << switch_sig_bit;
								break;
							}

							case DISPATCHER_CMD_PAUSE:
							case DISPATCHER_CMD_RESUME:
								AHI_ControlAudio(ahi_control, AHIC_Play, msg->dm_Command == DISPATCHER_CMD_RESUME, TAG_DONE);
								break;

							case DISPATCHER_CMD_BREAK:
								/* Break requests and quit */
								/*dprintf("Dispatcher thread: break requested\n");*/
								AHI_ControlAudio(ahi_control, AHIC_Play, FALSE, TAG_DONE);
								dispatcher_running = FALSE;
								break;
						}
					}

					ReplyMsg((struct Message *) msg);
				}
			}

			if (signal_set & (1UL << switch_sig_bit))
			{
				/* Switch to next read buffer */
				ULONG cur_buf;

				cur_buf = h->wh_ReadBuf;
				/*dprintf("Dispatcher thread: buffer switch requested. Releasing lock on %d, locking %d\n", locked_buf, cur_buf);*/
				memset(h->wh_Buffers[locked_buf].bn_SampleInfo.ahisi_Address, 0, h->wh_BufSize);
				ReleaseSemaphore(&h->wh_Buffers[locked_buf].bn_Semaphore);
				cur_buf++;
				if (cur_buf >= AHI_BUFFERS)
					cur_buf = 0;
				ObtainSemaphore(&h->wh_Buffers[cur_buf].bn_Semaphore);
				locked_buf = cur_buf;
				h->wh_ReadBuf = cur_buf;
				/*dprintf("Dispatcher thread: buffer switch done\n");*/
			}
		}
	}

	/* Cleanup */
	if (init_report)
	{
		FreeVec(init_report);
		init_report = NULL;
	}

	if (locked_buf != -1)
	{
		ReleaseSemaphore(&h->wh_Buffers[locked_buf].bn_Semaphore);
		locked_buf = -1;
	}

	if (switch_sig_bit != -1)
	{
		FreeSignal(switch_sig_bit);
		switch_sig_bit = -1;
	}

	if (ahi_control)
	{
		AHI_FreeAudio(ahi_control);  /* Also unloads all sounds */
		ahi_control = NULL;
	}

	if (ahi_request)
	{
		CloseDevice((struct IORequest*) ahi_request);
		DeleteIORequest((struct IORequest*) ahi_request);
		ahi_request = NULL;
		ahi_device = -1;
	}

	if (sample_bufs)
	{
		FreeVec(sample_bufs);
		sample_bufs = NULL;
	}

	if (ahi_port)
	{
		DeleteMsgPort(ahi_port);
		ahi_port = NULL;
	}
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

#endif /* USE_BACKEND_NATIVE_MORPHOS */
