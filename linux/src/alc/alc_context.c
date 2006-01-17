/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * alc_context.c
 *
 * Context management and application level calls.
 */

#include "al_siteconfig.h"

#include <AL/alc.h>
#include <AL/alext.h>
#include <stdlib.h>
#include <string.h>

#include "al_mixer.h"
#include "al_main.h"
#include "al_debug.h"
#include "al_ext.h"
#include "al_listen.h"
#include "al_source.h"
#include "al_spool.h"
#include "al_buffer.h"
#include "al_filter.h"
#include "al_distance.h"
#include "al_cpu_caps.h"

#include "alc/alc_error.h"
#include "alc/alc_device.h"
#include "alc/alc_speaker.h"
#include "alc/alc_context.h"

#include "backends/alc_backend.h"

/*
 * CONTEXT_BASE is the number which we start at for context ids.
 */
#define CONTEXT_BASE   0x9000

/*
 * canon_max and canon_min are the max/min values for PCM data in our
 * canonical format, respectively.
 */
const int canon_max = ((1<<(16-1))-1),
	  canon_min = -(1<<(16-1));

/*
 * canon_format is the canonical format that we represent data internally as.
 */
ALenum canon_format = _ALC_CANON_FMT;

/*
 * canon_speed is the sampling rate at which we internally represent data.
 */
ALuint canon_speed = _ALC_CANON_SPEED;

/*
 * _alcCCId holds the context id of the current context.
 */
ALuint _alcCCId = (ALuint) -1;

/*
 * al_contexts is our context pool.
 */
static struct {
	ALuint size;
	ALuint items;

	ALuint *map;
	ALboolean *inuse;
	AL_context *pool;
} al_contexts = { 0, 0, NULL, NULL, NULL };

/*
 * all_context_mutex is the mutex guarding operations which require all the
 * contexts to be locked.
 */
static MutexID all_context_mutex = NULL;

/*
 * contex_mutexen is a set of mutexes, one for each context, guarding each
 * contex.
 */
static MutexID *context_mutexen  = NULL;

/*
 * _alcDestroyContext( AL_context *cc )
 *
 * Non locking version of alcDestroyContext.
 */
static void _alcDestroyContext( AL_context *cc );

/*
 * _alcReallocContexts( ALuint newsize )
 *
 * Increases data structures to accomodate at least newsize contexts.
 */
static void _alcReallocContexts( ALuint newsize );

/*
 * _alcGenerateNewCid( void )
 *
 * Returns a new unique ALuint suitable for use as a cid.
 */
static ALuint _alcGenerateNewCid( void );

/*
 * _alcCidToIndex( ALuint cid )
 *
 * Converts cid into a simple index, returning that.
 */
static ALuint _alcCidToIndex( ALuint cid );

/*
 * _alcIndexToCid( int index )
 *
 * Converts index to a cid, returning that.
 */
static ALuint _alcIndexToCid( int cindex );

/*
 * ALCCONTEXTP_TO_ALUINT and ALUINT_TO_ALCCONTEXTP are macros to ease the
 * conversion of ALCcontext* to ALuint and visa versa.
 *
 * ToDo: I don't understand the need for the int64_t casts below...
 */
#if SIZEOF_VOID_P == 8
#define ALCCONTEXTP_TO_ALUINT(vp) ((ALuint) (int64_t) (vp))
#define ALUINT_TO_ALCCONTEXTP(al) ((ALCcontext *) (int64_t) (al))
#else
#define ALCCONTEXTP_TO_ALUINT(vp) ((ALuint) (vp))
#define ALUINT_TO_ALCCONTEXTP(al) ((ALCcontext *) (al))
#endif

/*
 * alcMakeContextCurrent( ALCcontext *handle )
 *
 * Makes the context refered to by handle the current context.  If handle does
 * not refer to a context, ALC_INVALID_CONTEXT is set and ALC_FALSE returned.
 * Otherwise, the operation suceeds and ALC_TRUE is returned.
 */
ALCboolean alcMakeContextCurrent( ALCcontext *handle )
{
	AL_context *cc;
	int cid;
	static ALboolean ispaused = AL_FALSE;
	ALboolean should_init_mixer = AL_FALSE;

	if(handle == NULL) {
		/* NULL handle means pause */
		if(ispaused == AL_FALSE) {
			if(al_contexts.items != 0) {
				/* only lock if a context has been
				 * created.  Otherwise, don't.
				 */

				/* Give mixer thread chance to catch up */

				_alLockMixerPause();

				_alcLockAllContexts();

				cc = _alcDCGetContext();
				if( cc == NULL ) {
					/* I don't even want to think about it */

					/* FIXME: wrong error */
					_alcSetError( ALC_INVALID_CONTEXT );
					_alcUnlockAllContexts();
					return ALC_FALSE;
				}

				/*
				 * inform current audio device about
				 * impending stall.
				 */
				if( cc->write_device ) {
					_alcDevicePause( cc->write_device );
				}
				if( cc->read_device ) {
					_alcDevicePause( cc->read_device );
				}

				_alcCCId = (ALuint) -1;
				_alcUnlockAllContexts();
			}

			ispaused = AL_TRUE;
		}

		return ALC_TRUE;
	}

	cid = ALCCONTEXTP_TO_ALUINT( handle );

	_alcLockAllContexts();

	if( _alcIsContext( _alcCCId ) == AL_FALSE ) {
		should_init_mixer = AL_TRUE;
	}

	_alcCCId = cid;

	cc = _alcGetContext( cid );
	if( cc == NULL ) {
		/* I don't even want to think about it */

		/* FIXME: wrong error */
		_alcSetError( ALC_INVALID_CONTEXT );
		_alcUnlockAllContexts( );
		return ALC_FALSE;
	}

	if( should_init_mixer == AL_TRUE ) {
		/* Set up mixer thread */
		if(_alInitMixer() == AL_FALSE) {
			/* do something */
		}
	}

	/* set device's current context */
	if(cc->write_device) {
		cc->write_device->cc = cc;
		_alcDeviceSet( cc->write_device );
	}

	_alSetMixer( cc->should_sync ); /* set mixing stats */

	if(cc->read_device) {
		cc->read_device->cc = cc;
		_alcDeviceSet( cc->read_device );
	}


	if(ispaused == AL_TRUE) {
		/* someone unpaused us */
		ispaused = AL_FALSE;

		_alcDeviceResume( cc->write_device );
		_alcDeviceResume( cc->read_device );

		_alcUnlockAllContexts();
		_alUnlockMixerPause();
	} else {
		/* just unlock contexts */
		_alcUnlockAllContexts();
	}

	return ALC_TRUE;
}

