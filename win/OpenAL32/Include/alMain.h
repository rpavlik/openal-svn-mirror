#ifndef _AL_MAIN_H_
#define _AL_MAIN_H_

#define AL_MAX_CHANNELS		4
#define AL_MAX_SOURCES		32

#define ALAPI __declspec(dllexport)
#define ALAPIENTRY __cdecl

#include "AL/altypes.h"
#include "AL/alctypes.h"
#include "AL/alu.h"
#include "alBuffer.h"
#include "alError.h"
#include "alExtension.h"
#include "alListener.h"
#include "alSource.h"
#include "alState.h"
#include "windows.h"
#include "dsound.h"

#define NUMWAVEBUFFERS	4

typedef struct ALCdevice_struct 
  {
 	ALenum		LastError;
 	ALboolean	InUse;
 	ALboolean	Valid;

	ALuint		Frequency;
	ALuint		Channels;
	ALenum		Format;

	ALint		MajorVersion;
	ALint		MinorVersion;

	ALubyte		szDeviceName[256];

	// Maximum number of sources that can be created
	ALuint		MaxNoOfSources;

	//DirectSound
	LPDIRECTSOUND DShandle;
	LPDIRECTSOUNDBUFFER DSpbuffer;
	LPDIRECTSOUNDBUFFER DSsbuffer;
	LPDIRECTSOUND3DLISTENER DS3dlistener;

	//waveOut
	HWAVEOUT	handle;
	WAVEHDR		buffer[NUMWAVEBUFFERS];

	//mmTimer
	MMRESULT	timer;
  } ALCdevice;
  typedef struct ALCcontext_struct 
  {
	ALlistener	Listener;
	
	ALsource *	Source;
	ALuint		SourceCount;

	ALuint		alPrivateSource;	// Guarantees that there is always one Source in existence
	
	ALenum		LastError;
	ALboolean	InUse;
	ALboolean	Valid;

	ALuint		Frequency;
	ALuint		Channels;
	ALenum		Format;

	ALenum		DistanceModel;

	ALfloat		DopplerFactor;
	ALfloat		DopplerVelocity;

	ALCdevice * Device;

	struct ALCcontext_struct *previous;
	struct ALCcontext_struct *next;
  }  ALCcontext;

#endif

ALCvoid alcUpdateContext(ALCcontext *context,ALuint type,ALuint name);
ALint LinearGainToDB(float flGain);
ALvoid SetGlobalRolloffFactor();

#ifdef __cplusplus
extern "C"
{
#endif

ALCvoid SuspendContext(ALCcontext *context);
ALCvoid ProcessContext(ALCcontext *context);

#ifdef __cplusplus
}
#endif