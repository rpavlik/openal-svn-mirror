/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * morphos_dispatcher.h
 *
 * Native MorphOS backend implementation.
 */
#ifndef MORPHOS_DISPATCHER_H_
#define MORPHOS_DISPATCHER_H_

#include <exec/lists.h>
#include <exec/ports.h>
#include <exec/semaphores.h>
#include <devices/ahi.h>

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

extern VOID DispatcherThread(struct MOSWriteHandle*);

#endif /* MORPHOS_DISPATCHER_H_ */

