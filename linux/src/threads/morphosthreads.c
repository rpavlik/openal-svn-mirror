/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * morphosthreads.c
 *
 * MorphOS thread implementation.
 */

#include <dos/dostags.h>

#include <proto/dos.h>
#include <proto/exec.h>

#include "al_siteconfig.h"
#include "al_types.h"
#include "morphosthreads.h"

struct ThreadData *_alCreateThread(int (*fn)(void *))
{
	struct ThreadData* thread;

	thread = AllocVec(sizeof (struct ThreadData), MEMF_PUBLIC);
	if (thread)
	{
		thread->td_MsgPort = CreateMsgPort();
		if (thread->td_MsgPort)
		{
			struct ThreadStartMsg* startup_msg;

			startup_msg = AllocVec(sizeof (struct ThreadStartMsg), MEMF_PUBLIC | MEMF_CLEAR);
			if (startup_msg)
			{
				startup_msg->tsm_Msg.mn_Node.ln_Type = NT_MESSAGE;
				startup_msg->tsm_Msg.mn_ReplyPort    = thread->td_MsgPort;
				startup_msg->tsm_Msg.mn_Length       = sizeof (*startup_msg);

				thread->td_Thread = CreateNewProcTags(NP_Entry, 	  (ULONG) fn,
																  NP_Name,		  (ULONG) "OpenAL Thread",
																  NP_CodeType,	  CODETYPE_PPC,
																  NP_StartupMsg, (ULONG) startup_msg,
																  NP_PPC_Arg1,	  0,
																  TAG_DONE);
				if (thread->td_Thread)
					return thread;

				FreeVec(startup_msg);
			}
			DeleteMsgPort(thread->td_MsgPort);
		}
		FreeVec(thread);
	}

	return NULL;
}

int _alWaitThread(struct ThreadData* waitfor)
{
	struct ThreadStartMsg* tsm;
	int retval = -1;

	if (waitfor == NULL)
		return -1;

	WaitPort(waitfor->td_MsgPort);
	tsm = (struct ThreadStartMsg*) GetMsg(waitfor->td_MsgPort);
	DeleteMsgPort(waitfor->td_MsgPort);
	retval = tsm->tsm_Result;
	FreeVec(tsm);
	FreeVec(waitfor);

	return retval;
}

unsigned int _alSelfThread(void)
{
	return (unsigned int) FindTask(NULL);
}


void _alExitThread(void)
{
	struct ThreadStartMsg *msg;

	if (NewGetTaskAttrs(NULL, &msg, sizeof(msg), TASKINFOTYPE_STARTUPMSG,
							  TAG_DONE) && msg)
		msg->tsm_Result = 0;

	/*RemTask(NULL);*/
}

