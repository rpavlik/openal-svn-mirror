/**
 * Copyright (c) 1999-2004, by authors
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice,
 *       this list of conditions and the following disclaimer in the documentation
 *       and/or other materials provided with the distribution.
 *     * The names of the authors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "OpenAL32/Include/alMain.h"
#include "AL/alc.h"
#include "AL/alut.h"

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

ALUTAPI ALvoid ALUTAPIENTRY alutInit(ALint *argc,ALbyte **argv) 
{
	ALCcontext *Context;
	ALCdevice *Device;
	
	//Open device
 	Device=alcOpenDevice(NULL);
	//Create context(s)
	Context=alcCreateContext(Device,NULL);
	//Set active context
	alcMakeContextCurrent(Context);
	//Register extensions
}

ALUTAPI ALvoid ALUTAPIENTRY alutExit(ALvoid) 
{
	ALCcontext *Context;
	ALCdevice *Device;

	//Unregister extensions

	//Get active context
	Context=alcGetCurrentContext();
	//Get device for active context
	Device=alcGetContextsDevice(Context);
	//Disable context
	alcMakeContextCurrent(NULL);
	//Release context(s)
	alcDestroyContext(Context);
	//Close device
	alcCloseDevice(Device);
}

ALUTAPI ALvoid ALUTAPIENTRY alutLoadWAVFile(ALbyte *file,ALenum *format,ALvoid **data,ALsizei *size,ALsizei *freq, ALboolean *loop)
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
					if (FmtHdr.Format==0x0001)
					{
						*format=(FmtHdr.Channels==1?
								(FmtHdr.BitsPerSample==8?AL_FORMAT_MONO8:AL_FORMAT_MONO16):
								(FmtHdr.BitsPerSample==8?AL_FORMAT_STEREO8:AL_FORMAT_STEREO16));
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
					if (FmtHdr.Format==0x0001)
					{
						*size=ChunkHdr.Size;
						*data=malloc(ChunkHdr.Size+31);
						if (*data) fread(*data,FmtHdr.BlockAlign,ChunkHdr.Size/FmtHdr.BlockAlign,Stream);
						memset(((char *)*data)+ChunkHdr.Size,0,31);
					}
					else if (FmtHdr.Format==0x0011)
					{
						//IMA ADPCM
					}
					else if (FmtHdr.Format==0x0055)
					{
						//MP3 WAVE
					}
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

ALUTAPI ALvoid ALUTAPIENTRY alutLoadWAVMemory(ALbyte *memory,ALenum *format,ALvoid **data,ALsizei *size,ALsizei *freq,ALboolean *loop)
{
	WAVChunkHdr_Struct ChunkHdr;
	WAVFmtExHdr_Struct FmtExHdr;
	WAVFileHdr_Struct FileHdr;
	WAVSmplHdr_Struct SmplHdr;
	WAVFmtHdr_Struct FmtHdr;
	ALbyte *Stream;
	
	*format=AL_FORMAT_MONO16;
	*data=NULL;
	*size=0;
	*freq=22050;
	*loop=AL_FALSE;
	if (memory)
	{
		Stream=memory;
		if (Stream)
		{
			memcpy(&FileHdr,Stream,sizeof(WAVFileHdr_Struct));
			Stream+=sizeof(WAVFileHdr_Struct);
			FileHdr.Size=((FileHdr.Size+1)&~1)-4;
			while ((FileHdr.Size!=0)&&(memcpy(&ChunkHdr,Stream,sizeof(WAVChunkHdr_Struct))))
			{
				Stream+=sizeof(WAVChunkHdr_Struct);
				if (!memcmp(ChunkHdr.Id,"fmt ",4))
				{
					memcpy(&FmtHdr,Stream,sizeof(WAVFmtHdr_Struct));
					if (FmtHdr.Format==0x0001)
					{
						*format=(FmtHdr.Channels==1?
								(FmtHdr.BitsPerSample==8?AL_FORMAT_MONO8:AL_FORMAT_MONO16):
								(FmtHdr.BitsPerSample==8?AL_FORMAT_STEREO8:AL_FORMAT_STEREO16));
						*freq=FmtHdr.SamplesPerSec;
						Stream+=ChunkHdr.Size;
					} 
					else
					{
						memcpy(&FmtExHdr,Stream,sizeof(WAVFmtExHdr_Struct));
						Stream+=ChunkHdr.Size;
					}
				}
				else if (!memcmp(ChunkHdr.Id,"data",4))
				{
					if (FmtHdr.Format==0x0001)
					{
						*size=ChunkHdr.Size;
						*data=malloc(ChunkHdr.Size+31);
						if (*data) memcpy(*data,Stream,ChunkHdr.Size);
						memset(((char *)*data)+ChunkHdr.Size,0,31);
						Stream+=ChunkHdr.Size;
					}
					else if (FmtHdr.Format==0x0011)
					{
						//IMA ADPCM
					}
					else if (FmtHdr.Format==0x0055)
					{
						//MP3 WAVE
					}
				}
				else if (!memcmp(ChunkHdr.Id,"smpl",4))
				{
					memcpy(&SmplHdr,Stream,sizeof(WAVSmplHdr_Struct));
					*loop = (SmplHdr.Loops ? AL_TRUE : AL_FALSE);
					Stream+=ChunkHdr.Size;
				}
				else Stream+=ChunkHdr.Size;
				Stream+=ChunkHdr.Size&1;
				FileHdr.Size-=(((ChunkHdr.Size+1)&~1)+8);
			}
		}
	}
}

ALUTAPI ALvoid ALUTAPIENTRY alutUnloadWAV(ALenum format,ALvoid *data,ALsizei size,ALsizei freq)
{
	if (data)
		free(data);
}
