/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_ext.c
 *
 * Defines the mojo for built-in or dlopened extensions.
 */
#include "al_siteconfig.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "al_main.h"
#include "al_types.h"
#include "al_ext.h"
#include "al_error.h"
#include "al_debug.h"

#include "al_mutexlib.h"
#include "al_dlopen.h"

/*
 * Maximum length of an extension function name.
 */
#define _AL_EXT_NAMELEN 240

/*
 * Maximum number of plugins supported
 */
#define MAX_EXT_LIBS 64

/* locking macros */
#define _alUnlockExtension() FL_alUnlockExtension(__FILE__, __LINE__)
#define _alLockExtension()   FL_alLockExtension(__FILE__, __LINE__)

/*
 * FL_alLockExtension( const char *fn, int ln )
 *
 * Lock mutex guarding extension data structures, passing fn and ln to
 * _alLockPrintf.
 */
static void FL_alLockExtension( const char *fn, int ln );

/*
 * FL_alUnlockExtension( const char *fn, int ln )
 *
 * Unlock mutex guarding extension data structures, passing fn and ln to
 * _alLockPrintf.
 */
static void FL_alUnlockExtension( const char *fn, int ln );

/* data structure storing extension library fini functions */
static struct {
	void (*pool[MAX_EXT_LIBS])(void);
	int index;
} FiniFunc;

/* data structure storing extension functions.  Simple binary tree. */
typedef struct _enode_t {
	ALubyte name[ _AL_EXT_NAMELEN + 1 ];
	AL_funcPtr addr;

	struct _enode_t *left;
	struct _enode_t *right;
} enode_t;

/*
 * etree is the base for our function registry.
 */
static enode_t *etree    = NULL;

/*
 * ext_mutex is the mutex guarding extension data structures.
 */
static MutexID ext_mutex = NULL;

/*
 * Adds a new node to treenode, with name name and address addr.  Returns
 * treenode.
 */
static enode_t *add_node( enode_t *treenode, const ALubyte *name, AL_funcPtr addr );

/*
 * Returns the node that exists in treenode with a matching name, or NULL if
 * no such node exists.
 */
static enode_t *get_node( enode_t *treenode, const ALchar *name );

/*
 * _alDestroyExtension( void *extp )
 *
 * Finalizes a enode_t, but not it's children.
 */
static void _alDestroyExtension( void *extp );

/*
 * tree_free( enode_t *treehead, void (*ff)(void *) )
 *
 * Finalize an entire enode tree.
 */
static void tree_free( enode_t *treehead, void (*ff)(void *) );

/*
 * Allocate and return a new extension node.
 */
static enode_t *new_ext( const ALubyte *name, AL_funcPtr addr );

#ifdef USE_DLOPEN
/* following functions unneeded in we don't have dlopen */

/*
 * _alExtPushFiniFunc( void (*func)(void) )
 *
 * Pushes a func on the fini stack, so that fini functions can be called on
 * exit or unload.
 */
static void _alExtPushFiniFunc( void (*func)(void) );

#endif /* USE_DLOPEN */

/*
 * On the level of the specification, "Extension" is a
 * group of functions and/or tokens that express a particular
 * extended functionality of the API.
 *
 * In terms of this implementation 'Extension' refers
 * to particular function that is registered in the
 * dictionary for fast GetProcAddress(). A simple dlsym()
 * is not possible because of the Linux implementation's
 * dynamic plugin system.
 *
 * Also in terms of the implementation, 'Extension Group'
 * refers to an extension in the specification sense--a name
 * that surrounds a group of functions and/or tokens. It
 * is possible to use a simple linked list for this, and
 * IsExtensionPresent() simply traverses this list.
 */

#define _AL_EXT_GROUPLEN 256

typedef struct _egroup_node_t {
	ALubyte name[_AL_EXT_GROUPLEN+1];
	struct _egroup_node_t* next;
} egroup_node_t;

/* linked list of exciting extension group names */
static egroup_node_t* egroup_list = NULL;

/*
 * Returns TRUE is gname is a supported extension, FALSE otherwise.
 */
ALboolean alIsExtensionPresent( const ALchar* gname ) {
	egroup_node_t* group = egroup_list;

	while( group ) {

		if( strncasecmp( (const char*) group->name,
			         (const char*) gname,
			         _AL_EXT_GROUPLEN ) == 0 ) {
			return AL_TRUE;
		}

		group = group->next;
	}

	return AL_FALSE;
}

