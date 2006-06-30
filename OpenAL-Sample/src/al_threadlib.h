/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_threadlib.h
 *
 * Header which sorts out which thread package we're using.
 */
#ifndef AL_AL_THREADLIB_H_
#define AL_AL_THREADLIB_H_

#include "al_siteconfig.h"

#if defined(USE_POSIXTHREADING)

#include <pthread.h>
typedef pthread_t *ThreadID;

#elif defined(USE_WINDOWSTHREADING)

#include <windows.h>
typedef HANDLE ThreadID;

#else /* not defined(USE_WINDOWSTHREADING) */

#error "No thread package"

#endif /* not defined(USE_WINDOWSTHREADING) */

/*
 * Creates a thread, which starts by running fn.
 */
extern ThreadID _alCreateThread( int ( *fn ) ( void * ) );

/*
 * Waits for thread to terminate before returning.
 */
extern int _alWaitThread( ThreadID thread );

/*
 * Returns the identifier for the callee's thread.
 */
extern unsigned int _alSelfThread( void );

/*
 * Forces the callee to terminate.
 */
extern void _alExitThread( void ) AL_ATTRIBUTE_NORETURN_;

#endif /* not AL_AL_THREADLIB_H_ */