/*
 * alcDestroyContext( ALvoid *handle )
 *
 * Destroys the context referred to by handle.
 */
void alcDestroyContext( ALCcontext *handle )
{
	AL_context *cc;
	int cid;

	if( handle == NULL ) {
		_alcSetError( ALC_INVALID_CONTEXT );
		return;
	}

	cid = ALCCONTEXTP_TO_ALUINT( handle );

	_alcLockContext( cid );
	cc = _alcGetContext( cid );
	if( cc == NULL ) {
		_alcSetError( ALC_INVALID_CONTEXT );
		_alcUnlockContext( cid );
		return;
	}

	/*
	 * If this is the last context, run _alExit()
	 * to clean up the cruft
	 */
	if( al_contexts.items == 1 ) {
		/* unlock context for final time */
		_alcUnlockContext( cid );

		/* cleanup */
		_alExit();

		/*
		 * Set NumContexts to 0
		 */
		al_contexts.items = 0;

		/*
		 * Destroy the all-locking-contexts
		 */
		_alDestroyMutex( all_context_mutex );
		all_context_mutex = NULL;

		return;
	}

	/* call internal destroyer */
	_alcDestroyContext( cc );

	/*
	 * Decrement the number of contexts in use.
	 */
	al_contexts.items--;

	_alcUnlockContext( cid );
}

/**
 * alcProcessContext( ALvoid *alcHandle )
 *
 * Performs processing on a synced context, nop on a asynchronous
 * context.
 *
 * If alcHandle is not valid, ALC_INVALID_CONTEXT is set.
 */
void alcProcessContext( ALCcontext *alcHandle )
{
	AL_context *cc;
	ALboolean should_sync;
	int cid;

	if( alcHandle == NULL ) {
		/*
		 * invalid name?
		 */
		_alDebug(ALD_CONTEXT, __FILE__, __LINE__,
		      "alcUpdateContext: alcHandle == NULL");

		_alcSetError( ALC_INVALID_CONTEXT );
		return;
	}

	cid = ALCCONTEXTP_TO_ALUINT( alcHandle );

	/* determine whether we need to sync or not */
	_alcLockAllContexts();

	cc = _alcGetContext( cid );
	if(cc == NULL) {
		_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
			  "alcUpdateContext: invalid context id %d",
			  cid );

		_alcSetError( ALC_INVALID_CONTEXT );
		_alcUnlockAllContexts();
		return;
	}

	should_sync = cc->should_sync;
	_alcUnlockAllContexts();

	if( should_sync == AL_TRUE ) {
		mixer_iterate( NULL );
	} else {
		/* unsuspend async contexts */
		cc->issuspended = AL_FALSE;
	}
}

/**
 *
 * alcSuspendContext( ALCcontext *alcHandle )
 *
 * Suspends processing on an asynchronous context.  This is a legal nop on a
 * synced context.
 *
 * If alcHandle is not valid, ALC_INVALID_CONTEXT is returned.
 */
void alcSuspendContext( ALCcontext *alcHandle )
{
	AL_context *cc;
	ALuint cid;

	if( alcHandle == NULL ) {
		/*
		 * invalid name?
		 */
		_alDebug(ALD_CONTEXT, __FILE__, __LINE__,
		      "alcUpdateContext: alcHandle == NULL");

		_alcSetError( ALC_INVALID_CONTEXT );

		return;
	}

	cid = ALCCONTEXTP_TO_ALUINT( alcHandle );

	/* determine whether we need to sync or not */
	_alcLockAllContexts();

	cc = _alcGetContext( cid );
	if( cc == NULL ) {
		_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
			  "alcUpdateContext: invalid context id %d",
			  cid );

		_alcSetError( ALC_INVALID_CONTEXT );

		_alcUnlockAllContexts();

		return;
	}

	if( cc->should_sync == AL_FALSE) {
		/*
		 * only asynchronous contexts can be
		 * suspended.
		 */
		cc->issuspended = AL_TRUE;
	}

	_alcUnlockAllContexts();

	return;
}

/*
 * alcCreateContext( ALCdevice *dev, ALCint *attrlist )
 *
 * Allocates, initialiaes, and returns an AL_context handle, suitable for
 * passing to other alc functions.  Uses dev as the write device for the
 * context.  attrlist is an int array, zero terminated, that contains
 * attribute/value pairs used to initialize the context.
 *
 * We use a meet-or-exceed system here.  If any attribute in attrlist cannot
 * have the required value met or exceeded, NULL is returned.  If dev is not
 * valid, ALC_INVALID_DEVICE is set and NULL is returned.
 *
 * FIXME: not as well tested as I'd like.
 */
ALCcontext *alcCreateContext( ALCdevice *dev, const ALCint *attrlist )
{
	ALint cid;

	if( dev == NULL ) {
		_alcSetError( ALC_INVALID_DEVICE );

		return NULL;
	}
	
	_alDetectCPUCaps();
	
	if( al_contexts.items == 0 ) {
		/*
		 * This is the first context to be created.  Initialize the
		 * library's data structures.
		 */

		/* get a context name for the new context */
		cid = _alcGetNewContextId();

		/* misc library initialization */
		_alInit();

		/* set the context attributes */
		_alcLockContext( cid );
		_alcSetContext( attrlist, cid, dev );
		_alcUnlockContext( cid );

		return ALUINT_TO_ALCCONTEXTP( cid );
	}

	_alcLockAllContexts();
	cid = _alcGetNewContextId();
	if(cid == -1) {
		_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
			  "alcCreateContext failed." );

		_alcSetError( ALC_INVALID_DEVICE );
		_alcUnlockAllContexts();

		return NULL;
	}

	_alcUnlockAllContexts();

	_alcLockContext( cid );
	_alcSetUse( cid, AL_TRUE );
	_alcSetContext( attrlist, cid, dev );
	_alcUnlockContext( cid );

	return ALUINT_TO_ALCCONTEXTP( cid );
}

/*
 * _alcDestroyContext( AL_context *cc )
 *
 * Non locking version of alcDestroyContext.
 *
 * FIXME: should assume that *all contexts* are locked?
 */
void _alcDestroyContext( AL_context *cc )
{
	free(cc->Flags);
	cc->Flags = 0;
	cc->NumFlags = 0;

	_alDestroyListener( &cc->listener );
	_alDestroySources( &cc->source_pool );
}


/*
 * FL_alcLockContext( ALuint cid, const char *fn, int ln )
 *
 * Locks the mutex associated with the context named by cid, passing fn and ln
 * to _alLockPrintf for debugging purposes.
 */