/*
 * Populate buffer, up to size-1, with a string representation of the
 * extension strings.  Returns AL_TRUE if size was greater or equal to 1,
 * AL_FALSE otherwise.
 */
ALboolean _alGetExtensionStrings( ALchar* buffer, size_t size ) {
	egroup_node_t *group = egroup_list;

	if( size < 1 ) {
		return AL_FALSE;
	}

	buffer[0] = '\0';

	while( group ) {
		size_t length = strlen( (char *) group->name ) + 1;

		if( length < size ) {
			strncat( (char *) buffer, (char *) group->name, size );
			strncat( (char *) buffer, " ", size - length + 1 );
		} else {
			break;
		}

		size -= length;
		group = group->next;
	}

	return AL_TRUE;
}

/*
 * Add an extension group (see above) to the current list of extensions,
 * returning AL_TRUE.  AL_FALSE is returned on error.
 */
ALboolean _alRegisterExtensionGroup( const ALubyte* gname ) {
	egroup_node_t* group;

	if( gname == NULL ) {
		return AL_FALSE;
	}

	group = (egroup_node_t*) malloc( sizeof( *group ) );

	if( group == NULL ) {
		return AL_FALSE;
	}

	strncpy( (char*) group->name, (const char*) gname, _AL_EXT_GROUPLEN );
	group->next = egroup_list;
	egroup_list = group;

	return AL_TRUE;
}

/*
 * _alDestroyExtensionGroups( void )
 *
 * Finalize the extension group data structures.
 */
void _alDestroyExtensionGroups( void ) {
	egroup_node_t *group = egroup_list;

	while( group ) {
		egroup_node_t *temp = group->next;
		free( group );
		group = temp;
	}

	egroup_list = NULL;

	return;
}

/**
 * Extension support.
 */

#define DEFINE_AL_PROC(p) { #p, (AL_funcPtr)p }

typedef struct
{
	const ALchar *name;
	AL_funcPtr value;
} funcNameAddressPair;

funcNameAddressPair alProcs[] = {
	/* this has to be sorted! */
	DEFINE_AL_PROC(alBuffer3f),
	DEFINE_AL_PROC(alBuffer3i),
	DEFINE_AL_PROC(alBufferData),
	DEFINE_AL_PROC(alBufferf),
	DEFINE_AL_PROC(alBufferfv),
	DEFINE_AL_PROC(alBufferi),
	DEFINE_AL_PROC(alBufferiv),
	DEFINE_AL_PROC(alDeleteBuffers),
	DEFINE_AL_PROC(alDeleteSources),
	DEFINE_AL_PROC(alDisable),
	DEFINE_AL_PROC(alDistanceModel),
	DEFINE_AL_PROC(alDopplerFactor),
	DEFINE_AL_PROC(alDopplerVelocity),
	DEFINE_AL_PROC(alEnable),
	DEFINE_AL_PROC(alGenBuffers),
	DEFINE_AL_PROC(alGenSources),
	DEFINE_AL_PROC(alGetBoolean),
	DEFINE_AL_PROC(alGetBooleanv),
	DEFINE_AL_PROC(alGetBuffer3f),
	DEFINE_AL_PROC(alGetBuffer3i),
	DEFINE_AL_PROC(alGetBufferf),
	DEFINE_AL_PROC(alGetBufferfv),
	DEFINE_AL_PROC(alGetBufferi),
	DEFINE_AL_PROC(alGetBufferiv),
	DEFINE_AL_PROC(alGetDouble),
	DEFINE_AL_PROC(alGetDoublev),
	DEFINE_AL_PROC(alGetEnumValue),
	DEFINE_AL_PROC(alGetError),
	DEFINE_AL_PROC(alGetFloat),
	DEFINE_AL_PROC(alGetFloatv),
	DEFINE_AL_PROC(alGetInteger),
	DEFINE_AL_PROC(alGetIntegerv),
	DEFINE_AL_PROC(alGetListener3f),
	DEFINE_AL_PROC(alGetListener3i),
	DEFINE_AL_PROC(alGetListenerf),
	DEFINE_AL_PROC(alGetListenerfv),
	DEFINE_AL_PROC(alGetListeneri),
	DEFINE_AL_PROC(alGetListeneriv),
	DEFINE_AL_PROC(alGetProcAddress),
	DEFINE_AL_PROC(alGetSource3f),
	/* DEFINE_AL_PROC(alGetSource3i), TODO: NOT YET IMPLEMENTED!!! */
	DEFINE_AL_PROC(alGetSourcef),
	DEFINE_AL_PROC(alGetSourcefv),
	DEFINE_AL_PROC(alGetSourcei),
	DEFINE_AL_PROC(alGetSourceiv),
	DEFINE_AL_PROC(alGetString),
	DEFINE_AL_PROC(alIsBuffer),
	DEFINE_AL_PROC(alIsEnabled),
	DEFINE_AL_PROC(alIsExtensionPresent),
	DEFINE_AL_PROC(alIsSource),
	DEFINE_AL_PROC(alListener3f),
	DEFINE_AL_PROC(alListener3i),
	DEFINE_AL_PROC(alListenerf),
	DEFINE_AL_PROC(alListenerfv),
	DEFINE_AL_PROC(alListeneri),
	DEFINE_AL_PROC(alListeneriv),
	DEFINE_AL_PROC(alSource3f),
	/* DEFINE_AL_PROC(alSource3i), TODO: NOT YET IMPLEMENTED!!! */
	DEFINE_AL_PROC(alSourcePause),
	DEFINE_AL_PROC(alSourcePausev),
	DEFINE_AL_PROC(alSourcePlay),
	DEFINE_AL_PROC(alSourcePlayv),
	DEFINE_AL_PROC(alSourceQueueBuffers),
	DEFINE_AL_PROC(alSourceRewind),
	DEFINE_AL_PROC(alSourceRewindv),
	DEFINE_AL_PROC(alSourceStop),
	DEFINE_AL_PROC(alSourceStopv),
	DEFINE_AL_PROC(alSourceUnqueueBuffers),
	DEFINE_AL_PROC(alSourcef),
	DEFINE_AL_PROC(alSourcefv),
	DEFINE_AL_PROC(alSourcei),
	/* DEFINE_AL_PROC(alSourceiv), TODO: NOT YET IMPLEMENTED!!! */
	DEFINE_AL_PROC(alSpeedOfSound)
};

