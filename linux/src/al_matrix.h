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

#endif /* AL_MATRIX_H_ */
