/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * morphosthreads.h
 *
 * MorphOS thread backend prototypes.
 */
#ifndef MORPHOS_THREADS_H_
#define MORPHOS_THREADS_H_

#include <exec/ports.h>
#include <dos/dosextens.h>

struct ThreadStartMsg
{
  struct Message  tsm_Msg;
  int					tsm_Result;
};

struct ThreadData
{
	struct Process* td_Thread;
	struct MsgPort* td_MsgPort;
};

/*
 * typedef our ThreadID type.
 */
typedef struct ThreadData *ThreadID;

/*
 * _alCreateThread( int (*fn )(void *) )
 *
 * Creates a thread, which starts by running fn.
 */
extern struct ThreadData *_alCreateThread( int (*fn )(void *) );

/*
 * _alWaitThread( struct ThreadData *waitfor )
 *
 * Waits for waitfor to terminate before returning.
 */
extern int _alWaitThread( struct ThreadData *waitfor );

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

#endif /* MORPHOS_THREADS_H_ */
