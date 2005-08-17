/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * windowsthreads.h
 *
 * Windows thread backend prototypes.
 */
#ifndef WINDOWS_THREADS_H_
#define WINDOWS_THREADS_H_

#include <windows.h>

/*
 * typedef our ThreadID type.
 */
typedef HANDLE ThreadID;

/*
 * _alCreateThread( int (*fn)(void *), void *data )
 */
extern ThreadID _alCreateThread( int (*fn)(void *), void *data );

/*
 * _alWaitThread( ThreadID waitfor )
 *
 * Wait for waitfor to terminate of natural causes.
 */
extern int _alWaitThread( ThreadID waitfor );

/*
 * _alSelfThread( void )
 *
 * Returns the identifier for the callee's thread.
 */
extern unsigned int _alSelfThread( void );

/*
 * _alExitThread( void )
 *
 * Terminate the callee's thread.
 */
extern void _alExitThread( void );

#endif /* WINDOWS_THREADS_H_ */
