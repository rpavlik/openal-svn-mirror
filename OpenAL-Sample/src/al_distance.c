/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_distance.c
 *
 * Implementation of distance models.
 */
#include "al_siteconfig.h"

#include <math.h>

#include "al_distance.h"
#include "al_main.h"

/*
 * No distance attenuation.  Which means we return 1.0 for all values.
 */
static ALfloat
noAttenuation( UNUSED(ALfloat distance),
	       UNUSED(ALfloat rolloffFactor),
	       UNUSED(ALfloat referenceDistance),
	       UNUSED(ALfloat maxDistance))
{
	return 1.0f;
}

static ALfloat
inverseDistance( ALfloat distance,
		 ALfloat rolloffFactor,
		 ALfloat referenceDistance,
		 UNUSED(ALfloat maxDistance))
{
	/*
	 * At this distance, the gain formula has a singularity and smaller
	 * distances would result in a negative gain. In these cases, the spec
	 * explicitly requires "no attenuation".
	 */
	ALfloat critDist = referenceDistance + (rolloffFactor * (distance - referenceDistance));
	if (critDist <= 0.0f) {
		return 1.0f;
	}

	return referenceDistance / critDist;
}

static ALfloat
inverseDistanceClamped( ALfloat distance,
			ALfloat rolloffFactor,
			ALfloat referenceDistance,
			ALfloat maxDistance )
{
	ALfloat critDist;

	if (maxDistance <= referenceDistance) {
		return 1.0;
	}

	/* distance = MAX( distance, referenceDistance ) */
	if( distance < referenceDistance ) {
		distance = referenceDistance;
	}

	/* distance = MIN( distance, maxDistance ) */
	if( distance > maxDistance ) {
		distance = maxDistance;
	}

	/*
	 * At this distance, the gain formula has a singularity and smaller
	 * distances would result in a negative gain. In these cases, the spec
	 * explicitly requires "no attenuation".
	 */
	critDist = referenceDistance + (rolloffFactor * (distance - referenceDistance));
	if (critDist <= 0.0f) {
		return 1.0f;
	}

	return referenceDistance / critDist;
}

static ALfloat
linearDistance( ALfloat distance,
		ALfloat rolloffFactor,
		ALfloat referenceDistance,
		ALfloat maxDistance)
{
	if (maxDistance <= referenceDistance) {
		return 1.0f;
	}

	return 1.0f - rolloffFactor * (distance - referenceDistance) / (maxDistance - referenceDistance);
}

static ALfloat
linearDistanceClamped( ALfloat distance,
		       ALfloat rolloffFactor,
		       ALfloat referenceDistance,
		       ALfloat maxDistance)
{
	if (maxDistance <= referenceDistance) {
		return 1.0f;
	}

	/* distance = MAX( distance, referenceDistance ) */
	if( distance < referenceDistance ) {
		distance = referenceDistance;
	}

	/* distance = MIN( distance, maxDistance ) */
	if( distance > maxDistance ) {
		distance = maxDistance;
	}

	return 1.0f - rolloffFactor * (distance - referenceDistance) / (maxDistance - referenceDistance);
}

static ALfloat
exponentDistance( ALfloat distance,
		  ALfloat rolloffFactor,
		  ALfloat referenceDistance,
		  UNUSED(ALfloat maxDistance))
{
	ALfloat quotient;

	if (referenceDistance == 0.0f) {
		return 1.0f;
	}

	quotient = distance / referenceDistance;
	if (quotient == 0.0f) {
		return 1.0f;
	}

	return (ALfloat)pow(quotient, -rolloffFactor);
}

static ALfloat
exponentDistanceClamped( ALfloat distance,
			 ALfloat rolloffFactor,
			 ALfloat referenceDistance,
			 ALfloat maxDistance)
{
	ALfloat quotient;

	if ((maxDistance <= referenceDistance) || (referenceDistance == 0.0f)) {
		return 1.0f;
	}

	/* distance = MAX( distance, referenceDistance ) */
	if( distance < referenceDistance ) {
		distance = referenceDistance;
	}

	/* distance = MIN( distance, maxDistance ) */
	if( distance > maxDistance ) {
		distance = maxDistance;
	}

	quotient = distance / referenceDistance;
	if (quotient == 0.0f) {
		return 1.0f;
	}

	return (ALfloat)pow(quotient, -rolloffFactor);
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