#undef DEFINE_AL_PROC

static int
compareFuncNameAddressPairs(const void *s1, const void *s2)
{
	const funcNameAddressPair *p1 = (const funcNameAddressPair*)s1;
	const funcNameAddressPair *p2 = (const funcNameAddressPair*)s2;
	return strcmp((const char*)(p1->name), (const char*)(p2->name));
}

static ALboolean
getStandardProcAddress(AL_funcPtr *value, const ALchar *funcName)
{
	funcNameAddressPair key;
	funcNameAddressPair *p;
	key.name = funcName;
	p = bsearch(&key, alProcs,
		    sizeof(alProcs) / sizeof(alProcs[0]),
		    sizeof(alProcs[0]),
		    compareFuncNameAddressPairs);
	if (p == NULL) {
		return AL_FALSE;
	}
	*value = p->value;
	return AL_TRUE;
}

/* TODO: exporting this is a HACK */
ALboolean
_alGetExtensionProcAddress( AL_funcPtr *procAddress, const ALchar *funcName )
{
	enode_t *retpair;
	retpair = get_node( etree, funcName );
	if(retpair == NULL) {
		return AL_FALSE;
	}
	*procAddress = retpair->addr;
	return AL_TRUE;
}

/*
 * alGetProcAddress( const ALubyte *fname )
 *
 * Obtain the address of a function (usually an extension) with the name
 * fname. All addresses are context-independent.
 */
AL_DLFunPtr
_alGetProcAddress( const ALchar *funcName )
{
	AL_funcPtr value = NULL;
	if ((getStandardProcAddress(&value, funcName) == AL_FALSE) &&
            (_alGetExtensionProcAddress(&value, funcName) == AL_FALSE)) {
		    _alDCSetError( AL_INVALID_VALUE );
	}
	return value;
}

void *
alGetProcAddress( const ALchar *funcName )
{
	/* NOTE: The cast is not valid ISO C! */
	return (void *) alDLFunPtrAsDataPtr_ (_alGetProcAddress(funcName));
}

/*
 * _alRegisterExtension is called to register an extension in our tree.
 * extensions are not via GetProcAddress until they have been registered.
 *
 * Returns AL_TRUE if the name/addr pair was added, AL_FALSE otherwise.
 */
