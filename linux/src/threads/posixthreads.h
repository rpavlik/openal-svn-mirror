/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * posixthreads.h
 *
 * Posix thread backend prototypes.
 */
#ifndef POSIX_THREADS_H_
#define POSIX_THREADS_H_

#include <pthread.h>

/*
 * typedef our ThreadID type.
 */
typedef pthread_t *ThreadID;

/*
 * _alCreateThread( int (*fn )(void *) )
 *
 * Creates a thread, which starts by running fn.
 */
extern pthread_t *_alCreateThread( int (*fn )(void *) );

/*
 * _alWaitThread( pthread_t *waitfor )
 *
 * Waits for waitfor to terminate before returning.
 */
extern int _alWaitThread( pthread_t *waitfor );

/*
 * _alSelfThread( void )
 *
 * Returns the identifier for the callee's thread.
 */
extern unsigned int _alSelfThread( void );

/*
 * _alExitThread( void )
 *
 * Forces the callee to terminate.
 */
extern void _alExitThread( void );

#endif /* POSIX_THREADS_H_ */