void FL_alcLockContext(ALuint cid, UNUSED(const char *fn), UNUSED(int ln)) {
	int cindex;

	_alLockPrintf("_alcLockContext", fn, ln);

	cindex = _alcCidToIndex(cid);
	if( cindex < 0 ) {
		_alDebug(ALD_CONTEXT, __FILE__, __LINE__,
				"FL_alcLockContext: invalid context.");
		return;
	}

	_alcLockAllContexts();

	_alLockMutex(context_mutexen[cindex]);

	_alcUnlockAllContexts();

	return;
}

/*
 * FL_alcUnlockContext( ALuint cid, const char *fn, int ln )
 *
 * Unlocks the mutex associated with the context named by cid, passing fn and ln
 * to _alLockPrintf for debugging purposes.
 */
void FL_alcUnlockContext(ALuint cid, UNUSED(const char *fn), UNUSED(int ln)) {
	int cindex;

	_alLockPrintf("_alcUnlockContext", fn, ln);

	cindex = _alcCidToIndex( cid );
	if( cindex < 0 ) {
		_alDebug(ALD_CONTEXT, __FILE__, __LINE__,
				"FL_alcUnlockContext: invalid context.");
		return;
	}

	_alUnlockMutex(context_mutexen[cindex]);

	return;
}

/*
 * _alcGetContext( ALuint cid )
 *
 * Returns pointer to the AL_context named by cid, or NULL if cid is not a
 * valid context name.
 */
AL_context *_alcGetContext( ALuint cid ) {
	ALuint cindex;

	cindex = _alcCidToIndex(cid);

	if(cindex >= al_contexts.size) {
		return NULL;
	}

	if(al_contexts.inuse[cindex] == AL_FALSE) {
		return NULL;
	}

	return &al_contexts.pool[cindex];
}


/*
 * void _alcSetContext( const ALCint *attrlist, ALuint cid, AL_device *dev )
 *
 * Sets context id paramaters according to an attribute list and device.
 *
 */
void _alcSetContext(const ALCint *attrlist, ALuint cid, AL_device *dev ) {
	AL_context *cc;
	ALboolean reading_keys = AL_TRUE;
	struct { int key; int val; } rdr;
	ALuint refresh_rate = 15;
        ALuint bufsiz;

	cc = _alcGetContext( cid );
	if(cc == NULL) {
		return;
	}

	/* get ready to copy attrlist */
	free(cc->Flags);
	cc->Flags = 0;
	cc->NumFlags = 0;

	/* Set our preferred mixer stats */
        if (dev->flags & ALCD_WRITE)
	{
		cc->write_device = dev;
	}
        if (dev->flags & ALCD_READ)
	{
		cc->read_device = dev;
	}

	while(attrlist && (reading_keys == AL_TRUE)) {
		void *t = 0;

		rdr.key = *attrlist++;
		if(rdr.key != 0)
		{
			rdr.val = *attrlist++;
		}

		t = realloc(cc->Flags,
				  (2 + cc->NumFlags) * 2 * sizeof *cc->Flags);
		if(t)
		{
			cc->Flags = t;

			cc->Flags[2 * cc->NumFlags] = rdr.key;
			cc->Flags[2 * cc->NumFlags + 1] = rdr.val;

			cc->NumFlags++;
		}

		switch(rdr.key) {
			case ALC_FREQUENCY:
				canon_speed = rdr.val;

				_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
					"cc->external_speed = %d", rdr.val );
				break;
			case ALC_REFRESH:
			        refresh_rate = rdr.val;
				break;
			case ALC_SOURCES_LOKI:
				spool_resize(&cc->source_pool, rdr.val);

				_alDebug(ALD_CONTEXT,
					__FILE__, __LINE__,
					"ALC_SOURCES (%d)", rdr.val);
				break;
			case ALC_BUFFERS_LOKI:
				_alNumBufferHint( rdr.val );
				break;
			case ALC_SYNC:
				if(rdr.val == AL_TRUE) {
					cc->should_sync = AL_TRUE;
				} else {
					cc->should_sync = AL_FALSE;
				}
				break;
			case 0:
				reading_keys = AL_FALSE;
				break;
			default:
				reading_keys = AL_FALSE;
				_alDebug(ALD_CONTEXT,
					__FILE__, __LINE__,
					"unsupported context attr %d",
					rdr.key );
				break;
		}
	}

	/* okay, now post process */

	if( refresh_rate > canon_speed ) {
		/*
		 * We don't accept refresh rates greater than the
		 * sampling rate.
		 */
		refresh_rate = canon_speed;
	}

	bufsiz = _alSmallestPowerOfTwo(
				(ALuint) ((float) canon_speed / refresh_rate));
        if (dev->flags & ALCD_WRITE)
		cc->write_device->bufsiz = bufsiz;
        if (dev->flags & ALCD_READ)
		cc->read_device->bufsiz = bufsiz;

	_alDebug( ALD_CONTEXT, __FILE__, __LINE__,
		"new bufsiz = %d", rdr.val);

	return;
}

/*
 * _alcInitContext( ALuint cid )
 *
 * Initialize the context named by cid, and returns the AL_context associated
 * with that id.
 *
 * assumes locked context
 */
AL_context *
_alcInitContext( ALuint cid )
{
	AL_context *cc = _alcGetContext(cid);
	if(cc == NULL) {
		/* invalid context */
		return NULL;
	}

	/* initialize spec parameters */
	cc->doppler_factor = 1.0f;
	cc->doppler_velocity = 1.0f;
	cc->speed_of_sound = 343.3f;
	cc->distance_model = AL_INVERSE_DISTANCE_CLAMPED;

	_alUpdateDistanceModel(cc);

	_alInitTimeFilters(cc->time_filters);

	cc->alErrorIndex   = AL_NO_ERROR;

	_alInitListener(&cc->listener);

	/* Source initializations */
	spool_init(&cc->source_pool);

	cc->read_device = NULL;
	cc->write_device = NULL;

	/* This implicitly calls _alcSpeakerInit. The logic behind this is fascinating... */
	_alcSpeakerMove( cid );

	/*
	 * should_sync:
	 * 	AL_FALSE:
	 * 		we use async_mixer_iterate, and don't need
	 * 		to have alcUpdateContext called to actually
	 * 		mix the audio.
	 * 	AL_TRUE:
	 * 		we use sync_mixer_iterate, and need to have
	 * 		alcUpdateContext called to actually mix the
	 * 		audio.
	 */
	cc->should_sync = AL_FALSE;
	cc->issuspended = AL_FALSE; /* deviates */

	cc->Flags = 0;
	cc->NumFlags = 0;

	return cc;
}