ALboolean _alRegisterExtension( const ALubyte *name, AL_funcPtr addr ) {
	enode_t *temp;

	_alLockExtension();

	temp = add_node( etree, name, addr );
	if(temp == NULL) {
		_alUnlockExtension();
		_alDebug(ALD_EXT, __FILE__, __LINE__,
			"could not add extension %s", name);
		return AL_FALSE;
	}

	_alUnlockExtension();
	etree = temp;

	_alDebug(ALD_EXT, __FILE__, __LINE__, "registered %s", name);

	return AL_TRUE;
}

/*
 * _alDestroyExtension( void *extp )
 *
 * Finalizes a enode_t, but not it's children.
 */
static void _alDestroyExtension(void *extp) {
	free( extp );

	return;
}

/*
 * _alInitExtensions( void )
 *
 * Initializes extension bookkeeping data structures.
 *
 * Not much to do here, since the structures are built via the register calls.
 * So just init the mutex.  Return AL_TRUE unless the mutex could not be
 * initialized.
 */
ALboolean _alInitExtensions( void ) {
	if(ext_mutex == NULL) {
		ext_mutex = _alCreateMutex();
		if(ext_mutex == NULL) {
			return AL_FALSE;
		}
	}

	return AL_TRUE;
}

/*
 * _alDestroyExtensions( void )
 *
 * Destroy extension book keeping structures.
 */
void _alDestroyExtensions( void ) {
	int i;

	tree_free( etree, _alDestroyExtension );
	_alDestroyMutex( ext_mutex );

	etree     = NULL;
	ext_mutex = NULL;

	for(i = FiniFunc.index - 1; i >= 0; i--) {
		FiniFunc.pool[i](); /* call fini funcs */
		FiniFunc.index--;
	}

	return;
}

/*
 * Adds a function with name "name" and address "addr" to the tree with root
 * at treehead.
 *
 * assumes locked extensions
 */
static enode_t *add_node( enode_t *treehead, const ALubyte *name, AL_funcPtr addr ) {
	int i;
	enode_t *retval;

	if((addr == NULL) || (name == NULL)) {
		return NULL;
	}

	if(treehead == NULL) {
		retval = new_ext( name, addr );
		if(retval == NULL) {
			return NULL;
		}

		return retval;
	}

	i = ustrncmp( name, treehead->name, _AL_EXT_NAMELEN );

	if(i < 0) {
		treehead->left = add_node( treehead->left, name, addr );
	}
	if(i == 0) {
		return treehead;
	}
	if(i > 0) {
		treehead->right = add_node( treehead->right, name, addr );
	}

	return treehead;
}

/*
 * Returns a new extension node, using name and addr as initializers, or NULL
 * if resources were not available.
 */
static enode_t *new_ext( const ALubyte *name, AL_funcPtr addr ) {
	enode_t *retval = malloc( sizeof *retval );
	if(retval == NULL) {
		return NULL;
	}

	ustrncpy( retval->name, name, _AL_EXT_NAMELEN );
	retval->addr  = addr;
	retval->left  = NULL;
	retval->right = NULL;

	return retval;
}

/*
 * tree_free( enode_t *treehead, void (*ff)(void *) )
 *
 * Destroy the tree with root at treehead, calling ff on each data item.
 */
static void tree_free( enode_t *treehead, void (*ff)(void *) ) {
	enode_t *temp;

	if(treehead == NULL) {
		return;
	}

	if(treehead->left) {
		tree_free( treehead->left, ff );
	}

	temp = treehead->right;
	free( treehead );

	tree_free( temp, ff );

	return;
}

/*
 * Retrieve enode_t pointer for extension with name "name" from
 * tree with root "treehead", or NULL if not found.
 */
static enode_t *get_node( enode_t *treehead, const ALchar *name ) {
	int i;

	if((name == NULL) || (treehead == NULL)) {
		return NULL;
	}

	i = ustrncmp(name, treehead->name, _AL_EXT_NAMELEN);
	if(i < 0) {
		return get_node(treehead->left, name);
	}
	if(i > 0) {
		return get_node(treehead->right, name);
	}

	return treehead;
}

#ifdef USE_DLOPEN

/*
 * _alLoadDL( const char *fname )
 *
 * Load the plugin named by fname, returning AL_TRUE on sucess, AL_FALSE on
 * error.
 *
 *  FIXME: some sort of locking is in order.
 *
 *  FIXME: stick the fini_func function somewhere so we
 *         can actually call it.
 */
