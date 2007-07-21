/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * mutexlib.c
 */

#include "al_siteconfig.h"
#include <stdlib.h>
#include <stdio.h>
#include "al_mutexlib.h"

#if defined(USE_POSIXTHREADING)

MutexID _alCreateMutex( void )
{
	pthread_mutexattr_t attrib;
	MutexID mutex = (MutexID)malloc(sizeof(*mutex));
	if(mutex == NULL)
		return NULL;

	if(pthread_mutexattr_init(&attrib) != 0) {
		free(mutex);
		return NULL;
	}

	if(pthread_mutexattr_settype(&attrib, PTHREAD_MUTEX_RECURSIVE) != 0)
		goto error;
	if(pthread_mutex_init(mutex, &attrib) != 0)
		goto error;

	pthread_mutexattr_destroy(&attrib);
	return mutex;

error:
	pthread_mutexattr_destroy(&attrib);
	free(mutex);
	return NULL;
}

void _alDestroyMutex( MutexID mutex )
{
	if( pthread_mutex_destroy( mutex ) != 0 ) {
		fprintf( stderr, "mutex %p busy\n", ( void * ) mutex );
		return;
	}
	free( mutex );
}

void _alLockMutex( MutexID mutex )
{
	pthread_mutex_lock( mutex );
}

int _alTryLockMutex( MutexID mutex )
{
	return ( pthread_mutex_trylock( mutex ) == 0 ) ? 0 : -1;
}

void _alUnlockMutex( MutexID mutex )
{
	pthread_mutex_unlock( mutex );
}

#elif defined(USE_WINDOWSTHREADING)

MutexID _alCreateMutex( void )
{
	MutexID mutex = ( MutexID ) malloc( sizeof( *mutex ) );
	if( mutex == NULL ) {
		return NULL;
	}
	InitializeCriticalSection( mutex );
	return mutex;
}

void _alDestroyMutex( MutexID mutex )
{
	DeleteCriticalSection( mutex );
}

void _alLockMutex( MutexID mutex )
{
	EnterCriticalSection( mutex );
}

int _alTryLockMutex( MutexID mutex )
{
	return ( TryEnterCriticalSection( mutex ) != 0 ) ? 0 : -1;
}

void _alUnlockMutex( MutexID mutex )
{
	LeaveCriticalSection( mutex );
}
#endif
