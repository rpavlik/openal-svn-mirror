#include <AL/altypes.h>

#include "al_siteconfig.h"
#include "al_debug.h"
#include "al_main.h"
#include "al_vector.h"

#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI		3.14159265358979323846	/* pi */
#endif /* M_PI */

/*
 * _alVectorMagnitudeAtZero( const ALfloat *origin, const ALfloat *v2 )
 *
 * Returns magnitude of v2 with origin at (0,0,0).
 *
 * FIXME: please check my math
 */
ALfloat _alVectorMagnitudeAtZero( const ALfloat *v2 ) {
	ALfloat retval;

	retval = sqrt( v2[0] * v2[0] +
		       v2[1] * v2[1] +
		       v2[2] * v2[2] );

	retval = fabs( retval );

	return retval;

}

/*
 * _alVectorMagnitude( const ALfloat *origin, const ALfloat *v2 )
 *
 * Returns magnitude of v2 with origin at origin.
 *
 * FIXME: please check my math
 */
ALfloat _alVectorMagnitude( const ALfloat *origin, const ALfloat *v2 ) {
	ALfloat lsav[3];
	ALfloat retval;

	_alVectorDistance( lsav, origin, v2 );

	retval = sqrt( lsav[0] * lsav[0] +
		       lsav[1] * lsav[1] +
		       lsav[2] * lsav[2] );

	retval = fabs( retval );

	return retval;

}

/*
 * _alVectorNormalize( ALfloat *d, const ALfloat *s)
 *
 * Normalizes s, placing result in d.
 */
void _alVectorNormalize( ALfloat *d, const ALfloat *s) {
	ALfloat mag;

	mag = _alVectorMagnitudeAtZero( s );

	if( mag == 0 ) {
		d[0] = 0.0; d[1] = 0.0; d[2] = 0.0;

		return;
	}

	d[0] = s[0] / mag;
	d[1] = s[1] / mag;
	d[2] = s[2] / mag;

	return;
}

/*
 * _alVectorDistance( ALfloat *retref, const ALfloat *v1,
 *				     const ALfloat *v2 )
 *
 * Places distance between two vectors in retref.
 */
void _alVectorDistance( ALfloat *retref, const ALfloat *v1,
			const ALfloat *v2 ) {
	float fi1, fi2;
	int i;

	for( i = 0; i < 3; i++ ) {
		fi1 = v1[i];
		fi2 = v2[i];

		if( fi1 < fi2 ) {
			retref[i] = fi2 - fi1;
		} else {
			retref[i] = fi1 - fi2;
		}
	}

	return;
}

/*
 * _alVectorTranslate( ALfloat *d, const ALfloat *s,
 *		     const ALfloat *delta )
 *
 * translate s by delta, result in d
 */
void _alVectorTranslate( ALfloat *d, const ALfloat *s,
			 const ALfloat *delta ) {
	d[0] = s[0] + delta[0];
	d[1] = s[1] + delta[1];
	d[2] = s[2] + delta[2];

	return;
}

/*
 * _alVectorInverse( ALfloat *d, const ALfloat *s )
 *
 * place inverse of s in d
 */
void _alVectorInverse( ALfloat *d, const ALfloat *s ) {
	d[0] = -s[0];
	d[1] = -s[1];
	d[2] = -s[2];

	return;
}

/*
 * _alVectorAngleBetween( const ALfloat *origin, const ALfloat *v1,
 *                       const ALfloat *v2 )
 *
 * Returns the angle between two vectors, with origins at origin.
 *
 * FIXME: please check my math
 */
ALfloat _alVectorAngleBetween( const ALfloat *origin, const ALfloat *v1,
                              const ALfloat *v2 ) {
	ALfloat m1;     /* |v2| */
	ALfloat m2;     /* |v1| */
	ALfloat mt;     /* |v1| * |v2| */
	ALfloat dp;    /*  dot product */
	ALfloat mag;

	m1  = _alVectorMagnitude( origin, v1 );
	m2  = _alVectorMagnitude( origin, v2 );
	dp  = _alVectorDotp( origin, v1, v2 );

	mt = m1 * m2;
	if(mt == 0.0f) {
		return M_PI / 2.0f;
	}

	mag = acos( dp / mt );

	return mag;
}

/*
 * _alVectorDotp( const ALfloat *origin, const ALfloat *v1,
 *		const ALfloat *v2 )
 *
 * Returns dot product between v1 and v2, with origin at origin.
 */
ALfloat _alVectorDotp( const ALfloat *origin, const ALfloat *v1,
		       const ALfloat *v2 ) {
	ALfloat o_inverse[3];
	ALfloat v1_trans[3];
	ALfloat v2_trans[3];
	ALfloat retval = 0.0f;

	_alVectorInverse( o_inverse, origin );

	_alVectorTranslate( v1_trans, v1, o_inverse );
	_alVectorTranslate( v2_trans, v2, o_inverse );

	retval += v1_trans[0] * v2_trans[0];
	retval += v1_trans[1] * v2_trans[1];
	retval += v1_trans[2] * v2_trans[2];

	return retval;
}

/*
 * _alVectorCrossProduct( ALfloat *d, const ALfloat *v1,
 *			const ALfloat *v2 )
 *
 * Returns cross product between v1 and v2, result in d.
 */
void _alVectorCrossProduct( ALfloat *d, const ALfloat *v1,
			      const ALfloat *v2 ) {
	d[0] = v1[1] * v2[2] - v1[2] * v2[1];
	d[1] = v1[2] * v2[0] - v1[0] * v2[2];
	d[2] = v1[0] * v2[1] - v1[1] * v2[0];

	return;
}
