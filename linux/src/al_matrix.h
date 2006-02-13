/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_vector.h
 *
 * matrix math stuff
 */

#ifndef AL_MATRIX_H_
#define AL_MATRIX_H_

/*
 * Multiplies transposed vector v by matrix provided as 3x3 array,
 * populating result.
 */
void _alVecMatrixMulA3( ALfloat result[3], ALfloat v[3], ALfloat m[3][3] );

#if 0
/*
 * Allocates, initializes, and returns a matrix with the dimensions matching
 * the passed arguments, or NULL on error.
 */
ALmatrix *_alMatrixAlloc( int rows, int cols );

/*
 * Frees a matrix allocated with _alMatrixAlloc.
 */
void _alMatrixFree( ALmatrix *m );

/*
 * Multiplies two matrices and places the result in result.
 */
void _alMatrixMul( ALmatrix *result, ALmatrix *m1, ALmatrix *m2 );
#endif /* 0 */

#endif /* AL_MATRIX_H_ */
