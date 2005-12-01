/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_threadlib.c
 */

#include <stdlib.h>
#include "al_threadlib.h"

#if defined(USE_POSIXTHREADING)

typedef int ( *ptfunc ) ( void * );

static void *runThread( void *data )
{
	ptfunc fn = ( ptfunc ) data;
	fn( NULL );
	pthread_exit( NULL );
	return NULL;		/* Prevent compiler warning */
}

ThreadID _alCreateThread( int ( *fn ) ( void * ) )
{
	pthread_attr_t type;
	pthread_t *thread = malloc( sizeof *thread );
	if( thread == NULL ) {
		return NULL;
	}

	if( pthread_attr_init( &type ) != 0 ) {
		free( thread );
		return NULL;
	}

	pthread_attr_setdetachstate( &type, PTHREAD_CREATE_JOINABLE );

	if( pthread_create( thread, &type, runThread, ( void * ) fn ) != 0 ) {
		free( thread );
		return NULL;
	}

	return thread;
}

int _alWaitThread( ThreadID thread )
{
	int retval = pthread_join( *thread, NULL );
	free( thread );
	return retval;
}

unsigned int _alSelfThread( void )
{
	return ( unsigned int ) pthread_self(  );
}

void _alExitThread( void )
{
	pthread_exit( NULL );
}

#elif defined(USE_WINDOWSTHREADING)

/* for _alMicroSleep */
#include "al_main.h"

typedef int ( *ptfunc ) ( void * );

ThreadID _alCreateThread( int ( *fn ) ( void * ) )
{
	DWORD dummy;
	return CreateThread( NULL, 0, ( LPTHREAD_START_ROUTINE ) fn, NULL, 0,
			     &dummy );
}

int _alWaitThread( ThreadID thread )
{
	int tries = 20;		/* gets tries iterations before we nuke it */
	const int interval = 40000;

	do {
		DWORD ExitCode;
		if( GetExitCodeThread( thread, &ExitCode ) == 0 ) {
			break;
		}

		/* thread is still running, be nice and wait a bit */
		_alMicroSleep( interval );
	} while( tries-- );

	return ( TerminateThread( thread, 0 ) == 0 ) ? -1 : 0;
}

void _alExitThread( void )
{
	ExitThread( 0 );
}

unsigned int _alSelfThread( void )
{
	return ( unsigned int ) GetCurrentThreadId(  );
}

#elif defined(USE_MORPHOSTHREADING)

#include <dos/dostags.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "al_types.h"

struct ThreadStartMsg {
	struct Message tsm_Msg;
	int tsm_Result;
};

ThreadID _alCreateThread( int ( *fn ) ( void * ) )
{
	struct ThreadData *thread =
	    AllocVec( sizeof( struct ThreadData ), MEMF_PUBLIC );
	if( thread == NULL ) {
		return NULL;
	}

	thread->td_MsgPort = CreateMsgPort(  );
	if( !thread->td_MsgPort ) {
		FreeVec( thread );
		return NULL;
	}

	struct ThreadStartMsg *startup_msg =
	    AllocVec( sizeof( struct ThreadStartMsg ),
		      MEMF_PUBLIC | MEMF_CLEAR );
	if( startup_msg == NULL ) {
		DeleteMsgPort( thread->td_MsgPort );
		FreeVec( thread );
		return NULL;
	}

	startup_msg->tsm_Msg.mn_Node.ln_Type = NT_MESSAGE;
	startup_msg->tsm_Msg.mn_ReplyPort = thread->td_MsgPort;
	startup_msg->tsm_Msg.mn_Length = sizeof( *startup_msg );

	thread->td_Thread =
	    CreateNewProcTags( NP_Entry, ( ULONG ) fn, NP_Name, ( ULONG )
			       "OpenAL Thread",
			       NP_CodeType,
			       CODETYPE_PPC,
			       NP_StartupMsg,
			       ( ULONG ) startup_msg,
			       NP_PPC_Arg1, 0, TAG_DONE );
	if( !thread->td_Thread ) {
		FreeVec( startup_msg );
		DeleteMsgPort( thread->td_MsgPort );
		FreeVec( thread );
		return NULL;
	}

	return thread;
}

int _alWaitThread( ThreadID thread )
{
	struct ThreadStartMsg *tsm;
	int retval;
	WaitPort( thread->td_MsgPort );
	tsm = ( struct ThreadStartMsg * ) GetMsg( thread->td_MsgPort );
	DeleteMsgPort( thread->td_MsgPort );
	retval = tsm->tsm_Result;
	FreeVec( tsm );
	FreeVec( thread );
	return retval;
}

unsigned int _alSelfThread( void )
{
	return ( unsigned int ) FindTask( NULL );
}

void _alExitThread( void )
{
	struct ThreadStartMsg *msg;
	if( NewGetTaskAttrs( NULL, &msg, sizeof( msg ), TASKINFOTYPE_STARTUPMSG,
			     TAG_DONE ) && msg )
		msg->tsm_Result = 0;
	/*RemTask(NULL); */
}

#endif
