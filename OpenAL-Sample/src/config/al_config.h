/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_config.h
 *
 * Prototypes, macros and definitions related to the input and parsing of
 * config state.
 *
 */
#ifndef AL_AL_CONFIG_H_
#define AL_AL_CONFIG_H_

#include "al_siteconfig.h"
#include <AL/al.h>
#include "config/al_rctree.h"
#include "config/al_rcvar.h"

/*
 * Parse the openalrc config file, if any.  Returns AL_TRUE if one was found
 * and contained valid openalrc syntax, AL_FALSE otherwise.
 */
ALboolean _alParseConfig( void );

/*
 * Deallocate the memory reserved in the call to _alParseConfig, as well as
 * any alrc objects that have been created since that point.
 */
void _alDestroyConfig( void );

/*
 * If str names an existing alrc symbol, return a pointer to the value
 * associated with that symbol.  Otherwise, return NULL.
 *
 */
AL_rctree *_alGlobalBinding( const char *str );

/*
 * Evaluate an alrc expression (expression), returning result.
 */
AL_rctree *_alEvalStr( const char *expression );

/*
 * Bind a symbol, named by symname, to the evaluation of value.
 */
AL_rctree *_alDefine( const char *symname, AL_rctree *value );

/*
 * Create and return a cons cell, with the car section pointing to ls1 and the
 * cdr section pointing to ls2.
 */
AL_rctree *alrc_cons( AL_rctree *ls1, AL_rctree *ls2 );

/*
 * Return the car section of the cons cell named by ls, or NULL if ls is not a
 * cons cell.
 */
AL_rctree *alrc_car( AL_rctree *ls );

/*
 * Return the cdr section of the cons cell named by ls, or NULL if ls is not a
 * cons cell.
 */
AL_rctree *alrc_cdr( AL_rctree *ls );

/*
 * scmtrue is returned by some of the rc_ functions.  It is probably advised
 * to remove it.
 */
extern const AL_rctree scmtrue;

#endif /* not AL_AL_CONFIG_H_ */
