/****************************************************************************

   PROGRAM: audioenv.cpp

   DESCRIPTION: audio object class code

****************************************************************************/
#include "audioenv.h"
#include "Obstruct.h"

#ifdef WINDOWS
#define USE_EAX
#endif

// EAX-test
// add initguid.h under Windows
#ifdef USE_EAX
#define OPENAL
#ifdef WINDOWS
#include <initguid.h>
#else
#define INITGUID
#endif
#include "eax.h"
#endif // USE_EAX

// constructor
AudioEnv::AudioEnv ()
{
	nextBuffer = 0;
	nextSource = 0;
	EAXlevel = 0;
}

// destructor
AudioEnv::~AudioEnv ()
{
	ALCcontext	*pContext;
	ALCdevice	*pDevice;

	// Get active context
	pContext = alcGetCurrentContext();
	if (pContext)
	{
		// Get device for active context
		pDevice = alcGetContextsDevice(pContext);
		
		// Disable context
		alcMakeContextCurrent(NULL);

		// Release context
		alcDestroyContext(pContext);

		// Close device
		alcCloseDevice(pDevice);
	}
}

// init
void AudioEnv::Init ()
{
    ALCdevice *pDevice;
    ALCcontext *pContext;

	// Open device
	pDevice = alcOpenDevice(NULL);
	if (pDevice)
	{
		// Create context
		pContext = alcCreateContext(pDevice,NULL);
		if (pContext)
		{
			// Set active context
			alcMakeContextCurrent(pContext);
		}
	}

   // global settings
   alListenerf(AL_GAIN, 1.0);
   alDopplerFactor(1.0); // don't exaggerate doppler shift
   alDopplerVelocity(343); // using meters/second

#ifdef USE_EAX
   // determine EAX support level
	if (alIsExtensionPresent("EAX2.0") == AL_TRUE)
	{
		EAXlevel = 2;
	} else
	{
		if (alIsExtensionPresent("EAX") == AL_TRUE)
		{
			EAXlevel = 1;
		}
	}

   // set EAX environment if EAX is available
	EAXSet pfPropSet;
	unsigned long ulEAXVal;
	long lGlobalReverb;
	if (EAXlevel != 0)
	{
		pfPropSet = (EAXSet) alGetProcAddress("EAXSet");
		if (pfPropSet != NULL)
		{
		    lGlobalReverb = -10000;
			pfPropSet(&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOM, 0, &lGlobalReverb, sizeof(unsigned long));
			ulEAXVal = EAX_ENVIRONMENT_GENERIC;
			pfPropSet(&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ENVIRONMENT, 0, &ulEAXVal, sizeof(unsigned long));
		}
	}
#endif // USE_EAX
}

// ListenerPostition -- update listener's position and direction
void AudioEnv::ListenerPosition (ALfloat* position, ALfloat* angle)
{
	alListenerfv(AL_POSITION, position);
	alListenerfv(AL_ORIENTATION, angle);
}

