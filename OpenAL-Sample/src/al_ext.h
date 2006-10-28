/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_ext.h
 *
 * Extension handling.
 */
#ifndef AL_AL_EXT_H_
#define AL_AL_EXT_H_

#include "al_siteconfig.h"
#include "al_types.h"
#include "al_filter.h"
#include "al_dlopen.h"

/*
 * the LAL... defines are used in conjunction with plugins, to determine the
 * symbols defined in each at runtime.
 */

#define LAL_EXT_SUFFIX		03282000
#define LAL_EXT_TABLE		alExtension
#define LAL_EXT_INIT		alExtInit
#define LAL_EXT_FINI		alExtFini

#define LAL_EXT_TABLE_STR	PASTE(LAL_EXT_TABLE, LAL_EXT_SUFFIX)
#define LAL_EXT_INIT_STR	PASTE(LAL_EXT_INIT, LAL_EXT_SUFFIX)
#define LAL_EXT_FINI_STR	PASTE(LAL_EXT_FINI, LAL_EXT_SUFFIX)

#define PASTE__(a1)             #a1
#define PASTE_(a1, a2)          PASTE__(a1 ## _ ## a2)
#define PASTE(a1, a2)           PASTE_(a1, a2)


/* stuff that was in the standard 1.0 headers, but shouldn't have been... */
#ifndef AL_BYTE_LOKI
#define AL_BYTE_LOKI                              0x100C
#endif /* not AL_BYTE_LOKI */


/* bookkeeping stuff */

/*
 * Initialize data structures neccesary for the registration and management of
 * extension groups and extension functions.  Return AL_TRUE if successful,
 * AL_FALSE otherwise.
 */
ALboolean _alInitExtensions( void );

/*
 * Deallocs the data structures allocated in _alInitExtensions.
 */
void _alDestroyExtensions( void );

/*
 * Registers an extension group.  Before registration, queries of
 * alIsExtensionPresent( extGroup) will return AL_FALSE, after will
 * return AL_TRUE.
 *
 * Returns AL_TRUE if registration was successful, AL_FALSE otherwise.
 */
ALboolean _alRegisterExtensionGroup( const ALubyte *extGroup );

/*
 * Destroy data structures needed to hold extension group information.
 */
void _alDestroyExtensionGroups( void );

/*
 * Gets a list of extension groups registered, populating buffer up to size.
 *
 * Returns AL_FALSE if size < 1, AL_TRUE otherwise.
 */
ALboolean _alGetExtensionStrings( ALchar *buffer, size_t size );

/* TODO: exporting this is a HACK */
ALboolean _alGetExtensionProcAddress( AL_funcPtr *procAddress, const ALchar *funcName );

/*
 * _alGetProcAddress( const ALubyte *fname )
 *
 * Obtain the address of a function (usually an extension) with the name
 * fname. All addresses are context-independent. Internal version to avoid
 * compiler warnings.
 */
AL_DLFunPtr
_alGetProcAddress( const ALchar *funcName );

/*
 * Adds a function to the available extension registry.  Before registration,
 * queries of alGetProcAddress( name ) will return NULL.  After, they will
 * return addr.
 *
 * Returns AL_FALSE if name has already been registered, AL_TRUE otherwise.
 */
ALboolean _alRegisterExtension( const ALubyte *name, AL_funcPtr addr );

/*
 * dlopens a shared object, gets the extension table from it, and
 * the runs _alRegisterExtension on each extension pair in it.
 *
 * Returns AL_TRUE if fname refers to a valid plugin, AL_FALSE otherwise.
 */
ALboolean _alLoadDL( const char *fname );

/*
 *  Functions that extensions can call to add functionality.
 */

/*
 * add a filter by name, replacing if already there.  Returns
 * AL_TRUE if function was added or replaced, AL_FALSE if that
 * wasn't possible.
 */
ALboolean lal_addTimeFilter( const char *name, time_filter *addr );

/*
 * alLokiTest( void *dummy )
 *
 * For testing purposes
 */
void alLokiTest( void *dummy );

#endif /* not AL_AL_EXT_H_ */