/*
 * _alcSetUse( ALuint cid, ALboolean value )
 *
 * Sets the use flag of context with id cid to value.
 *
 * Assumes context is locked
 *
 * NOTES:
 *    Can't use alcGetContext, because that checks the use flag,
 *    which maybe set to false, which is what this function seeks
 *    to correct.
 *
 */
ALboolean _alcSetUse(ALuint cid, ALboolean val) {
	ALuint cindex;

	cindex = _alcCidToIndex(cid);

	if(cindex >= al_contexts.size) {
		return !val;
	}

	return al_contexts.inuse[cindex] = val;
}

/*
 * _alcInUse( ALuint cid )
 *
 * Returns AL_TRUE if the context named by cid is in use, AL_FALSE otherwise.
 */
ALboolean _alcInUse(ALuint cid) {
	ALuint cindex;

	cindex = _alcCidToIndex(cid);

	if(cindex >= al_contexts.size) {
		return AL_FALSE;
	}

	return al_contexts.inuse[cindex];
}

/*
 * FL_alcLockAllContexts( const char *fn, int ln )
 *
 * Locks the mutex associated guarding all contexts, passing fn and ln to
 * _alLockPrintf for debugging purposes.
 */
void FL_alcLockAllContexts(UNUSED(const char *fn), UNUSED(int ln)) {
	if( all_context_mutex == NULL ) {
		return;
	}

	_alLockPrintf("_alcLockAllContexts", fn, ln);
	_alLockMutex(all_context_mutex);
}

/*
 * FL_alcUnlockAllContexts( const char *fn, int ln )
 *
 * Unlocks the mutex associated guarding all contexts, passing fn and ln to
 * _alLockPrintf for debugging purposes.
 */
void FL_alcUnlockAllContexts(UNUSED(const char *fn), UNUSED(int ln)) {
	if( all_context_mutex == NULL ) {
		return;
	}

	_alLockPrintf("_alcUnlockAllContexts", fn, ln);
	_alUnlockMutex(all_context_mutex);
}

/*
 * _alcGetListener( ALuint cid )
 *
 * Returns a pointer to the listener associated with context named by cid, or
 * NULL if cid does not name a valid context.
 *
 * assumes locked context
 */
AL_listener *_alcGetListener( ALuint cid ) {
	AL_context *cc;

	cc = _alcGetContext(cid);
	if(cc == NULL) {
		return NULL;
	}

	return &cc->listener;
}

/*
 * _alcDestroyAll( void )
 *
 * Deallocates the data structures for all contexts.
 */
void _alcDestroyAll( void ) {
	AL_context *freer;
	ALuint i;
	ALuint cid;

	for(i = 0; i < al_contexts.items; i++) {
		cid = _alcIndexToCid( i );

		if(context_mutexen[i] != NULL) {
			_alDestroyMutex( context_mutexen[i] );
			context_mutexen[i] = NULL;
		}

		if(_alcInUse(cid) == AL_TRUE) {
			freer = _alcGetContext( cid );

			if(freer != NULL) {
				_alcDestroyContext( freer );
			}
		}
	}

	free( context_mutexen );
	context_mutexen = NULL;

	free( al_contexts.map );
	free( al_contexts.pool );
	free( al_contexts.inuse );

	al_contexts.map   = NULL;
	al_contexts.pool  = NULL;
	al_contexts.inuse = NULL;
	al_contexts.items = 0;
	al_contexts.size  = 0;

	return;
}

/*
 * _alcGetNewContextId( void )
 *
 * Returns a new id for use as a context name, setting its use flag to
 * AL_TRUE, and returns the id.
 *
 * If there are no unused contexts, at least one more is created,
 * and it is modified and returned in the manner described above.
 *
 * assumes locked contexts
 */
ALint _alcGetNewContextId(void) {
	ALuint i;
	ALuint cid;
	ALuint cindex;

	for(i = 0; i < al_contexts.size; i++) {
		if(al_contexts.inuse[i] == AL_TRUE) {
			continue;
		}

		al_contexts.items++;
		al_contexts.inuse[i] = AL_TRUE;
		return al_contexts.map[i] = _alcGenerateNewCid();
	}

	_alcReallocContexts(al_contexts.size + 1);

	cindex = al_contexts.size - 1;
	cid = _alcGenerateNewCid();

	assert(al_contexts.inuse[cindex] == AL_FALSE);

	al_contexts.inuse[cindex] = AL_TRUE;
	al_contexts.map[cindex]   = cid;

	if(_alcInitContext(cid) == NULL) {
		assert(0);
		return -1;
	}

	al_contexts.items++;

	/*
	 *  We create contexts at the end, so the context id
	 *  will be the last valid element index (al_contexts.items - 1)
	 */
	return cid;
}

/*
 * _alcReallocContexts( ALuint newsize )
 *
 * _alcReallocContexts resizes the context pool to at least
 * newsize contexts, and creates mutex such that the new
 * contexts can be locked.
 *
 * assumes locked contexts
 */
static void _alcReallocContexts(ALuint newsize) {
	void *temp;
	ALuint i;

	if(al_contexts.size >= newsize) {
		return;
	}

	/* resize context pool */
	temp = realloc(al_contexts.pool, sizeof *al_contexts.pool * newsize);
	if(temp == NULL) {
		perror("_alcReallocContexts malloc");
		exit(4);
	}
	al_contexts.pool = temp;

	/* resize inuse flags */
	temp = realloc(al_contexts.inuse, sizeof *al_contexts.inuse * newsize);
	if(temp == NULL) {
		perror("_alcReallocContexts malloc");
		exit(4);
	}
	al_contexts.inuse = temp;

	/* resize context map */
	temp = realloc(al_contexts.map, sizeof *al_contexts.map * newsize);
	if(temp == NULL) {
		perror("_alcReallocContexts malloc");
		exit(4);
	}
	al_contexts.map = temp;

	temp = realloc(context_mutexen, sizeof *context_mutexen * newsize);
	if(temp == NULL) {
		perror("_alcReallocContexts malloc");
		exit(4);
	}
	context_mutexen = temp;

	/* initialize new data */
	for(i = al_contexts.items; i < newsize; i++) {
		al_contexts.inuse[i] = AL_FALSE;
		al_contexts.map[i] = 0;
		context_mutexen[i] = _alCreateMutex();
	}

	if(al_contexts.items == 0) {
		/* If al_contexts.items is <= 0, then were are creating
		 * the contexts for the first time, and must create the
		 * "lock all contexts" mutex as well.
		 */

		all_context_mutex = _alCreateMutex();
		if(all_context_mutex == NULL) {
			perror("CreateMutex");
			exit(2);
		}
	}

	al_contexts.size = newsize;

	return;
}

