/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * threadlib.h
 *
 * Header which sorts out which thread package we're using.
 */
#ifndef THREADLIB_H_
#define THREADLIB_H_

#ifdef USE_POSIXTHREADS
#include <pthread.h>
#include "posixthreads.h"

#define tlCreateThread(f, d)       Posix_CreateThread(f, d)
#define tlWaitThread(t)            Posix_WaitThread(t)
#define tlKillThread(t)            Posix_KillThread(t)
#define tlSelfThread()             Posix_SelfThread()
#define tlExitThread(r)            Posix_ExitThread(r);

#elif defined USE_WINDOWSTHREADS
#include "windowsthreads.h"

#define tlCreateThread(f, d) Windows_CreateThread(f, d)
#define tlWaitThread(t)      Windows_WaitThread(t)
#define tlKillThread(t)      Windows_KillThread(t)
#define tlSelfThread()       Windows_SelfThread()
#define tlExitThread(r)      Windows_ExitThread(r);

#elif defined USE_MORPHOSTHREADS
#include "morphosthreads.h"

#define tlCreateThread(f, d) MorphOS_CreateThread(f, d)
#define tlWaitThread(t)      MorphOS_WaitThread(t)
#define tlKillThread(t)      MorphOS_KillThread(t)
#define tlSelfThread()       MorphOS_SelfThread()
#define tlExitThread(r)      MorphOS_ExitThread(r);

#else /* USE_WINDOWSTHREADS */

typedef void *ThreadID;

#define tlCreateThread(f, d) NULL
#define tlWaitThread(t)
#define tlKillThread(t)
#define tlExitThread(r)

#endif /* USE_POSIXTHREADS */

#endif /* THREADLIB_H_ */
