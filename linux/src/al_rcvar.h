/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_rcvar.h
 *
 * Stuff related to the Rcvar config interface
 *
 */
#ifndef AL_RCVAR_H_
#define AL_RCVAR_H_

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
 * If sym is a numerical type, returns the float value of sym.  Otherwise,
 * returns 0.0f.
 */
ALfloat rc_tofloat( Rcvar sym );

/*
 * If d1 and d2 both have type AL_STRING, returns AL_TRUE if there are
 * equivilant.  Returns AL_FALSE otherwise.
 */
ALboolean rc_strequal( Rcvar d1, Rcvar d2 );

/*
 * Evaluates needle and sees if it is a member in haystack, returning a list
 * with the first conscell to have a matching car as its head.
 */
Rcvar rc_lookup_list( Rcvar haystack, const char *needle );

/*
 * Returns a list with the first conscell to have a matching car with sym as
 * its head, NULL otherwise.
 */
Rcvar rc_member( Rcvar ls, Rcvar sym );

/*
 * Returns AL_TRUE if r1 and r2 and equivilant, AL_FALSE otherwise.
 */
ALboolean rc_equal( Rcvar r1, Rcvar r2 );

struct _AL_rctree;
struct _AL_rctree *(*rc_toprim( Rcvar sym ))( struct _AL_rctree *,
					      struct _AL_rctree * );

/*
 * Returns a const char *representation of type.
 */
const char *rc_typestr( ALRcEnum type );

/*
 * For each member in ls, apply op to the member.
 */
void rc_foreach( Rcvar ls, Rcvar (*op)( Rcvar operand ));

/*
 * Prints a stdout representation of sym to stdout.
 */
void rc_print( Rcvar sym );

/*
 * Quotes sym, returning that.
 */
Rcvar alrc_quote( Rcvar sym );

/*
 * car and cdr convienence macros.
 */
#define rc_cddr(s)                            rc_cdr(rc_cdr(s))
#define rc_cdddr(s)                           rc_cdr(rc_cddr(s))
#define rc_cddddr(s)                          rc_cdr(rc_cdddr(s))
#define rc_cddddddr(s)                        rc_cdr(rc_cdddddr(s))

#define rc_cadr(s)                            rc_car(rc_cdr(s))
#define rc_caddr(s)                           rc_car(rc_cddr(s))
#define rc_cadddr(s)                          rc_car(rc_cdddr(s))
#define rc_caddddr(s)                         rc_car(rc_cddddr(s))
#define rc_cadddddr(s)                        rc_car(rc_cdddddr(s))

#endif /* AL_RCVAR_H_ */
