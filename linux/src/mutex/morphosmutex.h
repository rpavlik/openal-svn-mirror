/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * morphosmutex.h
 *
 * Header of MorphOS mutex implementation.
 */
#ifndef MORPHOS_MUTEXS_H_
#define MORPHOS_MUTEXS_H_

#include <exec/semaphores.h>

/*
 * typedef the MutexID type to struct Semaphore *.
 */
typedef struct SignalSemaphore *MutexID;

/*
 * MorphOS_CreateMutex( void )
 *
 * Creates a MorphOS mutex, returning it, or NULL on error.
 */
MutexID MorphOS_CreateMutex( void );

/*
 * MorphOS_DestroyMutex( MutexID mutex )
 *
 * Destroys the MorphOS mutex mutex.
 */
void MorphOS_DestroyMutex( MutexID mutex );

/*
 * MorphOS_LockMutex( MutexID mutex )
 *
 * Locks the MorphOS mutex mutex.
 */
void MorphOS_LockMutex( MutexID mutex );

/*
 * MorphOS_TryLockMutex( MutexID mutex )
 *
 * Returns 1 and locks this mutex if possible, 0 ( with no lock change) if
 * not.
 */
int MorphOS_TryLockMutex( MutexID mutex );

/*
 * MorphOS_UnlockMutex( MutexID mutex )
 *
 * Unlocks MorphOS mutex.
 */
void MorphOS_UnlockMutex( MutexID mutex );

#endif /* MORPHOS_MUTEXS_H_ */
