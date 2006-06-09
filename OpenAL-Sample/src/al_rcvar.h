/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_rcvar.h
 *
 * Stuff related to the Rcvar config interface
 *
 */
#ifndef AL_AL_RCVAR_H_
#define AL_AL_RCVAR_H_

#include "al_siteconfig.h"
#include <AL/al.h>
#include <stdlib.h>

/*
 * opaque pointer to a lisp-like var.  Calling code shouldn't mess with it
 * directly.
 */
typedef void *Rcvar;

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
 * Returns the binding for the symbol named by name, if it exists, or NULL if
 * it doesn't.
 */
Rcvar rc_lookup( const char *name );

/*
 * Creates a binding between symname and the evaluation of val in the
 * global scope, returning val.
 */
Rcvar rc_define( const char *symname, Rcvar val );

/*
 * Creates a binding between car(ls) (symbol) the evaluation of cadr(ls) in the
 * global scope, returning cadr(ls).
 */
Rcvar rc_define_list( Rcvar ls );

/*
 * Evaluates str, returning result.
 */
Rcvar rc_eval( const char *str );

/*
 * Returns the type of sym.
 */
ALRcEnum rc_type( Rcvar sym );

/*
 * Returns the car portion of sym.  If sym is not a cons cell, returns NULL.
 */
Rcvar rc_car( Rcvar sym );

/*
 * Returns the cdr portion of sym.  If sym is not a cons cell, returns NULL.
 */
Rcvar rc_cdr( Rcvar sym );

/*
 * If sym has type ALRC_SYMBOL, this call populates retstr ( up to len bytes )
 * with the name of the symbol.
 */
Rcvar rc_symtostr0( Rcvar sym, char *retstr, size_t len );

/*
 * If sym has type ALRC_STRING, this call populates retstr ( up to len bytes )
 * with the value of the string.
 */
Rcvar rc_tostr0( Rcvar sym, char *retstr, size_t len );

/*
 * Returns AL_TRUE if sym is a boolean type and equal to #t, AL_FALSE
 * otherwise.
 */
ALboolean rc_tobool( Rcvar sym );

/*
 * If sym is a numerical type, returns the integer value of sym.  Otherwise,
 * returns 0.
 */
ALint rc_toint( Rcvar sym );

/*
 * For each member in ls, apply op to the member.
 */
void rc_foreach( Rcvar ls, Rcvar (*op)( Rcvar operand ));

/*
 * Quotes sym, returning that.
 */
Rcvar alrc_quote( Rcvar sym );

#endif /* not AL_AL_RCVAR_H_ */
