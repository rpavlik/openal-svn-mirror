#ifndef _AL_MAIN_H_
#define _AL_MAIN_H_

#define AL_MAX_CHANNELS		4
#define AL_MAX_SOURCES		32

#define ALAPI __declspec(dllexport)
#define ALAPIENTRY __cdecl

#include "AL/altypes.h"
#include "AL/alctypes.h"
#include "alu.h"
#include "windows.h"
#include "mmsystem.h"
#include "dsound.h"
#include "alBuffer.h"
#include "alError.h"
#include "alExtension.h"
#include "alListener.h"
#include "alSource.h"
#include "alState.h"
#include "alThunk.h"

#define NUMWAVEBUFFERS	4

typedef struct ALCdevice_struct
{
 	ALboolean	InUse;
 	ALboolean	Valid;

	ALuint		Frequency;
	ALuint		Channels;
	ALenum		Format;

	ALubyte		szDeviceName[256];

	// Maximum number of sources that can be created
	ALuint		MaxNoOfSources;

	// MMSYSTEM Device
	ALboolean	bWaveShutdown;
	HANDLE		hWaveHdrEvent;
	HANDLE		hWaveThreadEvent;
	HANDLE		hWaveThread;
	ALuint		ulWaveThreadID;
	ALint		lWaveBuffersCommitted;
	HWAVEOUT	hWaveHandle;
	WAVEHDR		buffer[NUMWAVEBUFFERS];

	// DirectSound and DirectSound3D Devices
	LPDIRECTSOUND			lpDS;

	// DirectSound Device
	LPDIRECTSOUNDBUFFER		DSpbuffer;
	LPDIRECTSOUNDBUFFER		DSsbuffer;
	MMRESULT				ulDSTimerID;

	// DirectSound3D Device
	LPDIRECTSOUND3DLISTENER lpDS3DListener;
	ALuint					ulDS3DTimerInterval;
	ALuint					ulDS3DTimerID;
} ALCdevice;

typedef struct ALCcontext_struct
{
	ALlistener	Listener;

	ALsource *	Source;
	ALuint		SourceCount;

    ALuint      alPrivateSource;    // Guarantees that there is always one Source in existence

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

	ALboolean	bUseManualAttenuation;

	struct ALCcontext_struct *previous;
	struct ALCcontext_struct *next;
}  ALCcontext;

#endif

#define AL_FORMAT_MONO_IMA4                      0x1300
#define AL_FORMAT_STEREO_IMA4                    0x1301

ALCvoid UpdateContext(ALCcontext *context,ALuint type,ALuint name);
ALint LinearGainToMB(float flGain);

//ALvoid SetGlobalRolloffFactor(ALsource *ALSource);

#define LEVELFLAG_RECALCULATE_ATTENUATION	0x01
#define LEVELFLAG_FORCE_EAX_CALL			0x02
#define LEVELFLAG_USE_DEFERRED				0x80000000

void InitializeManualAttenuation(ALCcontext *pContext);
void SetSourceLevel(ALsource *pSource, ALuint ulFlags);

#ifdef __cplusplus
extern "C"
{
#endif

ALCvoid SuspendContext(ALCcontext *context);
ALCvoid ProcessContext(ALCcontext *context);

#ifdef __cplusplus
}
#endif