ALboolean _alLoadDL( const char *fname ) {
	AL_DLHandle handle;
	AL_extension *ext_table;
	static void (*init_func)( void );
	static void (*fini_func)( void );
	int i;

	handle = alDLOpen_ ( fname );
	if(handle == (AL_DLHandle)0) {
		_alDebug(ALD_EXT, __FILE__, __LINE__,
			"Could not load %s:\n\t%s", fname, alDLError_ ());
		return AL_FALSE;
	}

	ext_table = (AL_extension *) alDLDataSym_ (handle, LAL_EXT_TABLE_STR);
	if(ext_table == NULL) {
		_alDebug(ALD_EXT, __FILE__, __LINE__,
			"%s has no extension table.", fname);
		return AL_FALSE;
	}

	init_func = (void (*)(void)) alDLFunSym_ (handle, LAL_EXT_INIT_STR);
	fini_func = (void (*)(void)) alDLFunSym_ (handle, LAL_EXT_FINI_STR);

	for( i = 0; (ext_table[i].name != NULL) &&
		    (ext_table[i].addr != NULL); i++) {
		_alRegisterExtension( ext_table[i].name, ext_table[i].addr );
	}

	if(init_func != NULL) {
		/* initialize library */
		init_func();
	}

	if(fini_func != NULL) {
		_alExtPushFiniFunc( fini_func );
	}

	return AL_TRUE;
}

#else

ALboolean _alLoadDL(UNUSED(const char *fname)) {
	return AL_FALSE;
}

#endif

/*
 * lal_addTimeFilter( const char *name, time_filter *addr )
 *
 * Adds or replaces a time filter in the filter pipeline.  Returns AL_TRUE on
 * success, AL_FALSE on error.
 *
 *  Not super-well tested.
 *
 *  FIXME: set error?  Probably not.
 */
ALboolean lal_addTimeFilter( const char *name, time_filter *addr ) {
	AL_context *cc;
	time_filter_set *tfs;
	int i;
	int scmp;

	/* check args */
	if((name == NULL) || (addr == NULL)) {
		return AL_FALSE;
	}

	_alcDCLockContext();

	cc = _alcDCGetContext();
	if(cc == NULL) {
		_alcDCUnlockContext();
		return AL_FALSE;
	}

	tfs = cc->time_filters;

	for(i = 0; (i < _ALC_MAX_FILTERS) && (tfs->filter != NULL); i++) {
		scmp = strncmp(tfs[i].name, name, _ALF_MAX_NAME);
		if(scmp == 0) {
			/* overwrite existing filter */
			tfs[i].filter = addr;

			_alcDCUnlockContext();
			return AL_TRUE;
		}
	}

	if(i == _ALC_MAX_FILTERS) {
		/* no room for new filter */
		_alcDCUnlockContext();

		return AL_FALSE;
	}

	/* add new filter */
	strncpy(tfs[i].name, name, _ALF_MAX_NAME);
	tfs[i].filter = addr;

	_alcDCUnlockContext();

	return AL_TRUE;
}

/*
 * FL_alLockExtension( const char *fn, int ln )
 *
 * Lock mutex guarding extension data structures, passing fn and ln to
 * _alLockPrintf.
 */
static void FL_alLockExtension(UNUSED(const char *fn), UNUSED(int ln)) {
	_alLockPrintf("_alLockExtension", fn, ln);

	if(ext_mutex == NULL) {
		/*
		 * Sigh.  Extensions are loaded from config, so they
		 * need to have locks even before _alMain is called,
		 * so InitExtension may not be called at this point.
		 */
		ext_mutex = _alCreateMutex();
	}

	_alLockMutex( ext_mutex );

	return;
}

/*
 * FL_alUnlockExtension( const char *fn, int ln )
 *
 * Unlock mutex guarding extension data structures, passing fn and ln to
 * _alLockPrintf.
 */
static void FL_alUnlockExtension(UNUSED(const char *fn), UNUSED(int ln)) {
	_alLockPrintf("_alUnlockExtension", fn, ln);

	_alUnlockMutex( ext_mutex );

	return;
}


#ifdef USE_DLOPEN
/* avoid errors on platforms that don't support dlopen */

