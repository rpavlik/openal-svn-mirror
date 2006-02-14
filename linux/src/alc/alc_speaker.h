/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * alc_speaker.h
 *
 * Prototypes, defines etc for speaker management.
 */
#ifndef ALC_SPEAKER_H_
#define ALC_SPEAKER_H_

#include "al_siteconfig.h"

#include <AL/al.h>

#include "alc/alc_context.h"

typedef enum {
	ALS_LEFT,
	ALS_RIGHT,
	ALS_LEFTS,
	ALS_RIGHTS
} _alcSpeakerEnum;

/*
 *  Updates the speaker setup for the context named by cid for changes in
 *  listener position and orientation.
 */
void _alcSpeakerMove( ALuint cid );

/*
 * Returns the 3-tuple position (x/y/z) of the speaker enumerated by
 * speaker_num for the context named by cid, or NULL if cid does not name a
 * context.
 */
ALfloat *_alcGetSpeakerPosition( ALuint cid, ALuint speaker_num );

/*
 * Returns the number of speakers associated with the context named by cid.
 */
ALuint _alcGetNumSpeakers( ALuint cid );

/* default context macros */
#define _alcDCSpeakerMove()     _alcSpeakerMove(_alcCCId)
#define _alcDCGetNumSpeakers()  _alcGetNumSpeakers(_alcCCId)

#endif /* _ALC_SPEAKERS_H_ */
