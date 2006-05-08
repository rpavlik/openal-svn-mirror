#include "al_siteconfig.h"

#include <AL/al.h>
#include <math.h>
#include <stdlib.h>

#include "al_debug.h"
#include "al_types.h"
#include "al_main.h"
#include "al_matrix.h"

/*
 * Multiplies transposed vector v by matrix provided as 3x3 array,
 * populating result.
 */
void _alVecMatrixMulA3( ALfloat result[3], ALfloat v[3], ALfloat m[3][3] ) {

	result[0] = v[0] * m[0][0] + v[1] * m[1][0] + v[2] * m[2][0];
	result[1] = v[0] * m[0][1] + v[1] * m[1][1] + v[2] * m[2][1];
	result[2] = v[0] * m[0][2] + v[1] * m[1][2] + v[2] * m[2][2];

	return;
}

#if 0
/*
 * _alMatrixMul( ALmatrix *result, ALmatrix *m1, ALmatrix *m2 )
 *
 * Multiplies m1 by m2, populating result.
 *
 *  FIXME: please check my math
 */
void _alMatrixMul( ALmatrix *result, ALmatrix *m1, ALmatrix *m2 ) {
	int m2cols = m2->cols;
	int m1rows = m1->rows;
	int m1cols = m1->cols;
	int i;
	int j;
	int k;

	ALfloat sum;

	for(i = 0; i < m2cols; i++) {
		for(j = 0; j < m1rows; j++) {
			sum = 0.0f;

			for(k = 0; k < m1cols; k++) {
				sum += m1->data[j][k] * m2->data[k][i];
			}

			result->data[j][i] = sum;
		}
	}

	return;
}

/*
 * _alMatrixAlloc( int rows, int cols )
 *
 * Allocates, initializes, and returns a matrix with the dimensions matching
 * the passed arguments, or NULL on error.
 */
ALmatrix *_alMatrixAlloc(int rows, int cols) {
	ALmatrix *retval;
	int i;

	retval = malloc(sizeof *retval);
	if(retval == NULL) {
		return NULL;
	}

	retval->data = malloc(rows * sizeof *retval->data);
	if(retval->data == NULL) {
		return NULL;
	}

	for(i = 0; i < rows; i++) {
		/* FIXME: clean return on error */
		retval->data[i] = malloc(cols * sizeof *retval->data[i]);
	}

	retval->rows = rows;
	retval->cols = cols;

	return retval;
}

/*
 * _alMatrixFree( ALmatrix *m )
 *
 * Frees a matrix allocated with _alMatrixAlloc.
 */
void _alMatrixFree(ALmatrix *m) {
	int i;

	if(m == NULL) {
		return;
	}

	for(i = 0; i < m->rows; i++) {
		free( m->data[i] );
	}

	free(m->data);
	free(m);

	return;
}
#endif /* 0 */