/*
 * _alExtPushFiniFunc( void (*func)(void) )
 *
 * Saves fini func to call on dlclose time.
 */
static void _alExtPushFiniFunc( void (*func)(void) ) {
	if(FiniFunc.index >= MAX_EXT_LIBS) {
		return;
	}

	FiniFunc.pool[FiniFunc.index] = func;
	FiniFunc.index++;

	return;
}
#endif /* USE_DLOPEN */

#define DEFINE_AL_ENUM(e) { #e, e }

typedef struct
{
	const ALchar *name;
	ALenum value;
} enumNameValuePair;

enumNameValuePair alEnums[] = {
	/* this has to be sorted! */
	DEFINE_AL_ENUM(AL_BITS),
	DEFINE_AL_ENUM(AL_BUFFER),
	DEFINE_AL_ENUM(AL_BUFFERS_PROCESSED),
	DEFINE_AL_ENUM(AL_BUFFERS_QUEUED),
	DEFINE_AL_ENUM(AL_BYTE_OFFSET),
	DEFINE_AL_ENUM(AL_CHANNELS),
	DEFINE_AL_ENUM(AL_CONE_INNER_ANGLE),
	DEFINE_AL_ENUM(AL_CONE_OUTER_ANGLE),
	DEFINE_AL_ENUM(AL_CONE_OUTER_GAIN),
	DEFINE_AL_ENUM(AL_DIRECTION),
	DEFINE_AL_ENUM(AL_DISTANCE_MODEL),
	DEFINE_AL_ENUM(AL_DOPPLER_FACTOR),
	DEFINE_AL_ENUM(AL_DOPPLER_VELOCITY),
	DEFINE_AL_ENUM(AL_EXPONENT_DISTANCE),
	DEFINE_AL_ENUM(AL_EXPONENT_DISTANCE_CLAMPED),
	DEFINE_AL_ENUM(AL_EXTENSIONS),
	DEFINE_AL_ENUM(AL_FALSE),
	DEFINE_AL_ENUM(AL_FORMAT_MONO16),
	DEFINE_AL_ENUM(AL_FORMAT_MONO8),
	DEFINE_AL_ENUM(AL_FORMAT_STEREO16),
	DEFINE_AL_ENUM(AL_FORMAT_STEREO8),
	DEFINE_AL_ENUM(AL_FREQUENCY),
	DEFINE_AL_ENUM(AL_GAIN),
	DEFINE_AL_ENUM(AL_INITIAL),
	DEFINE_AL_ENUM(AL_INVALID_ENUM),
	DEFINE_AL_ENUM(AL_INVALID_NAME),
	DEFINE_AL_ENUM(AL_INVALID_OPERATION),
	DEFINE_AL_ENUM(AL_INVALID_VALUE),
	DEFINE_AL_ENUM(AL_INVERSE_DISTANCE),
	DEFINE_AL_ENUM(AL_INVERSE_DISTANCE_CLAMPED),
	DEFINE_AL_ENUM(AL_LINEAR_DISTANCE),
	DEFINE_AL_ENUM(AL_LINEAR_DISTANCE_CLAMPED),
	DEFINE_AL_ENUM(AL_LOOPING),
	DEFINE_AL_ENUM(AL_MAX_DISTANCE),
	DEFINE_AL_ENUM(AL_MAX_GAIN),
	DEFINE_AL_ENUM(AL_MIN_GAIN),
	DEFINE_AL_ENUM(AL_NONE),
	DEFINE_AL_ENUM(AL_NO_ERROR),
	DEFINE_AL_ENUM(AL_ORIENTATION),
	DEFINE_AL_ENUM(AL_OUT_OF_MEMORY),
	DEFINE_AL_ENUM(AL_PAUSED),
	DEFINE_AL_ENUM(AL_PENDING),
	DEFINE_AL_ENUM(AL_PITCH),
	DEFINE_AL_ENUM(AL_PLAYING),
	DEFINE_AL_ENUM(AL_POSITION),
	DEFINE_AL_ENUM(AL_PROCESSED),
	DEFINE_AL_ENUM(AL_REFERENCE_DISTANCE),
	DEFINE_AL_ENUM(AL_RENDERER),
	DEFINE_AL_ENUM(AL_ROLLOFF_FACTOR),
	DEFINE_AL_ENUM(AL_SAMPLE_OFFSET),
	DEFINE_AL_ENUM(AL_SEC_OFFSET),
	DEFINE_AL_ENUM(AL_SIZE),
	DEFINE_AL_ENUM(AL_SOURCE_RELATIVE),
	DEFINE_AL_ENUM(AL_SOURCE_STATE),
	DEFINE_AL_ENUM(AL_SOURCE_TYPE),
	DEFINE_AL_ENUM(AL_SPEED_OF_SOUND),
	DEFINE_AL_ENUM(AL_STATIC),
	DEFINE_AL_ENUM(AL_STOPPED),
	DEFINE_AL_ENUM(AL_STREAMING),
	DEFINE_AL_ENUM(AL_TRUE),
	DEFINE_AL_ENUM(AL_UNDETERMINED),
	DEFINE_AL_ENUM(AL_UNUSED),
	DEFINE_AL_ENUM(AL_VELOCITY),
	DEFINE_AL_ENUM(AL_VENDOR),
	DEFINE_AL_ENUM(AL_VERSION)
};