/*
 * _alcGetTimeFilters( ALuint cid )
 *
 * Returns a pointer to the time_filter_set associated with the context named
 * by cid, or NULL if cid does not name a context.
 *
 * assumes locked context cid
 */
time_filter_set *_alcGetTimeFilters( ALuint cid ) {
	AL_context *cc;

	cc = _alcGetContext( cid );
	if(cc == NULL) {
		return NULL;
	}

	return cc->time_filters;
}

/*
 * _alcIndexToCid( int index )
 *
 * Converts index to a cid, returning that.
 */
static ALuint _alcCidToIndex( ALuint cid ) {
	ALuint i;

	for(i = 0; i < al_contexts.size; i++) {
		if( al_contexts.map[i] == cid ) {
			return i;
		}
	}

	return -1;
}

/*
 * _alcIndexToCid( int index )
 *
 * Converts index to a cid, returning that.
 */
static ALuint _alcIndexToCid(int ind)
{
	assert(ind >= 0);
	assert(ind < (int) al_contexts.size);

	return al_contexts.map[ind];
}

/*
 * _alcGenerateNewCid( void )
 *
 * Returns a new unique ALuint suitable for use as a cid.
 */
static ALuint _alcGenerateNewCid(void) {
	static ALuint base = CONTEXT_BASE;

	return base++;
}

/*
 * alcGetCurrentContext( void )
 *
 * Returns context handle suitable associated with current context,
 * suitable for use with every function that takes a context handle,
 * or NULL if there is no current context.
 */
ALCcontext *alcGetCurrentContext( void )
{
	if(al_contexts.items == 0)
	{
		return NULL;
	}

	if( _alcCCId == (ALuint) -1 )
	{
		/* We are paused */
		return NULL;
	}

	return ALUINT_TO_ALCCONTEXTP( _alcCCId );
}

/*
 * _alcGetReadBufsiz( ALuint cid )
 *
 * Returns the preferred read buffer size of the context named by cid,
 * in bytes.
 */
ALuint _alcGetReadBufsiz( ALuint cid ) {
	AL_context *cc = _alcGetContext( cid );

	if(cc == NULL) {
		return 0;
	}

	if( cc->read_device == NULL) {
		return 0;
	}

	return cc->read_device->bufsiz;
}

/*
 * _alcGetWriteBufsiz( ALuint cid )
 *
 * Returns the preferred write buffer size of the context named by cid,
 * in bytes.
 *
 * assumes locked context
 */
ALuint _alcGetWriteBufsiz( ALuint cid ) {
	AL_context *cc = _alcGetContext( cid );

	if(cc == NULL) {
		return 0;
	}

	if( cc->write_device == NULL) {
		return 0;
	}

	return cc->write_device->bufsiz;
}

/*
 * _alcGetReadFormat( ALuint cid )
 *
 * Returns the preferred read openal format of the context named by cid.
 *
 * assumes locked context
 */
ALenum _alcGetReadFormat( ALuint cid ) {
	AL_context *cc;

	cc = _alcGetContext( cid );
	if(cc == NULL) {
		return 0;
	}

	if( cc->read_device == NULL) {
		return 0;
	}

	return cc->read_device->format;
}

/*
 * _alcGetWriteFormat( ALuint cid )
 *
 * Returns the preferred write openal format of the context named by cid.
 */
ALenum _alcGetWriteFormat( ALuint cid ) {
	AL_context *cc;

	cc = _alcGetContext( cid );
	if(cc == NULL) {
		return 0;
	}

	if( cc->write_device == NULL) {
		return 0;
	}

	return cc->write_device->format;
}

/*
 * _alcGetReadSpeed( ALuint cid )
 *
 * Returns the preferred sampling rate of the read device associated with the
 * context named by cid.
 *
 * assumed locked context
 */
ALuint _alcGetReadSpeed(ALuint cid) {
	AL_context *cc;

	cc = _alcGetContext( cid );
	if(cc == NULL) {
		return 0;
	}

	if( cc->read_device == NULL) {
		return 0;
	}

	return cc->read_device->speed;
}

/*
 * _alcGetWriteSpeed( ALuint cid )
 *
 * Returns the preferred sampling rate of the write device associated with the
 * context named by cid.
 *
 * assumes locked context
 */
ALuint _alcGetWriteSpeed( ALuint cid ) {
	AL_context *cc;

	cc = _alcGetContext( cid );
	if(cc == NULL) {
		return 0;
	}

	if( cc->write_device == NULL) {
		return 0;
	}

	return cc->write_device->speed;
}

/*
 * _alcDeviceRead( ALuint cid, ALvoid *dataptr, ALuint bytes_to_read )
 *
 * Reads bytes_to_read worth of data from the read device
 * associated with the context named cid.
 *
 * assumes locked context
 */
ALsizei _alcDeviceRead( ALuint cid, ALvoid *dataptr, ALuint bytes_to_read )
{
	AL_context *cc;

	cc = _alcGetContext( cid );
	if( cc == NULL ) {
		return 0;
	}

	if( cc->read_device == NULL ) {
		return 0;
	}

	return alcBackendRead_(cc->read_device->handle, dataptr, bytes_to_read);
}

/*
 * _alcDeviceWrite( ALuint cid, ALvoid *dataptr, ALuint bytes_to_write )
 *
 * Writes bytes_to_write worth of data from dataptr to the write device
 * associated with the context named cid.
 *
 * assumes locked context
 */
void _alcDeviceWrite( ALuint cid, ALvoid *dataptr, ALuint bytes_to_write ) {
	AL_context *cc;

	cc = _alcGetContext( cid );
	if( cc == NULL ) {
		return;
	}

	if( cc->write_device == NULL ) {
		return;
	}

	alcBackendWrite_( cc->write_device->handle, dataptr, bytes_to_write );

	return;
}

/*
 * _alcIsContext( ALuint cid )
 *
 * Returns AL_TRUE if cid names a valid context, AL_FALSE otherwise.
 *
 * assumes locked context
 */
ALboolean _alcIsContext( ALuint cid ) {
	AL_context *cc;

	cc = _alcGetContext( cid );
	if( cc == NULL ) {
		return AL_FALSE;
	}

	return AL_TRUE;
}