// UpdateObstruction -- update obstruction value for a specific source, using EAX if available
void AudioEnv::UpdateObstruction (int handle)
{
	SVector lposition, sposition;

	alGetListenerfv(AL_POSITION, (float *) &lposition);
	alGetSourcefv(source[handle -1], AL_POSITION, (float *) &sposition);

	// location of each "house"
	SVector svMinHouse1, svMinHouse2, svMaxHouse1, svMaxHouse2;
	svMinHouse1.fX = 0;
	svMaxHouse1.fX = 9;
	svMinHouse1.fY = -100;
	svMaxHouse1.fY = 100;
	svMinHouse1.fZ = -9;
	svMaxHouse1.fZ = 0;

	svMinHouse2.fX = 24;
	svMaxHouse2.fX = 33;
	svMinHouse2.fY = -100;
	svMaxHouse2.fY = 100;
	svMinHouse2.fZ = 9;
	svMaxHouse2.fZ = 18;

	// calculate "obstruction level" from 0 to 1
	float obstructionLevel1 = GetObstructionLevel(lposition, sposition, svMinHouse1, svMaxHouse1);
	float obstructionLevel2 = GetObstructionLevel(lposition, sposition, svMinHouse2, svMaxHouse2);

	// use value for the house that obstructs the most (lower value)
	float obstructionLevel;
	if (obstructionLevel1 < obstructionLevel2)
	{
		obstructionLevel = obstructionLevel1;
	} else
	{
		obstructionLevel = obstructionLevel2;
	}

	//exaggerate obstruction level
	obstructionLevel = (obstructionLevel - 0.98f) * 50.0f;
	if (obstructionLevel < 0)
	{
		obstructionLevel = 0;
	}

#ifdef USE_EAX
	if (EAXlevel == 2)
	{
		// have EAX 2.0, so apply the obstruction
		EAXSet pfPropSet = (EAXSet) alGetProcAddress("EAXSet");
		float fvalue = 0.25;
		pfPropSet(&DSPROPSETID_EAX_SourceProperties, DSPROPERTY_EAXBUFFER_OBSTRUCTIONLFRATIO, source[handle-1], &fvalue, sizeof(float));
		long obstruction = (long) ((1.0f - obstructionLevel) * -10000.0f);
		pfPropSet(&DSPROPSETID_EAX_SourceProperties, DSPROPERTY_EAXBUFFER_OBSTRUCTION, source[handle-1], &obstruction, sizeof(long));
	} else
	{
#endif // USE_EAX
		// the non-EAX case should really involve low-pass filtering, but we'll just do a gain control here...
		alSourcef(source[handle -1], AL_GAIN, obstructionLevel);
#ifdef USE_EAX
	}
#endif // USE_EAX
}


#if defined _MSC_VER
	#pragma pack (push,1) 							/* Turn off alignment */
#elif defined __GNUC__
	#define PADOFF_VAR __attribute__((packed))
#endif

#ifndef PADOFF_VAR
	#define PADOFF_VAR
#endif

typedef struct                                  /* WAV File-header */
{
  ALubyte  Id[4]			PADOFF_VAR;
  ALsizei  Size				PADOFF_VAR;
  ALubyte  Type[4]			PADOFF_VAR;
} WAVFileHdr_Struct;

typedef struct                                  /* WAV Fmt-header */
{
  ALushort Format			PADOFF_VAR;
  ALushort Channels			PADOFF_VAR;
  ALuint   SamplesPerSec	PADOFF_VAR;
  ALuint   BytesPerSec		PADOFF_VAR;
  ALushort BlockAlign		PADOFF_VAR;
  ALushort BitsPerSample	PADOFF_VAR;
} WAVFmtHdr_Struct;

typedef struct									/* WAV FmtEx-header */
{
  ALushort Size				PADOFF_VAR;
  ALushort SamplesPerBlock	PADOFF_VAR;
} WAVFmtExHdr_Struct;

typedef struct                                  /* WAV Smpl-header */
{
  ALuint   Manufacturer		PADOFF_VAR;
  ALuint   Product			PADOFF_VAR;
  ALuint   SamplePeriod		PADOFF_VAR;
  ALuint   Note				PADOFF_VAR;
  ALuint   FineTune			PADOFF_VAR;
  ALuint   SMPTEFormat		PADOFF_VAR;
  ALuint   SMPTEOffest		PADOFF_VAR;
  ALuint   Loops			PADOFF_VAR;
  ALuint   SamplerData		PADOFF_VAR;
  struct
  {
    ALuint Identifier		PADOFF_VAR;
    ALuint Type				PADOFF_VAR;
    ALuint Start			PADOFF_VAR;
    ALuint End				PADOFF_VAR;
    ALuint Fraction			PADOFF_VAR;
    ALuint Count			PADOFF_VAR;
  }      Loop[1]			PADOFF_VAR;
} WAVSmplHdr_Struct;

