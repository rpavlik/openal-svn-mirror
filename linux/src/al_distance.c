/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_distance.c
 *
 * Implementation of distance models.
 */
#include "al_siteconfig.h"

#include "al_distance.h"
#include "al_main.h"

/*
 * No distance attenuation.  Which means we return 1.0 for all values.
 */
static ALfloat
noAttenuation( UNUSED(ALfloat dist),
	       UNUSED(ALfloat rolloff),
	       ALfloat gain,
	       UNUSED(ALfloat ref),
	       UNUSED(ALfloat max))
{
	return gain;
}

/*
 * We are using the linear adaptation of the spec's formula, because we are
 * dealing with all linear gains at this point:
 *
 * G = GAIN * REF / ( REF + ROLLOFF * ( dist - REF ) );
 */
static ALfloat
inverseDistance( ALfloat dist,
		 ALfloat rolloff,
		 ALfloat gain,
		 ALfloat ref,
		 UNUSED(ALfloat max))
{
	ALfloat retval;

	/* dist = MAX( dist, ref ) */
	if( dist < ref ) {
		dist = ref;
	}

	/* formula, expressed in linear terms */
	retval = gain * ref / ( ref + rolloff * ( dist - ref ) );

	if( retval < 0.0 ) {
		return 0.0;
	} else if( retval > 1.0 ) {
		return 1.0;
	}

	return retval;
}

static ALfloat
inverseDistanceClamped( ALfloat dist,
			ALfloat rolloff,
			ALfloat gain,
			ALfloat ref,
			ALfloat max )
{
	ALfloat retval;

	/* dist = MAX( dist, ref ) */
	if( dist < ref ) {
		dist = ref;
	}

	/* dist = MIN( dist, max ) */
	if( dist > max ) {
		dist = max;
	}

	/* linear gain formula */
	retval = gain * ref / ( ref + rolloff * ( dist - ref ) );

	return retval;
}

/* TODO */
static ALfloat
linearDistance( UNUSED(ALfloat dist),
		UNUSED(ALfloat rolloff),
		ALfloat gain,
		UNUSED(ALfloat ref),
		UNUSED(ALfloat max))
{
	return gain;
}

/* TODO */
static ALfloat
linearDistanceClamped( UNUSED(ALfloat dist),
		       UNUSED(ALfloat rolloff),
		       ALfloat gain,
		       UNUSED(ALfloat ref),
		       UNUSED(ALfloat max))
{
	return gain;
}

/* TODO */
static ALfloat
exponentDistance( UNUSED(ALfloat dist),
		  UNUSED(ALfloat rolloff),
		  ALfloat gain,
		  UNUSED(ALfloat ref),
		  UNUSED(ALfloat max))
{
	return gain;
}

/* TODO */
static ALfloat
exponentDistanceClamped( UNUSED(ALfloat dist),
			 UNUSED(ALfloat rolloff),
			 ALfloat gain,
			 UNUSED(ALfloat ref),
			 UNUSED(ALfloat max))
{
	return gain;
}

void
_alUpdateDistanceModel( AL_context *cc )
{
	switch( cc->distance_model) {
	case AL_NONE:
		cc->distance_func = noAttenuation;
		break;
	case AL_INVERSE_DISTANCE:
		cc->distance_func = inverseDistance;
		break;
	case AL_INVERSE_DISTANCE_CLAMPED:
		cc->distance_func = inverseDistanceClamped;
		break;
	case AL_LINEAR_DISTANCE:
		cc->distance_func = linearDistance;
		break;
	case AL_LINEAR_DISTANCE_CLAMPED:
		cc->distance_func = linearDistanceClamped;
		break;
	case AL_EXPONENT_DISTANCE:
		cc->distance_func = exponentDistance;
		break;
	case AL_EXPONENT_DISTANCE_CLAMPED:
		cc->distance_func = exponentDistanceClamped;
		break;
	default:
		/* should never happen, just to be safe */
		cc->distance_func = inverseDistanceClamped;
		break;
	}
}