/*
 * _alcIsContextSuspended( ALuint cid )
 *
 * Returns AL_TRUE if this context is suspended, AL_FALSE otherwise.
 * Suspended contexts do not have their sources updated, or mixed.
 *
 * assumes locked context
 */
ALboolean _alcIsContextSuspended( ALuint cid ) {
	AL_context *cc;

	cc = _alcGetContext( cid );
	if( cc == NULL ) {
		return AL_TRUE;
	}

	return cc->issuspended;
}

/*
 * alcIsExtensionPresent( UNUSED(ALCdevice *device), ALCubyte *extName )
 *
 * Returns AL_TRUE if the alc extension extName is present, AL_FALSE
 * otherwise.
 */
ALCboolean alcIsExtensionPresent( UNUSED(ALCdevice *device), const ALCchar *extName ) {
	return alIsExtensionPresent( extName );
}

#define DEFINE_ALC_PROC(p) { #p, (AL_funcPtr)p }

typedef struct
{
	const ALCchar *name;
	AL_funcPtr value;
} funcNameAddressPair;

funcNameAddressPair alcProcs[] = {
	DEFINE_ALC_PROC(alcCaptureCloseDevice),
	DEFINE_ALC_PROC(alcCaptureOpenDevice),
	DEFINE_ALC_PROC(alcCaptureSamples),
	DEFINE_ALC_PROC(alcCaptureStart),
	DEFINE_ALC_PROC(alcCaptureStop),
	DEFINE_ALC_PROC(alcCloseDevice),
	DEFINE_ALC_PROC(alcCreateContext),
	DEFINE_ALC_PROC(alcDestroyContext),
	DEFINE_ALC_PROC(alcGetContextsDevice),
	DEFINE_ALC_PROC(alcGetCurrentContext),
	DEFINE_ALC_PROC(alcGetEnumValue),
	DEFINE_ALC_PROC(alcGetError),
	DEFINE_ALC_PROC(alcGetIntegerv),
	DEFINE_ALC_PROC(alcGetProcAddress),
	DEFINE_ALC_PROC(alcGetString),
	DEFINE_ALC_PROC(alcIsExtensionPresent),
	DEFINE_ALC_PROC(alcMakeContextCurrent),
	DEFINE_ALC_PROC(alcOpenDevice),
	DEFINE_ALC_PROC(alcProcessContext),
	DEFINE_ALC_PROC(alcSuspendContext)
};

#undef DEFINE_ALC_PROC

static int
compareFuncNameAddressPairs(const void *s1, const void *s2)
{
	const funcNameAddressPair *p1 = (const funcNameAddressPair*)s1;
	const funcNameAddressPair *p2 = (const funcNameAddressPair*)s2;
	return strcmp((const char*)(p1->name), (const char*)(p2->name));
}

static ALCboolean
getStandardProcAddress(AL_funcPtr *value, const ALCchar *funcName)
{
	funcNameAddressPair key;
	funcNameAddressPair *p;
	key.name = funcName;
	p = bsearch(&key, alcProcs,
		    sizeof(alcProcs) / sizeof(alcProcs[0]),
		    sizeof(alcProcs[0]),
		    compareFuncNameAddressPairs);
	if (p == NULL) {
		return ALC_FALSE;
	}
	*value = p->value;
	return ALC_TRUE;
}

static ALCboolean
getExtensionProcAddress( AL_funcPtr *procAddress, UNUSED(ALCdevice *device), const ALCchar *funcName )
{
	/* TODO: using _alGetExtensionProcAddress is a HACK */
	return (_alGetExtensionProcAddress( procAddress, (const ALchar*)funcName) == AL_TRUE) ? ALC_TRUE : ALC_FALSE;
}

/*
 * alcGetProcAddress( UNUSED(ALCdevice *device), ALCubyte *funcName ).
 *
 * Returns the alc extension function named funcName, or NULL if it doesn't
 * exist.
 */
void *
alcGetProcAddress( ALCdevice *device, const ALCchar *funcName )
{
	AL_funcPtr value;
	if (getStandardProcAddress(&value, funcName) == ALC_TRUE) {
		return (void *)value; /* NOTE: The cast is not valid ISO C! */
	}
	if (getExtensionProcAddress(&value, device, funcName) == ALC_TRUE) {
		return (void *)value; /* NOTE: The cast is not valid ISO C! */
	}
	_alcSetError( ALC_INVALID_VALUE );
	return NULL;
}

#define DEFINE_ALC_ENUM(e) { #e, e }

typedef struct
{
	const ALCchar *name;
	ALCenum value;
} enumNameValuePair;

enumNameValuePair alcEnums[] = {
	/* this has to be sorted! */
	DEFINE_ALC_ENUM(ALC_ALL_ATTRIBUTES),
	DEFINE_ALC_ENUM(ALC_ATTRIBUTES_SIZE),
	DEFINE_ALC_ENUM(ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER),
	DEFINE_ALC_ENUM(ALC_CAPTURE_DEVICE_SPECIFIER),
	DEFINE_ALC_ENUM(ALC_CAPTURE_SAMPLES),
	DEFINE_ALC_ENUM(ALC_DEFAULT_DEVICE_SPECIFIER),
	DEFINE_ALC_ENUM(ALC_DEVICE_SPECIFIER),
	DEFINE_ALC_ENUM(ALC_EXTENSIONS),
	DEFINE_ALC_ENUM(ALC_FALSE),
	DEFINE_ALC_ENUM(ALC_FREQUENCY),
	DEFINE_ALC_ENUM(ALC_INVALID_CONTEXT),
	DEFINE_ALC_ENUM(ALC_INVALID_DEVICE),
	DEFINE_ALC_ENUM(ALC_INVALID_ENUM),
	DEFINE_ALC_ENUM(ALC_INVALID_VALUE),
	DEFINE_ALC_ENUM(ALC_MAJOR_VERSION),
	DEFINE_ALC_ENUM(ALC_MINOR_VERSION),
	DEFINE_ALC_ENUM(ALC_MONO_SOURCES),
	DEFINE_ALC_ENUM(ALC_NO_ERROR),
	DEFINE_ALC_ENUM(ALC_OUT_OF_MEMORY),
	DEFINE_ALC_ENUM(ALC_REFRESH),
	DEFINE_ALC_ENUM(ALC_STEREO_SOURCES),
	DEFINE_ALC_ENUM(ALC_SYNC),
	DEFINE_ALC_ENUM(ALC_TRUE)
};

#undef DEFINE_ALC_ENUM

