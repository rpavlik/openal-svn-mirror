/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * posixmutex.c
 *
 * posix mutex backend for out mutex lib interface.
 */
#include "al_siteconfig.h"
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "posixmutex.h"

static pthread_mutex_t *_newMutex(void);

pthread_mutex_t *Posix_CreateMutex(void) {
	pthread_mutex_t *retval = _newMutex();

	if(pthread_mutex_init(retval, NULL) != 0) {
		free(retval);
		return NULL;
	}

	return retval;
}

void Posix_DestroyMutex(pthread_mutex_t *mutex) {
	if(!mutex)
	{
		return;
	}

	if(pthread_mutex_destroy(mutex)) {
		fprintf(stderr, "mutex %p busy\n", (void *) mutex);
		assert(0);
		return;
	}

	free(mutex);

	return;
}


int Posix_TryLockMutex(pthread_mutex_t *mutex) {
	if(!mutex)
	{
		return -1;
	}

	if(pthread_mutex_trylock(mutex) == EBUSY) {
		return -1;
	}

	return 0;
}

void Posix_LockMutex(pthread_mutex_t *mutex) {
	if(!mutex)
	{
		return;
	}

	pthread_mutex_lock(mutex);
}

void Posix_UnlockMutex(pthread_mutex_t *mutex) {
	if(!mutex)
	{
		return;
	}

	pthread_mutex_unlock(mutex);
}

static pthread_mutex_t *_newMutex(void) {
	pthread_mutex_t *retval = malloc(sizeof *retval);

	return retval;
}
