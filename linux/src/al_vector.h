/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_vector.h
 *
 * kludgey vector math stuff
 */

#ifndef AL_VECTOR_H_
#define AL_VECTOR_H_

/*
 * Returns magnitude of v2 with origin at (0,0,0).
 */
ALfloat _alVectorMagnitudeAtZero( const ALfloat *v2 );

/*
 * Return magnitude of v with origin at origin.
 */
ALfloat _alVectorMagnitude( const ALfloat *origin, const ALfloat *v );

/*
 * Return distance between s1 and s2, placing result in d.
 */
void _alVectorDistance( ALfloat *d, const ALfloat *s1, const ALfloat *s2 );

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
void _alVectorTranslate( ALfloat *d, const ALfloat *s, const ALfloat *delta );

/*
 * Populates d with vector inverse of s.
 */
void _alVectorInverse( ALfloat *d, const ALfloat *s );

/*
 * Normalizes s, places result in d.
 */
void _alVectorNormalize( ALfloat *d, const ALfloat *s );

/*
 * Returns cross product between v1 and v2, result in d.
 */
void _alVectorCrossProduct( ALfloat *d, const ALfloat *v1,
			      const ALfloat *v2 );

#endif /* AL_VECTOR_H */