typedef struct                                  /* WAV Chunk-header */
{
  ALubyte  Id[4]			PADOFF_VAR;
  ALuint   Size				PADOFF_VAR;
} WAVChunkHdr_Struct;


#ifdef PADOFF_VAR			    				/* Default alignment */
	#undef PADOFF_VAR
#endif

#if defined _MSC_VER
	#pragma pack (pop)
#endif


ALvoid getWAVData(const ALbyte *file,ALenum *format,ALvoid **data,ALsizei *size,ALsizei *freq, ALboolean *loop)
{
	WAVChunkHdr_Struct ChunkHdr;
	WAVFmtExHdr_Struct FmtExHdr;
	WAVFileHdr_Struct FileHdr;
	WAVSmplHdr_Struct SmplHdr;
	WAVFmtHdr_Struct FmtHdr;
	FILE *Stream;
	
	*format=AL_FORMAT_MONO16;
	*data=NULL;
	*size=0;
	*freq=22050;
	*loop=AL_FALSE;
	if (file)
	{
		Stream=fopen(file,"rb");
		if (Stream)
		{
			fread(&FileHdr,1,sizeof(WAVFileHdr_Struct),Stream);
			FileHdr.Size=((FileHdr.Size+1)&~1)-4;
			while ((FileHdr.Size!=0)&&(fread(&ChunkHdr,1,sizeof(WAVChunkHdr_Struct),Stream)))
			{
				if (!memcmp(ChunkHdr.Id,"fmt ",4))
				{
					fread(&FmtHdr,1,sizeof(WAVFmtHdr_Struct),Stream);
					if ((FmtHdr.Format==0x0001)||(FmtHdr.Format==0xFFFE))
					{
						if (FmtHdr.Channels==1)
							*format=(FmtHdr.BitsPerSample==4?alGetEnumValue("AL_FORMAT_MONO_IMA4"):(FmtHdr.BitsPerSample==8?AL_FORMAT_MONO8:AL_FORMAT_MONO16));
						else if (FmtHdr.Channels==2)
							*format=(FmtHdr.BitsPerSample==4?alGetEnumValue("AL_FORMAT_STEREO_IMA4"):(FmtHdr.BitsPerSample==8?AL_FORMAT_STEREO8:AL_FORMAT_STEREO16));
						*freq=FmtHdr.SamplesPerSec;
						fseek(Stream,ChunkHdr.Size-sizeof(WAVFmtHdr_Struct),SEEK_CUR);
					} 
					else if (FmtHdr.Format==0x0011)
					{
						if (FmtHdr.Channels==1)
							*format=alGetEnumValue("AL_FORMAT_MONO_IMA4");
						else if (FmtHdr.Channels==2)
							*format=alGetEnumValue("AL_FORMAT_STEREO_IMA4");
						*freq=FmtHdr.SamplesPerSec;
						fseek(Stream,ChunkHdr.Size-sizeof(WAVFmtHdr_Struct),SEEK_CUR);
					}
					else if (FmtHdr.Format==0x0055)
					{
						*format=alGetEnumValue("AL_FORMAT_MP3");
						*freq=FmtHdr.SamplesPerSec;
						fseek(Stream,ChunkHdr.Size-sizeof(WAVFmtHdr_Struct),SEEK_CUR);
					}
					else
					{
						fread(&FmtExHdr,1,sizeof(WAVFmtExHdr_Struct),Stream);
						fseek(Stream,ChunkHdr.Size-sizeof(WAVFmtHdr_Struct)-sizeof(WAVFmtExHdr_Struct),SEEK_CUR);
					}
				}
				else if (!memcmp(ChunkHdr.Id,"data",4))
				{
					*size=ChunkHdr.Size;
					*data=malloc(ChunkHdr.Size+31);
					if (*data) fread(*data,FmtHdr.BlockAlign,ChunkHdr.Size/FmtHdr.BlockAlign,Stream);
					memset(((char *)*data)+ChunkHdr.Size,0,31);
				}
				else if (!memcmp(ChunkHdr.Id,"smpl",4))
				{
					fread(&SmplHdr,1,sizeof(WAVSmplHdr_Struct),Stream);
					*loop = (SmplHdr.Loops ? AL_TRUE : AL_FALSE);
					fseek(Stream,ChunkHdr.Size-sizeof(WAVSmplHdr_Struct),SEEK_CUR);
				}
				else fseek(Stream,ChunkHdr.Size,SEEK_CUR);
				fseek(Stream,ChunkHdr.Size&1,SEEK_CUR);
				FileHdr.Size-=(((ChunkHdr.Size+1)&~1)+8);
			}
			fclose(Stream);
		}
		
	}
}

