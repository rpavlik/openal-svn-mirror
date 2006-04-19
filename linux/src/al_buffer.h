/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_buffer.h
 *
 * Prototypes, macros and definitions related to the creation and
 * management of buffers.
 *
 */
#ifndef AL_AL_BUFFER_H_
#define AL_AL_BUFFER_H_

#include "al_siteconfig.h"
#include "al_types.h"

/*
 * Initialize data structures common to all buffers.  Returns AL_TRUE if
 * initialization was successful, AL_FALSE otherwise.
 */
ALboolean _alInitBuffers( void );

/*
 * Destroys data structures created by _alInitBuffers, and any AL_buffer or
 * bpool objects as well.
 */
void _alDestroyBuffers( void );

/*
 * Non locking version of alIsBuffer.
 */
ALboolean _alIsBuffer( ALuint bid );

/*
 * Returns a pointer to the AL_buffer object named by id.  If id does not name
 * a buffer, NULL is returned.
 */
AL_buffer *_alGetBuffer( ALuint id );

/*
 * Returns a pointer to the AL_buffer object associated with the source named
 * by sid from the context named by cid.  If sid in cid is not a valid source,
 * or is a valid source but does not have its buffer attribute set, NULL is
 * returned.
 */
AL_buffer *_alGetBufferFromSid( ALuint cid, ALuint sid );


/**
 * Buffer State functions
 */

/*
 * Returns one member of the set AL_UNUSED, AL_PROCESSED, or AL_PENDING.  If the
 * AL_buffer named by bid is not being used by a playing source, AL_UNUSED is
 * returned.  If it is being used by a playing source, but is part of a queue
 * and is not the current buffer being played, AL_PENDING is returned.
 * Otherwise, AL_PROCESSED is returned.
 */
ALenum _alGetBidState( ALuint bid );

/*
 * Returns one member of the set AL_UNUSED, AL_PROCESSED, or AL_PENDING.  If the
 * AL_buffer (buffer) is not being used by a playing source, AL_UNUSED is
 * returned.  If it is being used by a playing source, but is part of a queue
 * and is not the current buffer being played, AL_PENDING is returned.
 * Otherwise, AL_PROCESSED is returned.
 */
ALenum _alGetBufferState( AL_buffer *buffer );

/*
 * adds a queue reference to the buffer named by bid.  The queue reference
 * refers to the source named by sid.
 *
 * If no current reference is added, and this queue reference is not deleted,
 * _alGet{Bid,Buffer}State will return AL_PENDING.
 *
 */
void _alBidAddQueueRef( ALuint bid, ALuint sid );

/*
 * removes a queue reference to the buffer named by bid.  The first queue
 * reference refering to sid will be removed.
 */
void _alBidRemoveQueueRef( ALuint bid, ALuint sid );

/*
 * adds a current reference to the buffer named by bid.  The reference refers
 * to the source named by sid.
 *
 * If this reference is not removed, _alGet{Bid,Buffer}state will return
 * AL_PROCESSED.
 */
void _alBidAddCurrentRef( ALuint bid, ALuint sid );

/*
 * removes a current reference to the buffer named by bid.  The first current
 * reference refering to sid will be removed.
 */
void _alBidRemoveCurrentRef( ALuint bid, ALuint sid );

/*
 * Returns AL_TRUE if the AL_buffer named by bid is valid and is a streaming
 * buffer ( created by alGenStreamingBuffer_LOKI ), AL_FALSE otherwise.
 */
ALboolean _alBidIsStreaming( ALuint bid );

/*
 * Returns AL_TRUE if the AL_buffer named by bid is valid and is a callback
 * buffer ( has been used with the call alBufferDataWithCallback ), AL_FALSE
 * otherwise.
 */
ALboolean _alBidIsCallback( ALuint bid );

/*
 * Returns AL_TRUE if the AL_buffer (buffer) is a callback buffer ( has been
 * used with the call alBufferDataWithCallback ), AL_FALSE otherwise.
 */
ALboolean _alBufferIsCallback( AL_buffer *buffer );


/*
 * Free the orig_buffers of buf.  Don't just use free(buf->orig_buffer[n]
 * because there are duplicates in there.
 */
void _alBufferFreeOrigBuffers(AL_buffer *buf);

/*
 * Reserves space for num_buffers AL_buffer objects.
 */
void _alNumBufferHint( ALuint num_buffers );

/*
 * Associates the AL_buffer named by bid with a callback.  This is somewhat
 * equivilant to calling alBufferData( bid, ... ), except that instead of
 * getting all the data at once, whenever the buffer is required to provide
 * data, it relies on the callback.
 *
 * The destroyer callbacks are used to update data structures.  They are not
 * available publically, and are only used by internal functions ( in alut,
 * mostly ).
 */
void _alBufferDataWithCallback_LOKI( ALuint bid,
					int (*callback)( ALuint sid,
							 ALuint bid,
							 ALshort *outdata,
							 ALenum format,
							 ALint freq,
							 ALint samples ),
					DestroyCallback_LOKI source_destroyer,
					DestroyCallback_LOKI buffer_destroyer );

/*
 * Calls the source-callback-completion notification function for the
 * buffer associated with the source named by sid.
 */
void _alBidCallDestroyCallbackSource( ALuint sid );

/*
 * Locks the buffer mutex.  fn and ln name the file and line that this call
 * takes place in, for debugging purposes.
 */
ALboolean FL_alLockBuffer( const char *fn, int ln );

/*
 * Unlocks the buffer mutex.  fn and ln name the file and line that this call
 * takes place in, for debugging purposes.
 */
ALboolean FL_alUnlockBuffer( const char *fn, int ln );

/*
 * convert data passed from the format specified by the tuple format and
 * ffreq, into the format specified by tformat and tfreq.  *retsize is set to
 * the resulting size.
 *
 * If should_use_passed_data is AL_TRUE, then the conversion happens
 * in-place, and the return value is equal to data.  Otherwise, a
 * new memory region is alloced, and it is the responsibility of the
 * caller to free it.
 *
 * If no conversion is possible, NULL is returned.
 */
ALvoid *_alBufferCanonizeData( ALenum format, const ALvoid *data, ALuint size, ALuint freq,
			     ALenum tformat, ALuint tfreq, ALuint *retsize,
			     ALenum should_use_passed_data );

/* macros */
#define _alLockBuffer()       FL_alLockBuffer(__FILE__, __LINE__)
#define _alUnlockBuffer()     FL_alUnlockBuffer(__FILE__, __LINE__)

#define _alDCGetBufferFromSid(x)                                    \
	_alGetBufferFromSid((ALuint) _alcCCId, x)

#endif /* not AL_AL_BUFFER_H_ */
