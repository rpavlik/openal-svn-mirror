/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * mutexlib.h
 *
 * Include that sorts out which mutex library we're using.
 */
#ifndef MUTEXLIB_H_
#define MUTEXLIB_H_

#include "al_siteconfig.h"

#if defined(USE_EMPTY_LOCKS)

#define mlCreateMutex()
#define mlDestroyMutex(m)
#define mlLockMutex(m)
#define mlTryLockMutex(m)
#define mlUnlockMutex(m)

#else

#if defined(USE_POSIXMUTEX)

#include <pthread.h>
typedef pthread_mutex_t *MutexID;

#elif defined(USE_WINDOWSMUTEX)

#include <windows.h>
typedef LPCRITICAL_SECTION MutexID;

#elif defined(USE_MORPHOSMUTEX)

#include <exec/semaphores.h>
typedef struct SignalSemaphore *MutexID;

#else

#error "No mutex package"

#endif

/* Creates a new mutex. Returns NULL on error. */
MutexID mlCreateMutex( void );

/* Destroys the given mutex, which must be unlocked. */
void mlDestroyMutex( MutexID mutex );

/* Locks the given mutex, blocking if it is already locked. */
void mlLockMutex( MutexID mutex );

/* Try to lock the given mutex, returning zero if it succeeded, non-zero
   otherwise. Non-blocking. */
int mlTryLockMutex( MutexID mutex );

/* Unlocks the given mutex, which must be locked by the same thread. */
void mlUnlockMutex( MutexID mutex );

#endif				/* USE_EMPTY_LOCKS */

#endif				/* MUTEXLIB_H_ */