ALvoid unloadWAVData(ALvoid *data)
{
	if (data)
		free(data);
}


// LoadFile -- loads a file into a buffer and source
int AudioEnv::LoadFile (ALbyte *filename, bool loop)
{
   // create buffer
   alGetError(); /* clear */
   alGenBuffers(1, &buffer[nextBuffer]);
   if(alGetError() != AL_NO_ERROR) {
	   return 0;
   }

   // create source
   alGetError(); /* clear */
   alGenSources(1, &source[nextSource]);
   if(alGetError() != AL_NO_ERROR) {
	   return 0;
   }

   // load data into buffer
   ALsizei size, freq;
   ALenum format;
   ALvoid *data;
   ALboolean throwawayLoop;

   getWAVData(filename, &format, &data, &size, &freq, &throwawayLoop);
   alBufferData (buffer[nextBuffer], format, data, size, freq);
   unloadWAVData(data);

   // set static source properties
   alSourcei(source[nextSource], AL_BUFFER, buffer[nextBuffer]);
   alSourcei(source[nextSource], AL_LOOPING, (loop ? AL_TRUE : AL_FALSE));
   alSourcef(source[nextSource], AL_REFERENCE_DISTANCE, 10);

   nextBuffer++;
   nextSource++;
   return nextBuffer;
}

// Playfile -- loads a file into a buffer and source, then plays it
int AudioEnv::PlayFile (ALbyte *filename, bool loop)
{
	int loadhandle;

	loadhandle = LoadFile(filename, loop);

	if (loadhandle != 0) { 
		Play(loadhandle); 
	} else
	{
		return 0;
	}

	return loadhandle;
}

//SetSourcePosition
void AudioEnv::SetSourcePosition (int handle, float *position)
{
	alSourcefv(source[handle-1], AL_POSITION, position);
}

//SetSourceVelocity
void AudioEnv::SetSourceVelocity (int handle, float *velocity)
{
	alSourcefv(source[handle-1], AL_VELOCITY, velocity);
}

// Play
void AudioEnv::Play(int handle)
{
	alSourcePlay(source[handle-1]); 
}

// Stop
void AudioEnv::Stop(int handle)
{
	alSourceStop(source[handle-1]);
}

// IncrementEnv -- steps through the global environment presets if available
int AudioEnv::IncrementEnv()
{
#ifdef USE_EAX
    // increment EAX environment if EAX is available
	EAXSet pfPropSet;
	long lEAXVal;
	static unsigned long ulEAXEnv = 0;
	if (EAXlevel != 0)
	{
		pfPropSet = (EAXSet) alGetProcAddress("EAXSet");
		if (pfPropSet != NULL)
		{
		    lEAXVal = -10000;
			pfPropSet(&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOM, 0, &lEAXVal, sizeof(long));
			ulEAXEnv += 1;
			if (ulEAXEnv >= EAX_ENVIRONMENT_COUNT) { ulEAXEnv = EAX_ENVIRONMENT_GENERIC; }
			pfPropSet(&DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ENVIRONMENT, 0, &ulEAXEnv, sizeof(unsigned long));
		}
	}
	return (int) ulEAXEnv;
#else // not using EAX
	return 0;
#endif // USE_EAX
}