#undef DEFINE_AL_ENUM

static int
compareEnumNameValuePairs(const void *s1, const void *s2)
{
	const enumNameValuePair *p1 = (const enumNameValuePair*)s1;
	const enumNameValuePair *p2 = (const enumNameValuePair*)s2;
	return strcmp((const char*)(p1->name), (const char*)(p2->name));
}

static ALboolean
getStandardEnumValue(ALenum *value, const ALchar *enumName)
{
	enumNameValuePair key;
	enumNameValuePair *p;
	key.name = enumName;
	p = bsearch(&key, alEnums,
		    sizeof(alEnums) / sizeof(alEnums[0]),
		    sizeof(alEnums[0]),
		    compareEnumNameValuePairs);
	if (p == NULL) {
		return AL_FALSE;
	}
	*value = p->value;
	return AL_TRUE;
}

static ALboolean
getExtensionEnumValue( ALenum *value, const ALchar *enumName )
{
	/* ToDo: Hook in our extension loader somehow */
	const char *name = (const char*)enumName;
	if (strcmp(name, "AL_BYTE_LOKI") == 0) {
		*value = AL_BYTE_LOKI;
		return AL_TRUE;
	} else if (strcmp(name, "AL_FORMAT_QUAD16_LOKI") == 0) {
		*value = AL_FORMAT_QUAD16_LOKI;
		return AL_TRUE;
	} else if (strcmp(name, "AL_FORMAT_QUAD8_LOKI") == 0) {
		*value = AL_FORMAT_QUAD8_LOKI;
		return AL_TRUE;
	} else if (strcmp(name, "AL_FORMAT_IMA_ADPCM_MONO16_EXT") == 0) {
		*value = AL_FORMAT_IMA_ADPCM_MONO16_EXT;
		return AL_TRUE;
	} else if (strcmp(name, "AL_FORMAT_IMA_ADPCM_STEREO16_EXT") == 0) {
		*value = AL_FORMAT_IMA_ADPCM_STEREO16_EXT;
		return AL_TRUE;
	} else if (strcmp(name, "AL_FORMAT_MONO_IMA4") == 0) {
		*value = AL_FORMAT_IMA_ADPCM_MONO16_EXT; /* white lie */
		return AL_TRUE;
	} else if (strcmp(name, "AL_FORMAT_STEREO_IMA4") == 0) {
		*value = AL_FORMAT_IMA_ADPCM_STEREO16_EXT; /* white lie */
		return AL_TRUE;
	} else if (strcmp(name, "AL_FORMAT_VORBIS_EXT") == 0) {
		*value = AL_FORMAT_VORBIS_EXT;
		return AL_TRUE;
	} else if (strcmp(name, "AL_FORMAT_WAVE_EXT") == 0) {
		*value = AL_FORMAT_WAVE_EXT;
		return AL_TRUE;
	} else {
		return AL_FALSE;
	}
}

/*
 * alGetEnumValue( const ALubyte *enumName )
 *
 * Returns the integer value of an enumeration (usually an extension)
 * with the name ename.
 */
ALenum
alGetEnumValue( const ALchar *enumName )
{
	ALenum value = 0;
	if (getStandardEnumValue(&value, enumName) == AL_TRUE) {
		return value;
	}
	if (getExtensionEnumValue(&value, enumName) == AL_TRUE) {
		return value;
	}
	_alDCSetError( AL_INVALID_VALUE );
	return value;
}
