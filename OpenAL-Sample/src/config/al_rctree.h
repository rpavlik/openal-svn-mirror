/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_rctree.h
 *
 * Stuff related to the rctree data structure
 *
 */
#ifndef AL_AL_RCTREE_H_
#define AL_AL_RCTREE_H_

#define ALRC_MAXSTRLEN 90

#include "al_siteconfig.h"
#include <stdlib.h>

/*
 * Each AL_rctree has a type, which reflects how its data should be
 * interpreted.  There are those types.
 */
typedef enum {
	ALRC_INVALID,
	ALRC_PRIMITIVE,
	ALRC_CONSCELL,
	ALRC_SYMBOL,
	ALRC_INTEGER,
	ALRC_FLOAT,
	ALRC_STRING,
	ALRC_BOOL,
	ALRC_POINTER
} ALRcEnum;

/*
 * The AL_rctree is the base type for the alrc language.
 */
typedef struct _AL_rctree {
	ALRcEnum type;

	union {
		ALboolean b;
		ALint   i;
		ALfloat f;
		struct {
			char c_str[ALRC_MAXSTRLEN];
			size_t len;
		} str;
		struct _AL_rctree *(*proc)(struct _AL_rctree *env, struct _AL_rctree *args);
		struct {
			struct _AL_rctree *car;
			struct _AL_rctree *cdr;
		} ccell;
	} data;
} AL_rctree;

/*
 * alrc_prim is a typedef that aids in the handling of alrc primitives, which
 * provide a sort of foreign function interface.
 */
typedef	AL_rctree *(*alrc_prim)( AL_rctree *env, AL_rctree *args );

/*
 * Allocate, initialize, and return an AL_rctree object.
 */
AL_rctree *_alRcTreeAlloc( void );

/*
 * Finalize and deallocate an AL_rctree object.
 */
void _alRcTreeFree( AL_rctree *node );

/*
 * Deallocates any and all AL_rctree objects creates thus far.
 */
void _alRcTreeDestroyAll( void );

#endif /* not AL_AL_RCTREE_H_ */
