/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_threadlib.c
 */

#include "al_siteconfig.h"
#include <stdlib.h>
#include "al_threadlib.h"

#if defined(USE_POSIXTHREADING)

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

	if( pthread_create( thread, &type, (void *(*) (void *)) fn, NULL ) != 0 ) {
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
#endif
