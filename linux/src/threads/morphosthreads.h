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
 * MorphOS_CreateThread( int (*fn )(void *), void *data )
 *
 * Creates a thread, which starts by running fn and passing data to it.
 */
extern struct ThreadData *MorphOS_CreateThread( int (*fn )(void *), void *data );

/*
 * MorphOS_WaitThread( struct ThreadData *waitfor )
 *
 * Waits for waitfor to terminate before returning.
 */
extern int MorphOS_WaitThread( struct ThreadData *waitfor );

/*
 * MorphOS_KillThread( struct ThreadData *killit )
 *
 * Kills the thread specified by killit.
 */
extern int MorphOS_KillThread( struct ThreadData *killit );

/*
 * MorphOS_SelfThread( void )
 *
 * Returns the identifier for the callee's thread.
 */
extern unsigned int MorphOS_SelfThread( void );

/*
 * MorphOS_ExitThread( int retval )
 *
 * Forces the callee to terminate.
 */
extern void MorphOS_ExitThread( int retval );

#endif /* MORPHOS_THREADS_H_ */