static int
compareEnumNameValuePairs(const void *s1, const void *s2)
{
	const enumNameValuePair *p1 = (const enumNameValuePair*)s1;
	const enumNameValuePair *p2 = (const enumNameValuePair*)s2;
	return strcmp((const char*)(p1->name), (const char*)(p2->name));
}

static ALCboolean
getStandardEnumValue(ALCenum *value, const ALCchar *enumName)
{
	enumNameValuePair key;
	enumNameValuePair *p;
	key.name = enumName;
	p = bsearch(&key, alcEnums,
		    sizeof(alcEnums) / sizeof(alcEnums[0]),
		    sizeof(alcEnums[0]),
		    compareEnumNameValuePairs);
	if (p == NULL) {
		return ALC_FALSE;
	}
	*value = p->value;
	return ALC_TRUE;
}

static ALCboolean
getExtensionEnumValue( UNUSED(ALCenum *value), UNUSED(ALCdevice *device), UNUSED(const ALCchar *enumName) )
{
	/* ToDo: Hook in our extension loader somehow */
	return ALC_FALSE;
}

/*
 * alcGetEnumValue( ALCdevice *device, ALCubyte *enumName )
 *
 * Returns enum value for enumName.
 */
ALCenum
alcGetEnumValue( ALCdevice *device, const ALCchar *enumName )
{
	ALCenum value = 0;
	if (getStandardEnumValue(&value, enumName) == ALC_TRUE) {
		return value;
	}
	if (getExtensionEnumValue(&value, device, enumName) == ALC_TRUE) {
		return value;
	}
	_alcSetError( ALC_INVALID_VALUE );
	return value;
}

ALCdevice *alcGetContextsDevice(ALCcontext *handle)
{
	AL_device *dc;
	AL_context *cc;
	ALuint cid = ALCCONTEXTP_TO_ALUINT( handle );

	_alcLockAllContexts();

	cc = _alcGetContext( cid );
	if( cc == NULL )
	{
		_alcSetError( ALC_INVALID_CONTEXT );
		_alcUnlockAllContexts( );

		return NULL;
	}

	dc = cc->write_device;

	/* just unlock contexts */
	_alcUnlockAllContexts();

	return dc;
}

const ALCchar *alcGetString( ALCdevice *dev, ALCenum token )
{
	switch(token)
	{
		case ALC_DEFAULT_DEVICE_SPECIFIER:
		  return (const ALCchar *) "'((sampling-rate 44100) (device '(native))";
		  break;
		case ALC_DEVICE_SPECIFIER:
		  if(!dev)
		  {
			  _alcSetError(ALC_INVALID_DEVICE);
			  return (const ALCchar *) "";
		  }

		  return dev->specifier;
		  break;
		case ALC_EXTENSIONS:
		  return (const ALCchar *) "";
		  break;
		case ALC_NO_ERROR:
		  return (const ALCchar *) "ALC_NO_ERROR";
		  break;
		case ALC_INVALID_DEVICE:
		  return (const ALCchar *) "ALC_INVALID_DEVICE";
		  break;
		case ALC_INVALID_CONTEXT:
		  return (const ALCchar *) "ALC_INVALID_CONTEXT";
		  break;
		case ALC_INVALID_ENUM:
		  return (const ALCchar *) "ALC_INVALID_ENUM";
		  break;
		case ALC_INVALID_VALUE:
		  return (const ALCchar *) "ALC_INVALID_VALUE";
		  break;
		default:
		  _alcSetError(ALC_INVALID_ENUM);
		  break;
	}

	return (const ALCchar *) "";
}

/* evil */
extern ALint __alcGetAvailableSamples (void);

static AL_context *
_alcGetContextOfDevice (ALCdevice *deviceHandle)
{
  AL_context *cc;

  if (deviceHandle == NULL)
    {
      _alcSetError (ALC_INVALID_DEVICE);
      return NULL;
    }

  cc = deviceHandle->cc;
  if (cc == NULL)
    {
      _alcSetError (ALC_INVALID_CONTEXT);
      return NULL;
    }

  return cc;
}

static int
_alcIsDestinationValid (ALCsizei neededSize, ALCsizei size, ALCint *dest)
{
  if ((neededSize > size) || (dest == NULL))
    {
      _alcSetError (ALC_INVALID_VALUE);
      return 0;
    }
  return 1;
}

/*
 * FIXME: Do we have to do some locking below? Move major/minor to
 * header and copy attributes at context creation time.
 */
void
alcGetIntegerv (ALCdevice *deviceHandle, ALCenum token,
                ALCsizei size, ALCint *dest)
{
  AL_context *cc;
  ALint i;

  switch (token)
    {
    case ALC_ATTRIBUTES_SIZE:
      cc = _alcGetContextOfDevice (deviceHandle);
      if ((cc == NULL) || !_alcIsDestinationValid (1, size, dest))
        {
          return;
        }
      *dest = 2 * cc->NumFlags + 1;
      return;

    case ALC_ALL_ATTRIBUTES:
      cc = _alcGetContextOfDevice (deviceHandle);
      if ((cc == NULL)
          || !_alcIsDestinationValid (2 * cc->NumFlags + 1, size, dest))
        {
          return;
        }
      for (i = 0; i < 2 * cc->NumFlags; i++)
        {
          dest[i] = cc->Flags[i];
        }
      dest[2 * cc->NumFlags] = 0;
      return;

    case ALC_MAJOR_VERSION:
      if (!_alcIsDestinationValid (1, size, dest))
        {
          return;
        }
      *dest = 1;
      return;

    case ALC_MINOR_VERSION:
      if (!_alcIsDestinationValid (1, size, dest))
        {
          return;
        }
      *dest = 0;
      return;

    case ALC_CAPTURE_SAMPLES:
      cc = _alcGetContextOfDevice (deviceHandle);
      if ((cc == NULL) || !_alcIsDestinationValid (1, size, dest))
        {
          return;
        }
      *dest = __alcGetAvailableSamples ();
      return;

    default:
      _alcSetError (ALC_INVALID_ENUM);
      return;
    }
}

/*
 Capture functions
*/

/* Hacked in ALC_EXT_capture support.  --ryan. */
/* This doesn't support multiple devices, device enumeration, or capture */
/*  devices seperate from an existing context. How painful. */

/* ring buffer functionality... */

typedef struct {
	ALubyte *buffer;
	ALsizei size;
	ALsizei write;
	ALsizei read;
	ALsizei used;
} __ALRingBuffer;

