/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_vector.h
 *
 * kludgey vector math stuff
 */

#ifndef AL_AL_VECTOR_H_
#define AL_AL_VECTOR_H_

#include "al_siteconfig.h"
#include <math.h>

/*
 * Returns magnitude of v2 with origin at (0,0,0).
 */
static __inline ALfloat _alVectorMagnitudeAtZero( const ALfloat *v2 ) {
	ALfloat retval;

	retval = sqrt( v2[0] * v2[0] +
		       v2[1] * v2[1] +
		       v2[2] * v2[2] );

	retval = fabs( retval );

	return retval;
}

/*
 * Return magnitude of v with origin at origin.
 */
ALfloat _alVectorMagnitude( const ALfloat *origin, const ALfloat *v );

/*
 * Places distance between two vectors in retref.
 */
static __inline void _alVectorDistance( ALfloat *retref, const ALfloat *v1,
			const ALfloat *v2 ) {

	retref[0] = fabs( v1[0] - v2[0] );
	retref[1] = fabs( v1[1] - v2[1] );
	retref[2] = fabs( v1[2] - v2[2] );
	
	return;
}

/*
 * Rotate point about axis by angle.
 */
void _alRotatePointAboutAxis( const ALfloat angle, ALfloat *point,
                              const ALfloat *axis );

/*
 * Returns angle between two vectors with v1, v2 with shared origin.
 *
 * NOTE: Always positive
 */
ALfloat _alVectorAngleBetween( const ALfloat *origin, const ALfloat *v1,
                              const ALfloat *v2 );

/*
 * Returns dot product of v1 . v2 ( with shared origin )
 */
ALfloat _alVectorDotp( const ALfloat *origin, const ALfloat *v1,
                       const ALfloat *v2 );

/*
 * Translate s by delta, populating d.
 */
static __inline void _alVectorTranslate( ALfloat *d, const ALfloat *s,
			 const ALfloat *delta ) {
	d[0] = s[0] + delta[0];
	d[1] = s[1] + delta[1];
	d[2] = s[2] + delta[2];

	return;
}

/*
 * Populates d with vector inverse of s.
 */
static __inline void _alVectorInverse( ALfloat *d, const ALfloat *s ) {
	d[0] = -s[0];
	d[1] = -s[1];
	d[2] = -s[2];

	return;
}

/*
 * Normalizes s, places result in d.
 */
void _alVectorNormalize( ALfloat *d, const ALfloat *s );

/*
 * Returns cross product between v1 and v2, result in d.
 */
static __inline void _alVectorCrossProduct( ALfloat *d, const ALfloat *v1,
			      const ALfloat *v2 ) {
	d[0] = v1[1] * v2[2] - v1[2] * v2[1];
	d[1] = v1[2] * v2[0] - v1[0] * v2[2];
	d[2] = v1[0] * v2[1] - v1[1] * v2[0];

	return;
}

#endif /* not AL_AL_VECTOR_H_ */
