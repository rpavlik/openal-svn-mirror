/*
 *	morphos_dispatcher.c
 *
 * MorphOS backend for OpenAL
 * using AHI for sound output
 */

#include "al_siteconfig.h"

#include <devices/ahi.h>
#include <clib/ddebug_protos.h>
#include <proto/ahi.h>
#include <proto/exec.h>

#include "alc/alc_context.h"

#include "arch/morphos/morphos_native.h"
#include "arch/morphos/morphos_dispatcher.h"

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

VOID DispatcherThread(struct MOSWriteHandle* h)
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