static ALboolean __alRingBufferInit( __ALRingBuffer * ring, ALsizei size );
static ALvoid __alRingBufferShutdown( __ALRingBuffer * ring );
static ALsizei __alRingBufferSize( __ALRingBuffer * ring );
static ALvoid __alRingBufferPut( __ALRingBuffer * ring, ALubyte *data,
				 ALsizei size );
static ALsizei __alRingBufferGet( __ALRingBuffer * ring, ALubyte *data,
				  ALsizei size );

static __ALRingBuffer captureRing;

static ALboolean __alRingBufferInit( __ALRingBuffer * ring, ALsizei size )
{
	ALubyte *ptr = ( ALubyte * ) realloc( ring->buffer, size );
	if( ptr == NULL ) {
		return AL_FALSE;
	}

	ring->buffer = ptr;
	ring->size = size;
	ring->write = 0;
	ring->read = 0;
	ring->used = 0;
	return AL_TRUE;
}

static ALvoid __alRingBufferShutdown( __ALRingBuffer * ring )
{
	free( ring->buffer );
	ring->buffer = NULL;
}

static ALsizei __alRingBufferSize( __ALRingBuffer * ring )
{
	return ring->used;
}

static ALvoid __alRingBufferPut( __ALRingBuffer * ring, ALubyte *data,
				 ALsizei _size )
{
	register ALsizei size = _size;
	register ALsizei cpy;
	register ALsizei avail;

	if( !size ) {		/* just in case... */
		return;
	}

	/* Putting more data than ring buffer holds in total? Replace it all. */
	if( size > ring->size ) {
		ring->write = 0;
		ring->read = 0;
		ring->used = ring->size;
		memcpy( ring->buffer, data + ( size - ring->size ),
			ring->size );
		return;
	}

	/* Buffer overflow? Push read pointer to oldest sample not overwritten. */
	avail = ring->size - ring->used;
	if( size > avail ) {
		ring->read += size - avail;
		if( ring->read > ring->size ) {
			ring->read -= ring->size;
		}
	}

	/* Clip to end of buffer and copy first block... */
	cpy = ring->size - ring->write;
	if( size < cpy ) {
		cpy = size;
	}
	if( cpy ) {
		memcpy( ring->buffer + ring->write, data, cpy );
	}

	/* Wrap around to front of ring buffer and copy remaining data... */
	avail = size - cpy;
	if( avail ) {
		memcpy( ring->buffer, data + cpy, avail );
	}

	/* Update write pointer... */
	ring->write += size;
	if( ring->write > ring->size ) {
		ring->write -= ring->size;
	}

	ring->used += size;
	if( ring->used > ring->size ) {
		ring->used = ring->size;
	}
}

static ALsizei __alRingBufferGet( __ALRingBuffer * ring, ALubyte *data,
				  ALsizei _size )
{
	register ALsizei cpy;
	register ALsizei size = _size;
	register ALsizei avail = ring->used;

	/* Clamp amount to read to available data... */
	if( size > avail ) {
		size = avail;
	}

	/* Clip to end of buffer and copy first block... */
	cpy = ring->size - ring->read;
	if( cpy > size ) {
		cpy = size;
	}
	if( cpy ) {
		memcpy( data, ring->buffer + ring->read, cpy );
	}

	/* Wrap around to front of ring buffer and copy remaining data... */
	avail = size - cpy;
	if( avail ) {
		memcpy( data + cpy, ring->buffer, avail );
	}

	/* Update read pointer... */
	ring->read += size;
	if( ring->read > ring->size ) {
		ring->read -= ring->size;
	}

	ring->used -= size;

	return size;	/* may have been clamped if there wasn't enough data... */
}

static ALenum captureFmt = AL_NONE;
static ALuint captureFreq = 0;
static ALint captureFmtSize = 0;

ALCdevice *alcCaptureOpenDevice( const ALCchar *deviceName, ALCuint frequency, ALCenum format, ALCsizei bufferSize )
{
	ALCdevice *retval;
	AL_context *cc;
	ALuint cid;

	if ( deviceName != NULL )  { /* !!! FIXME */
		return NULL;
	}

	switch( format ) { /* try to keep this sane for now... */
        case AL_FORMAT_MONO8:
        case AL_FORMAT_MONO16:
        case AL_FORMAT_STEREO8:
        case AL_FORMAT_STEREO16:
		break;  /* format okay. */
        default:
		return NULL;
	}

	captureFmt = format;
	captureFreq = frequency;
	captureFmtSize = _alGetBitsFromFormat( format ) / 8;
	if( ( format == AL_FORMAT_STEREO8 ) || ( format == AL_FORMAT_STEREO16 ) ) {
		captureFmtSize *= 2;
	}

	bufferSize *= captureFmtSize;

	if ( !__alRingBufferInit( &captureRing, bufferSize )) {
		return NULL;
	}

	if( !alCaptureInit_EXT( format, frequency, bufferSize) ) {
		return NULL;
	}

	cid = _alcCCId;
	_alcLockContext( cid );
	cc = _alcGetContext(cid);
	retval = cc->read_device;
	retval->cc = cc;
	_alcUnlockContext( cid );

	return retval;
}

ALCboolean alcCaptureCloseDevice( ALCdevice *device )
{
	if( device == NULL ) {
		return ALC_FALSE;
	}

	alCaptureDestroy_EXT();
	__alRingBufferShutdown( &captureRing );
	return ALC_TRUE;
}

void alcCaptureStart( UNUSED(ALCdevice *device) )
{
	alCaptureStart_EXT();

}

void alcCaptureStop( UNUSED(ALCdevice *device) )
{
	alCaptureStop_EXT();
}

/* !!! FIXME: Not ideal; reads samples in ALC_CAPTURE_SAMPLES query */
/* !!! FIXME: should query hardware here and do read in alcCaptureSamples() */
ALint __alcGetAvailableSamples( void )
{
	static ALubyte buf[2048];
	ALsizei got;
    
	while ( (got = alCaptureGetData_EXT(buf, sizeof (buf), captureFmt, captureFreq) ) > 0 ) {
		__alRingBufferPut( &captureRing, buf, got );
	}

	/* printf("got %d have %d\n", (int) got, (int) (__alRingBufferSize(&captureRing) / captureFmtSize)); */

	return __alRingBufferSize(&captureRing) / captureFmtSize;
}

void alcCaptureSamples( UNUSED(ALCdevice *device), ALCvoid *buffer, ALCsizei samples )
{
	if( ( __alRingBufferSize(&captureRing) / captureFmtSize ) < samples ) {
		return;  /* !!! FIXME: This is an error condition! */
	}

	__alRingBufferGet( &captureRing, buffer, samples * captureFmtSize );
}
