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
	       ALfloat sourceGain,
	       UNUSED(ALfloat referenceDistance),
	       UNUSED(ALfloat maxDistance))
{
	return sourceGain;
}

static ALfloat
inverseDistance( ALfloat distance,
		 ALfloat rolloffFactor,
		 ALfloat sourceGain,
		 ALfloat referenceDistance,
		 ALfloat maxDistance)
{
	ALfloat distDiff;

	if ((maxDistance <= referenceDistance) || (referenceDistance == 0.0f)) {
		return sourceGain;
	}

	distDiff = referenceDistance + (rolloffFactor * (distance - referenceDistance));
	if (distDiff <= 0.0f) {
		return sourceGain * 1000000.0f;
	}

	return sourceGain * referenceDistance / distDiff;
}

static ALfloat
inverseDistanceClamped( ALfloat distance,
			ALfloat rolloffFactor,
			ALfloat sourceGain,
			ALfloat referenceDistance,
			ALfloat maxDistance )
{
	ALfloat distDiff;

	if ((maxDistance <= referenceDistance) || (referenceDistance == 0.0f)) {
		return sourceGain;
	}

	/* distance = MAX( distance, referenceDistance ) */
	if( distance < referenceDistance ) {
		distance = referenceDistance;
	}

	/* distance = MIN( distance, maxDistance ) */
	if( distance > maxDistance ) {
		distance = maxDistance;
	}

	distDiff = referenceDistance + (rolloffFactor * (distance - referenceDistance));
	if (distDiff <= 0.0f) {
		return sourceGain * 1000000.0f;
	}

	return sourceGain * referenceDistance / distDiff;
}

static ALfloat
linearDistance( ALfloat distance,
		ALfloat rolloffFactor,
		ALfloat sourceGain,
		ALfloat referenceDistance,
		ALfloat maxDistance)
{
	if (maxDistance <= referenceDistance) {
		return sourceGain;
	}

	return sourceGain * (1 - rolloffFactor * (distance - referenceDistance) / (maxDistance - referenceDistance));
}

static ALfloat
linearDistanceClamped( ALfloat distance,
		       ALfloat rolloffFactor,
		       ALfloat sourceGain,
		       ALfloat referenceDistance,
		       ALfloat maxDistance)
{
	if (maxDistance <= referenceDistance) {
		return sourceGain;
	}

	/* distance = MAX( distance, referenceDistance ) */
	if( distance < referenceDistance ) {
		distance = referenceDistance;
	}

	/* distance = MIN( distance, maxDistance ) */
	if( distance > maxDistance ) {
		distance = maxDistance;
	}

	return sourceGain * (1 - rolloffFactor * (distance - referenceDistance) / (maxDistance - referenceDistance));
}

static ALfloat
exponentDistance( ALfloat distance,
		  ALfloat rolloffFactor,
		  ALfloat sourceGain,
		  ALfloat referenceDistance,
		  ALfloat maxDistance)
{
	if ((maxDistance <= referenceDistance) || (referenceDistance == 0.0f) || (distance == 0.0f)) {
		return sourceGain;
	}

	return sourceGain * ((ALfloat)pow(distance / referenceDistance, -rolloffFactor));
}

static ALfloat
exponentDistanceClamped( ALfloat distance,
			 ALfloat rolloffFactor,
			 ALfloat sourceGain,
			 ALfloat referenceDistance,
			 ALfloat maxDistance)
{
	if ((maxDistance <= referenceDistance) || (referenceDistance == 0.0f)) {
		return sourceGain;
	}

	/* distance = MAX( distance, referenceDistance ) */
	if( distance < referenceDistance ) {
		distance = referenceDistance;
	}

	/* distance = MIN( distance, maxDistance ) */
	if( distance > maxDistance ) {
		distance = maxDistance;
	}

	if (distance == 0.0f) {
		return sourceGain;
	}

	return sourceGain * ((ALfloat)pow(distance / referenceDistance, -rolloffFactor));
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
