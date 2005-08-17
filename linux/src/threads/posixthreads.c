/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * posixthreads.c
 *
 * posix thread implementation.
 */
#include "al_siteconfig.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "posixthreads.h"

#include "al_main.h"

typedef int (*ptfunc)(void *);

static void *RunThread(void *data) {
	ptfunc fn;

	fn = (ptfunc) data;

	fn(NULL);

	pthread_exit(NULL);
	return NULL;		/* Prevent compiler warning */
}


pthread_t *_alCreateThread(int (*fn)(void *)) {
	pthread_attr_t type;
	pthread_t *retval;

	retval = malloc(sizeof *retval);
	if(retval == NULL) {
		return NULL;
	}

	if(pthread_attr_init(&type) != 0) {
		free(retval);
		fprintf(stderr, "Couldn't pthread_attr_init\n");
		return NULL;
	}

	pthread_attr_setdetachstate(&type, PTHREAD_CREATE_JOINABLE);

	if(pthread_create(retval, &type, RunThread, (void *) fn) != 0) {
		fprintf(stderr, "No CreateThread\n");
		free(retval);
		return NULL;
	}

	return retval;
}

int _alWaitThread(pthread_t *waitfor) {
	int retval = -1;

	if(waitfor == NULL) {
		return -1;
	}

	retval = pthread_join(*waitfor, NULL);

	free(waitfor);

	return retval;
}

unsigned int _alSelfThread(void) {
	return (unsigned int) pthread_self();
}


void _alExitThread(void) {
	pthread_exit( NULL );
}
