/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_listen.h
 *
 * Prototypes, macros and definitions related to the management of listeners.
 *
 */
#ifndef AL_AL_LISTEN_H_
#define AL_AL_LISTEN_H_

#include "al_siteconfig.h"
#include "al_types.h"

/*
 * Initializes a listener to the default values for all its elements.
 */
void _alInitListener( AL_listener *listener );

/*
 * Performs any needed finalization on a listener.
 */
void _alDestroyListener( AL_listener *listener );

#endif /* not AL_AL_LISTEN_H_ */
