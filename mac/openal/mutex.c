/**
 * OpenAL cross platform audio library
 * Copyright (C) 1999-2000 by authors.
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA  02111-1307, USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */

#include <stdlib.h>
#include "mutex.h"

MutexID mix_mutex = NULL; 

pthread_mutex_t *_newMutex(void) {
	pthread_mutex_t *retval = malloc(sizeof *retval);

	return retval;
}

pthread_mutex_t *mlCreateMutex(void){
	pthread_mutex_t *retval = _newMutex();

	if(pthread_mutex_init(retval, NULL) != 0) {
		free(retval);
		return NULL;
	}

	return retval;
}

void mlDestroyMutex(pthread_mutex_t *mutex) {
	if(!mutex)
	{
		return;
	}

	if(pthread_mutex_destroy(mutex)) {
		assert(0);
		return;
	}

	free(mutex);

	return;
}

void LockBufs( void ) {
	if(!mix_mutex)
	{
		return;
	}
	pthread_mutex_lock(mix_mutex);
} 

void UnlockBufs( void ) {
	if(!mix_mutex)
	{
		return;
	}
	pthread_mutex_unlock(mix_mutex);
}

int mlTryLockMutex(void){
	/*if(!mix_mutex)
	{
		return -1;
	}
	if(pthread_mutex_trylock(mix_mutex) == EBUSY) {
		return -1;
	}
	return 1;
	*/
	return pthread_mutex_lock(mix_mutex);
}

/*int mlTryLockMutex(pthread_mutex_t *mutex) {
	if(!mutex)
	{
		return -1;
	}

	if(pthread_mutex_trylock(mutex) == EBUSY) {
		return -1;
	}

	return 0;
}*/