/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_listen.h
 *
 * Prototypes, macros and definitions related to the management of listeners.
 *
 */
#ifndef _AL_LISTENER_H_
#define _AL_LISTENER_H_

#include "al_types.h"

/*
 * _alInitListener( AL_listener *listener )
 *
 * Initializes a listener to the default values for all its elements.
 */
void _alInitListener( AL_listener *listener );

/*
 * _alDestroyListener( AL_listener *listener )
 *
 * Performs any needed finalization on a listener.
 */
void _alDestroyListener( AL_listener *listener );

#endif /* _AL_LISTENER_H_ */
