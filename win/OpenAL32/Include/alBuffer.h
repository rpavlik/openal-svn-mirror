#ifndef _AL_BUFFER_H_
#define _AL_BUFFER_H_

#define ALAPI __declspec(dllexport)
#define ALAPIENTRY __cdecl

#include "AL/altypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UNUSED		0
#define PENDING		1
#define PROCESSED	2

typedef struct ALbuffer_struct 
{
	ALenum		format;
	ALshort *	data;
	ALsizei		size;
	ALsizei		frequency;
	ALuint		writepos;
	ALuint		playpos;
	ALenum		state;
	ALuint		refcount;		// Number of sources using this buffer (deletion can only occur when this is 0)
	struct ALbuffer_struct *previous;
	struct ALbuffer_struct *next;
} ALbuffer;

ALvoid ReleaseALBuffers(ALvoid);

#ifdef __cplusplus
}
#endif

#endif