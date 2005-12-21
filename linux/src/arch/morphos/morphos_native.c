/*
 * morphos_native.c
 *
 * MorphOS backend for OpenAL
 * using AHI for sound output
 */

#include "arch/interface/interface_sound.h"
#include "arch/interface/platform.h"
#include "arch/morphos/morphos_dispatcher.h"

#include <fcntl.h>
#include <stdlib.h>
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

void *grab_read_native(void)
{
	/* Not implemented yet */
	return NULL;
}

void *grab_write_native(void)
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


ALboolean set_write_native(void *h,
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
			_alBlitBuffer = native_blitbuffer;
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

void release_native(void *h)
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

ALboolean set_read_native(UNUSED(void *handle),
			  UNUSED(ALuint *bufsiz),
			  UNUSED(ALenum *fmt),
			  UNUSED(ALuint *speed))
{
	/* Not yet implemented */
	return AL_FALSE;
}

void native_blitbuffer(void *h, void *data, int bytes)
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

float get_nativechannel(UNUSED(void *h), UNUSED(ALCenum channel))
{
	/* Not yet implemented */
	return 0.0;
}

int set_nativechannel(UNUSED(void *h),UNUSED(ALCenum channel),UNUSED(float volume))
{
	/* Not yet implemented */
	return 0;
}

void pause_nativedevice(UNUSED(void *h))
{
	/* Not tested */
	dprintf("pause_nativedevice\n");
//	  do_command(Handle(h), DISPATCHER_CMD_PAUSE);
}

void resume_nativedevice(UNUSED(void *h))
{
	/* Not tested */
	dprintf("resume_nativedevice\n");
//	  do_command(Handle(h), DISPATCHER_CMD_RESUME);
}

ALsizei capture_nativedevice(UNUSED(void *h), UNUSED(void *capture_buffer), UNUSED(int bufsiz))
{
	/* Not yet implemented */
	return NULL;
}

