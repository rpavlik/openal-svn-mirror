/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * morphosmutex.c
 *
 * MorphOS mutex backend for out mutex lib interface.
 */

#include <proto/exec.h>

#include "morphosmutex.h"

struct SignalSemaphore*	MorphOS_CreateMutex(void)
{
	struct SignalSemaphore *mutex = (struct SignalSemaphore*) AllocVec(sizeof (struct SignalSemaphore), MEMF_PUBLIC);

	if (mutex)
	{
		InitSemaphore(mutex);
	}

	return mutex;
}

void MorphOS_DestroyMutex(struct SignalSemaphore* mutex)
{
	if (mutex)
	{
		FreeVec(mutex);
	}
}


int MorphOS_TryLockMutex(struct SignalSemaphore *mutex)
{
	if (!mutex)
	{
		return -1;
	}

	if (AttemptSemaphore(mutex) == 0)
	{
		return -1;
	}

	return 0;
}

void MorphOS_LockMutex(struct SignalSemaphore *mutex)
{
	if (mutex)
	{
		ObtainSemaphore(mutex);
	}
}

void MorphOS_UnlockMutex(struct SignalSemaphore *mutex)
{
	if (mutex)
	{
		ReleaseSemaphore(mutex);
	}
}

