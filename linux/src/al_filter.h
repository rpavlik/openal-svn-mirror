/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_filter.h
 *
 * Prototypes, macros and definitions related to the application of filters.
 *
 */
#ifndef _AL_FILTER_H_
#define _AL_FILTER_H_

#include "al_siteconfig.h"

#include <AL/al.h>

#include "al_types.h"

/*
 * Initialize data structures needed for the management of time-domain
 * filters.
 */
void _alInitTimeFilters( time_filter_set *tf_ptr_ref );

/*
 * Apply all filters for the source specified by sid in the context specified
 * by cid.
 */
void _alApplyFilters( ALuint cid, ALuint sid );

/*
 * Destroy all filter data structures allocated.
 */
void _alDestroyFilters( void );

/*
 * time domain filters
 */

/*
 * Reverb filter.  Not well defined.
 */
time_filter alf_reverb;

/*
 * Coning filter.  Carries out the coning attenuation specified by
 * AL_CONE_INNER_ANGLE, AL_CONE_OUTER_ANGLE, and AL_CONE_OUTER_GAIN.  Closely
 * related to alf_da ( the distance attenuation filter ), but they are
 * mutually exclusive.
 */
time_filter alf_coning;

/*
 * Distance attenuation filter.  Applies distance-model specific attenuation
 * to a source.
 */
time_filter alf_da;

/*
 * Pitch filter.  Applies a pitch shift ( in the time domain ) to a source.
 */
time_filter alf_tpitch;

/*
 * Doppler filter.  Applies a doppler shift ( in the time domain ) to a source.
 */
time_filter alf_tdoppler;

/*
 * Clamps a source at its min/max gain settings, specified by AL_MIN_GAIN and
 * AL_MAX_GAIN.
 */
time_filter alf_minmax;

/*
 * Panning filter.
 */
time_filter alf_panning;

/*
 * Applies the listener gain to a source.
 */
time_filter alf_listenergain;

/* macros */
#define _alDCApplyFilters(s) 	_alApplyFilters(_alcCCId, s)

#endif /* AL_FILTER_H_ */
