/*
 * OpenAL Test
 *
 * Copyright (C) 1999-2000 by authors.
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundatio n, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA  02111-1307, USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */

/** notes for gendocs.py
 OpenAL Test Notes

 If there are any core OpenAL API shortcomings exposed by these
 tests at the code level, they are marked in this codebase by
 a comment string "*** API Issue ***".

 There are three classes of tests supported by this program:

    1) Fully Automated Testing
         These tests run entirely by themseves and require no
       user input.
    2) Semi-Automated Testing
         These tests require some analysis by the user. 
    3) Interactive Testing
         These tests allow a user to create their own OpenAL
       scenarios.

 To find the menu in the code, search for the string 
 "*** Main Menu ***".
  */

#define INITGUID
#define OPENAL

//#define TEST_VORBIS // enable for ogg vorbis testing
//#define TEST_EAX // enable for EAX testing

#ifdef _WIN32
#include <windows.h>	// For timeGetTime()
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <al.h>
#include <alc.h>
#include <alut.h>
#include <eax.h>
#endif

#ifdef LINUX
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <memory.h>
#include <math.h>
#ifndef __USE_BSD
#define __USE_BSD
#endif
#include <unistd.h>
#include <sys/stat.h>

#ifdef OSX_FRAMEWORK
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <OpenAL/alut.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>
#endif

#define AL_INVALID_ENUM AL_ILLEGAL_ENUM
#define AL_INVALID_OPERATION AL_ILLEGAL_COMMAND

void alGetSource3f(ALint sid, ALenum param,
		   ALfloat *f1, ALfloat *f2, ALfloat *f3)
{
	ALfloat safety_first[6];

	if(!f1 || !f2 || !f3)
	{
		return;
	}

	alGetSourcefv(sid, param, safety_first);

	*f1 = safety_first[0];
	*f2 = safety_first[1];
	*f3 = safety_first[3];
}
#endif

#ifdef __MACOS__
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <memory.h>
#include <math.h>
#include <string.h>
#include <al.h>
#include <alc.h>
#include <alut.h>
#include <eax.h>
#include <Timer.h>
#define SWAPBYTES
#endif

#ifdef MAC_OS_X
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <memory.h>
#include <math.h>
#include <string.h>
#include <al.h>
p#include <alc.h>
#include <alut.h>
#include <unistd.h>
#define SWAPBYTES
#endif

#define NUM_BUFFERS 8	// Number of buffers to be Generated

#ifndef LINUX
#ifndef MAC_OS_X
#pragma pack (push,1) 							/* Turn off alignment */
#endif
#endif

typedef struct
{
	ALubyte		riff[4];		// 'RIFF'
	ALsizei		riffSize;
	ALubyte		wave[4];		// 'WAVE'
	ALubyte		fmt[4];			// 'fmt '
	ALuint		fmtSize;
	ALushort	Format;                              
	ALushort	Channels;
	ALuint		SamplesPerSec;
	ALuint		BytesPerSec;
	ALushort	BlockAlign;
	ALushort	BitsPerSample;
	ALubyte		data[4];		// 'data'
	ALuint		dataSize;
} WAVE_Struct;

typedef struct ALenum_struct
{
	ALubyte		*enumName;
	ALenum		value;
} ALenums;


// Global variables
ALuint	g_Buffers[NUM_BUFFERS];		// Array of Buffers

#ifdef TEST_EAX
EAXSet	eaxSet;						// EAXSet function, retrieved if EAX Extension is supported
EAXGet	eaxGet;						// EAXGet function, retrieved if EAX Extension is supported
ALboolean g_bEAX;					// Boolean variable to indicate presence of EAX Extension 
#endif

#ifdef TEST_VORBIS
// vorbis extension
typedef ALboolean (vorbisLoader)(ALuint, ALvoid *, ALint);
vorbisLoader *alutLoadVorbisp = NULL;
#endif

static ALenums enumeration[]={
	// Types
	{ (ALubyte *)"AL_INVALID",						AL_INVALID							},
	{ (ALubyte *)"ALC_INVALID",						ALC_INVALID							},
	{ (ALubyte *)"AL_NONE",							AL_NONE								},
	{ (ALubyte *)"AL_FALSE",						AL_FALSE							},
	{ (ALubyte *)"ALC_FALSE",						ALC_FALSE							},
	{ (ALubyte *)"AL_TRUE",							AL_TRUE								},
	{ (ALubyte *)"ALC_TRUE",						ALC_TRUE							},
	
	// Source and Listener Properties
	{ (ALubyte *)"AL_SOURCE_RELATIVE",				AL_SOURCE_RELATIVE					},
	{ (ALubyte *)"AL_CONE_INNER_ANGLE",				AL_CONE_INNER_ANGLE					},
	{ (ALubyte *)"AL_CONE_OUTER_ANGLE",				AL_CONE_OUTER_ANGLE					},
	{ (ALubyte *)"AL_PITCH",						AL_PITCH							},
	{ (ALubyte *)"AL_POSITION",						AL_POSITION							},
	{ (ALubyte *)"AL_DIRECTION",					AL_DIRECTION						},
	{ (ALubyte *)"AL_VELOCITY",						AL_VELOCITY							},
	{ (ALubyte *)"AL_LOOPING",						AL_LOOPING							},
	{ (ALubyte *)"AL_BUFFER",						AL_BUFFER							},
	{ (ALubyte *)"AL_GAIN",							AL_GAIN								},
	{ (ALubyte *)"AL_MIN_GAIN",						AL_MIN_GAIN							},
	{ (ALubyte *)"AL_MAX_GAIN",						AL_MAX_GAIN							},
	{ (ALubyte *)"AL_ORIENTATION",					AL_ORIENTATION						},
	{ (ALubyte *)"AL_REFERENCE_DISTANCE",			AL_REFERENCE_DISTANCE				},
	{ (ALubyte *)"AL_ROLLOFF_FACTOR",				AL_ROLLOFF_FACTOR					},
	{ (ALubyte *)"AL_CONE_OUTER_GAIN",				AL_CONE_OUTER_GAIN					},
	{ (ALubyte *)"AL_MAX_DISTANCE",					AL_MAX_DISTANCE						},

	// Source State information
	{ (ALubyte *)"AL_SOURCE_STATE",					AL_SOURCE_STATE						},
	{ (ALubyte *)"AL_INITIAL",						AL_INITIAL							},
	{ (ALubyte *)"AL_PLAYING",						AL_PLAYING							},
	{ (ALubyte *)"AL_PAUSED",						AL_PAUSED							},
	{ (ALubyte *)"AL_STOPPED",						AL_STOPPED							},

	// Queue information
	{ (ALubyte *)"AL_BUFFERS_QUEUED",				AL_BUFFERS_QUEUED					},
	{ (ALubyte *)"AL_BUFFERS_PROCESSED",			AL_BUFFERS_PROCESSED				},
	
	// Buffer Formats
	{ (ALubyte *)"AL_FORMAT_MONO8",					AL_FORMAT_MONO8						},
	{ (ALubyte *)"AL_FORMAT_MONO16",				AL_FORMAT_MONO16					},
	{ (ALubyte *)"AL_FORMAT_STEREO8",				AL_FORMAT_STEREO8					},
	{ (ALubyte *)"AL_FORMAT_STEREO16",				AL_FORMAT_STEREO16					},

	// Buffer attributes
	{ (ALubyte *)"AL_FREQUENCY",					AL_FREQUENCY						},
	{ (ALubyte *)"AL_SIZE",							AL_SIZE								},

	// Buffer States (not supported yet)
	{ (ALubyte *)"AL_UNUSED",						AL_UNUSED							},
	{ (ALubyte *)"AL_PENDING",						AL_PENDING							},
	{ (ALubyte *)"AL_PROCESSED",					AL_PROCESSED						},

	// ALC Properties
	{ (ALubyte *)"ALC_MAJOR_VERSION",				ALC_MAJOR_VERSION					},
	{ (ALubyte *)"ALC_MINOR_VERSION",				ALC_MINOR_VERSION					},
	{ (ALubyte *)"ALC_ATTRIBUTES_SIZE",				ALC_ATTRIBUTES_SIZE					},
	{ (ALubyte *)"ALC_ALL_ATTRIBUTES",				ALC_ALL_ATTRIBUTES					},
	{ (ALubyte *)"ALC_DEFAULT_DEVICE_SPECIFIER",	ALC_DEFAULT_DEVICE_SPECIFIER		},
	{ (ALubyte *)"ALC_DEVICE_SPECIFIER",			ALC_DEVICE_SPECIFIER				},
	{ (ALubyte *)"ALC_EXTENSIONS",					ALC_EXTENSIONS						},
	{ (ALubyte *)"ALC_FREQUENCY",					ALC_FREQUENCY						},
	{ (ALubyte *)"ALC_REFRESH",						ALC_REFRESH							},
	{ (ALubyte *)"ALC_SYNC",						ALC_SYNC							},

	// AL Error Messages
	{ (ALubyte *)"AL_NO_ERROR",						AL_NO_ERROR							},
	{ (ALubyte *)"AL_INVALID_NAME",					AL_INVALID_NAME						},
	{ (ALubyte *)"AL_INVALID_ENUM",					AL_INVALID_ENUM						},
	{ (ALubyte *)"AL_INVALID_VALUE",				AL_INVALID_VALUE					},
	{ (ALubyte *)"AL_INVALID_OPERATION",			AL_INVALID_OPERATION				},
	{ (ALubyte *)"AL_OUT_OF_MEMORY",				AL_OUT_OF_MEMORY					},
	
	// ALC Error Message
	{ (ALubyte *)"ALC_NO_ERROR",					ALC_NO_ERROR						},
	{ (ALubyte *)"ALC_INVALID_DEVICE",				ALC_INVALID_DEVICE					},
	{ (ALubyte *)"ALC_INVALID_CONTEXT",				ALC_INVALID_CONTEXT					},
	{ (ALubyte *)"ALC_INVALID_ENUM",				ALC_INVALID_ENUM					},
	{ (ALubyte *)"ALC_INVALID_VALUE",				ALC_INVALID_VALUE					},
	{ (ALubyte *)"ALC_OUT_OF_MEMORY",				ALC_OUT_OF_MEMORY					},

	// Context strings
	{ (ALubyte *)"AL_VENDOR",						AL_VENDOR							},
	{ (ALubyte *)"AL_VERSION",						AL_VERSION							},
	{ (ALubyte *)"AL_RENDERER",						AL_RENDERER							},
	{ (ALubyte *)"AL_EXTENSIONS",					AL_EXTENSIONS						},
	
	// Global states
	{ (ALubyte *)"AL_DOPPLER_FACTOR",				AL_DOPPLER_FACTOR					},
	{ (ALubyte *)"AL_DOPPLER_VELOCITY",				AL_DOPPLER_VELOCITY					},
	{ (ALubyte *)"AL_DISTANCE_MODEL",				AL_DISTANCE_MODEL					},
	
	// Distance Models
	{ (ALubyte *)"AL_INVERSE_DISTANCE",				AL_INVERSE_DISTANCE					},
	{ (ALubyte *)"AL_INVERSE_DISTANCE_CLAMPED",		AL_INVERSE_DISTANCE_CLAMPED			},

	// Default
	{ (ALubyte *)NULL,								(ALenum  ) 0 						} };


// Function prototypes
void delay_ms(unsigned int ms);
ALvoid DisplayALError(ALbyte *szText, ALint errorcode);

#ifdef LINUX
ALvoid DisplayALError(const char *text, ALint errorcode);
#endif

char getUpperCh(void);
int CRToContinue(void);
void CRForNextTest(void);
int ContinueOrSkip(void);
ALvoid FullAutoTests(ALvoid);
ALvoid SemiAutoTests(ALvoid);

#ifdef __MACOS__
void SwapWords(unsigned int *puint);
void SwapBytes(unsigned short *pshort);
#endif
#ifdef MAC_OS_X
void SwapWords(unsigned int *puint);
void SwapBytes(unsigned short *pshort);
#endif

// Test Function prototypes
ALvoid I_PositionTest(ALvoid);
ALvoid I_LoopingTest(ALvoid);
ALvoid I_QueueTest(ALvoid);
ALvoid I_BufferTest(ALvoid);
ALvoid I_FreqTest(ALvoid);
ALvoid I_StereoTest(ALvoid);
ALvoid I_GainTest(ALvoid);
ALvoid I_StreamingTest(ALvoid);
ALvoid I_MultipleSourcesTest(ALvoid);
ALvoid FA_RequestObjectNames(ALvoid);
ALvoid FA_ReleaseObjectNames(ALvoid);
ALvoid FA_ValidateObjectNames(ALvoid);
ALvoid FA_StateTransition(ALvoid);
ALvoid FA_VectorStateTransition(ALvoid);
ALvoid FA_GetBufferProperties(ALvoid);
ALvoid FA_EnumerationValue(ALvoid);
ALvoid FA_QueuingUnderrunStates(ALvoid);
ALvoid SA_StringQueries(ALvoid);
ALvoid SA_SourceGain(ALvoid);
ALvoid SA_ListenerGain(ALvoid);
ALvoid SA_Position(ALvoid);
ALvoid SA_SourceRelative(ALvoid);
ALvoid SA_ListenerOrientation(ALvoid);
ALvoid SA_SourceCone(ALvoid);
ALvoid SA_MinMaxGain(ALvoid);
ALvoid SA_ReferenceDistance(ALvoid);
ALvoid SA_RolloffFactor(ALvoid);
ALvoid SA_DistanceModel(ALvoid);
ALvoid SA_Doppler(ALvoid);
ALvoid SA_Frequency(ALvoid);
ALvoid SA_Stereo(ALvoid);
ALvoid SA_Streaming(ALvoid);
ALvoid SA_QueuingUnderrunPerformance(ALvoid);

#ifdef TEST_VORBIS
ALvoid I_VorbisTest(ALvoid);
#endif

#ifdef TEST_EAX
ALvoid I_EAXTest(ALvoid);
#endif

/*
	delay_ms -- delay by given number of milliseconds
*/
void delay_ms(unsigned int ms)
{
#ifdef __MACOS__
	UnsignedWide endTime, currentTime;
	
	Microseconds(&currentTime);
	endTime = currentTime;
	endTime.lo += ms * 1000;
	
	while (endTime.lo > currentTime.lo)
	{
		Microseconds(&currentTime);
	}
#endif
#ifdef MAC_OS_X
	usleep(ms * 1000);
#endif
#ifdef LINUX
	usleep(ms * 1000);
#endif   
#ifdef _WIN32
	int startTime;
	startTime = timeGetTime();
	while ((startTime + ms) > timeGetTime());
#endif
}

/*
	Display AL Error message
*/
ALvoid DisplayALError(ALbyte *szText, ALint errorcode)
{
	printf("%s%s\n",szText,alGetString(errorcode));
	return;
}

#ifdef LINUX
ALvoid DisplayALError( const char *text, ALint errorcode)
{
	printf("%s%s\n", text, alGetString(errorcode));

	return;
}
#endif


/*
	Main application - Initializes Open AL, Sets up Listener, Generates Buffers, and loads in audio data.
	Displays Test Menu and calls appropriate Test functions.  On exit, buffers are destroyed and Open AL
	is shutdown.

	Each Test function is responsible for creating and destroying whatever Sources they require. All test
	applications use the same set of Buffers (created in this function).
*/
int main(int argc, char* argv[])
{
	ALubyte szFnName[128];
	ALbyte	ch;
	ALint	error;
	ALsizei size,freq;
	ALenum	format;
	ALvoid	*data;
	ALboolean loop;
#ifndef LINUX
        ALCcontext *Context;
#else
        ALvoid *Context;
#endif
	ALCdevice *Device;
	ALfloat listenerPos[]={0.0,0.0,0.0};
	ALfloat listenerVel[]={0.0,0.0,0.0};
	ALfloat	listenerOri[]={0.0,0.0,-1.0, 0.0,1.0,0.0};	// Listener facing into the screen

#ifdef TEST_EAX
	ALubyte szEAX[] = "EAX";
#endif	


	printf("OpenAL Test Application\n");
	printf("=======================\n\n");

	// Initialize Open AL manually
	//Open device
 	Device=alcOpenDevice((ALubyte*)"DirectSound3D");
	//Create context(s)
	Context=alcCreateContext(Device,NULL);
	//Set active context
	alcMakeContextCurrent(Context);

	// Clear Error Code
	alGetError();

	// Set Listener attributes

	// Position ...
	alListenerfv(AL_POSITION,listenerPos);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alListenerfv POSITION : ", error);
		exit(-1);
	}

	// Velocity ...
	alListenerfv(AL_VELOCITY,listenerVel);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alListenerfv VELOCITY : ", error);
		exit(-1);
	}

	// Orientation ...
	alListenerfv(AL_ORIENTATION,listenerOri);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alListenerfv ORIENTATION : ", error);
		exit(-1);
	}

	// Generate Buffers
	alGenBuffers(NUM_BUFFERS, g_Buffers);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alGenBuffers :", error);
		exit(-1);
	}

	// Load in samples to be used by Test functions
	// Load footsteps.wav
#ifdef LINUX
        ALsizei bits;
        alutLoadWAV("Footsteps.wav", &data, &format, &size, & bits, &freq);
#endif
#ifdef _WIN32
	alutLoadWAVFile("footsteps.wav",&format,&data,&size,&freq,&loop);
#endif
#ifdef __MACOS__
	alutLoadWAVFile((char *) "footsteps.wav",&format,&data,&size,&freq);
#endif
#ifdef MAC_OS_X
	alutLoadWAVFile((char *) "footsteps.wav",&format,&data,&size,&freq);
#endif
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alutLoadWAVFile footsteps.wav : ", error);
		// Delete Buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Copy footsteps.wav data into AL Buffer 0
	alBufferData(g_Buffers[0],format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alBufferData buffer 0 : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Unload footsteps.wav
#ifndef LINUX
        alutUnloadWAV(format,data,size,freq);
#else
        free(data);
#endif
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alutUnloadWAV : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Load ding.wav
#ifdef LINUX
        alutLoadWAV("ding.wav", &data, &format, &size, & bits, &freq);
#endif
#ifdef _WIN32
	alutLoadWAVFile("ding.wav",&format,&data,&size,&freq,&loop);
#endif
#ifdef __MACOS__
	alutLoadWAVFile((char *) "ding.wav",&format,&data,&size,&freq);
#endif
#ifdef MAC_OS_X
	alutLoadWAVFile((char *) "ding.wav",&format,&data,&size,&freq);
#endif
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alutLoadWAVFile ding.wav : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit (-1);
	}

	// Copy ding.wav audio data into AL Buffer 1
	alBufferData(g_Buffers[1],format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alBufferData buffer 1 : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}
	
	// Unload ding.wav
#ifndef LINUX
        alutUnloadWAV(format,data,size,freq);
#else
        free(data);
#endif
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alutUnloadWAV : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

		// Load wave1.wav
#ifdef LINUX
        alutLoadWAV("Wave1.WAV", &data, &format, &size, & bits, &freq);
#endif
#ifdef _WIN32
	alutLoadWAVFile("wave1.wav",&format,&data,&size,&freq,&loop);
#endif
#ifdef __MACOS__
	alutLoadWAVFile((char *) "wave1.wav",&format,&data,&size,&freq);
#endif
#ifdef MAC_OS_X
	alutLoadWAVFile((char *) "wave1.wav",&format,&data,&size,&freq);
#endif
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alutLoadWAVFile wave1.wav : ", error);
		// Delete Buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Copy wave1.wav data into AL Buffer 2
	alBufferData(g_Buffers[2],format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alBufferData buffer 2 : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Unload wave1.wav
#ifndef LINUX
        alutUnloadWAV(format,data,size,freq);
#else
        free(data);
#endif
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alutUnloadWAV : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

		// Load wave2.wav
#ifdef LINUX
        alutLoadWAV("Wave2.WAV", &data, &format, &size, & bits, &freq);
#endif
#ifdef _WIN32
	alutLoadWAVFile("wave2.wav",&format,&data,&size,&freq,&loop);
#endif
#ifdef __MACOS__
	alutLoadWAVFile((char *) "wave2.wav",&format,&data,&size,&freq);
#endif
#ifdef MAC_OS_X
	alutLoadWAVFile((char *) "wave2.wav",&format,&data,&size,&freq);
#endif
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alutLoadWAVFile wave2.wav : ", error);
		// Delete Buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Copy wave2.wav data into AL Buffer 3
	alBufferData(g_Buffers[3],format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alBufferData buffer 3 : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Unload wave2.wav
#ifndef LINUX
        alutUnloadWAV(format,data,size,freq);
#else
        free(data);
#endif
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alutUnloadWAV : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

		// Load wave3.wav
#ifdef LINUX
        alutLoadWAV("Wave3.WAV", &data, &format, &size, & bits, &freq);
#endif
#ifdef _WIN32
	alutLoadWAVFile("wave3.wav",&format,&data,&size,&freq,&loop);
#endif
#ifdef __MACOS__
	alutLoadWAVFile((char *) "wave3.wav",&format,&data,&size,&freq);
#endif
#ifdef MAC_OS_X
	alutLoadWAVFile((char *) "wave3.wav",&format,&data,&size,&freq);
#endif
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alutLoadWAVFile wave3.wav : ", error);
		// Delete Buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Copy wave3.wav data into AL Buffer 4
	alBufferData(g_Buffers[4],format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alBufferData buffer 4 : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Unload wave3.wav
#ifndef LINUX
        alutUnloadWAV(format,data,size,freq);
#else
        free(data);
#endif
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alutUnloadWAV : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Load wave4.wav
#ifdef LINUX
        alutLoadWAV("Wave4.WAV", &data, &format, &size, & bits, &freq);
#endif
#ifdef _WIN32
	alutLoadWAVFile("wave4.wav",&format,&data,&size,&freq,&loop);
#endif
#ifdef __MACOS__
	alutLoadWAVFile((char *) "wave4.wav",&format,&data,&size,&freq);
#endif
#ifdef MAC_OS_X
	alutLoadWAVFile((char *) "wave4.wav",&format,&data,&size,&freq);
#endif
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alutLoadWAVFile wave4.wav : ", error);
		// Delete Buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Copy wave4.wav data into AL Buffer 5
	alBufferData(g_Buffers[5],format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alBufferData buffer 5 : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Unload wave4.wav
#ifndef LINUX
        alutUnloadWAV(format,data,size,freq);
#else
        free(data);
#endif
        if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alutUnloadWAV : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Load stereo.wav
#ifdef LINUX
        alutLoadWAV("stereo.wav", &data, &format, &size, & bits, &freq);
#endif
#ifdef _WIN32
	alutLoadWAVFile("stereo.wav",&format,&data,&size,&freq,&loop);
#endif
#ifdef __MACOS__
	alutLoadWAVFile((char *) "stereo.wav",&format,&data,&size,&freq);
#endif
#ifdef MAC_OS_X
	alutLoadWAVFile((char *) "stereo.wav",&format,&data,&size,&freq);
#endif
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alutLoadWAVFile stereo.wav : ", error);
		// Delete Buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Copy stereo.wav data into AL Buffer 6
	alBufferData(g_Buffers[6],format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alBufferData buffer 6 : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Unload stereo.wav
#ifndef LINUX
        alutUnloadWAV(format,data,size,freq);
#else
        free(data);
#endif
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alutUnloadWAV : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

#ifdef TEST_VORBIS
     // Load boom.ogg
     struct stat sbuf;
     if (stat("boom.ogg", &sbuf) != -1) 
     {
        int size;
	size = sbuf.st_size;
	void *data;
	data = malloc(size);
	
	if (data != NULL) 
	  {
	     FILE *fh;
	     fh = fopen("boom.ogg", "rb");
	     if (fh != NULL) 
	       {
		  fread(data, size, 1, fh);
		  
		  alutLoadVorbisp = (vorbisLoader *) alGetProcAddress((ALubyte *) "alutLoadVorbis_LOKI");
		  if (alutLoadVorbisp != NULL) 
		    {		       
		       // Copy boom.ogg data into AL Buffer 7
		       if (alutLoadVorbisp(g_Buffers[7], data, size) != AL_TRUE) {
			  DisplayALError((ALbyte *) "alBufferData buffer 7 : ", error);
			  // Delete buffers
			  alDeleteBuffers(NUM_BUFFERS, g_Buffers);
			  exit(-1);
		       }
		    }
		  
		  
	       }
	     
	     // Unload boom.ogg
	     free(data);
	     fclose(fh);
	  }
     }
#endif

#ifdef TEST_EAX
	// Check for EAX extension
	g_bEAX = alIsExtensionPresent(szEAX);

        if (g_bEAX)
		printf("EAX Extensions available\n");

	sprintf((char*)szFnName, "EAXSet");

	eaxSet = (EAXSet)alGetProcAddress(szFnName);

	if (eaxSet == NULL)
		g_bEAX = false;

	sprintf((char*)szFnName,"EAXGet");

	eaxGet = (EAXGet)alGetProcAddress(szFnName);

	if (eaxGet == NULL)
		g_bEAX = false;
#endif

	/* *** Main Menu *** */
	do
	{
		printf("\n\n\nAutomated Test Series:\n\n");
		printf("A) Run Fully Automated Tests\n");
		printf("B) Run Semi-Automated Tests\n");
		printf("\nInteractive Tests:\n\n");
		printf("1) Position Test\n");
		printf("2) Looping Test\n");
		printf("3) Queue Test\n");
		printf("4) Buffer Test\n");
		printf("5) Frequency Test\n");
		printf("6) Stereo Test\n");
		printf("7) Gain Test\n");
		printf("8) Streaming Test\n");
		printf("9) Multiple Sources Test\n");
#ifdef TEST_EAX
		printf("C) EAX Test\n");
#endif
#ifdef TEST_VORBIS
		printf("D) Ogg Vorbis Test\n");
#endif
		printf("\nQ) to quit\n\n\n");

		ch = getUpperCh();

		switch (ch)
		{
			case 'A':
				FullAutoTests();
				break;
			case 'B':
				SemiAutoTests();
				break;
			case '1':
				I_PositionTest();
				break;
			case '2':
				I_LoopingTest();
				break;
			case '3':
				I_QueueTest();
				break;
			case '4':
				I_BufferTest();
				break;
			case '5':
				I_FreqTest();
				break;
			case '6':
				I_StereoTest();
				break;
			case '7':
				I_GainTest();
				break;
			case '8':
				I_StreamingTest();
				break;
			case '9':
				I_MultipleSourcesTest();
				break;
#ifdef TEST_EAX
			case 'C':
				if (g_bEAX)
					I_EAXTest();
				break;
#endif
#ifdef TEST_VORBIS
		        case 'D':
		                I_VorbisTest();
		                break;
#endif
			default:
				break;
		}
	} while (ch != 'Q');

	alGetError(); // clear error state
	alDeleteBuffers(NUM_BUFFERS, g_Buffers);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alDeleteBuffers : ", error);
		exit(-1);
	}

	//Get active context
	Context=alcGetCurrentContext();
	//Get device for active context
	Device=alcGetContextsDevice(Context);
	//Release context(s)
	alcDestroyContext(Context);
	//Close device
	alcCloseDevice(Device);

	printf("\nProgram Terminated\n");
	
	return 0;
}


/*
 	Fully Automatic Tests
*/
ALvoid FullAutoTests(ALvoid)
{	
	FA_RequestObjectNames();  // Request Object Names
	FA_ReleaseObjectNames();  // Release Object Names
	FA_ValidateObjectNames();  // Validating Object Names
	FA_StateTransition();  // State Transistion Testing
	FA_VectorStateTransition();  // Vector State Transistion Testing
	FA_GetBufferProperties();  // Get Buffer Properties
	FA_EnumerationValue(); // Enumeration Value Test
	FA_QueuingUnderrunStates(); // test underrun while queuing

	printf("\n\n");
	delay_ms(1000);
}

char getUpperCh(void)
{
	char ch;
	
#ifdef _WIN32
	ch = _getch();
#else
	int bs;
	bs = getchar();
	while (bs != 10)
	{
		ch = bs;
		bs = getchar();
	}
#endif

 	ch = toupper(ch);
 		
 	return ch;
 }

int CRToContinue()
{
	ALubyte ch = 0;
	ALubyte lastchar = 0;
	
	do 
	{ 
		lastchar = ch;
		ch = getchar(); 
	} while (ch != 10);
	
	return lastchar;
}

void CRForNextTest()
{   
	printf("\nPress Return to continue on to the next test.\n");	

	CRToContinue();
}

int ContinueOrSkip()
{
    ALubyte ch;
    
	printf("\nPress Return to run this test, or 'S' to skip:\n");	

	while (1)
	{
		ch = CRToContinue();
		if ((ch == 'S') || (ch == 's'))
		{
			return 0;
		} 
		if (ch == 0)
		{
			return 1;
		}
	}
}

/*
 	Semi Automatic Tests
*/
ALvoid SemiAutoTests(ALvoid)
{
	SA_StringQueries();  // String Queries Test
	SA_SourceGain();  // Source Gain Test
	SA_ListenerGain();  // Listener Gain Test
	SA_Position();  // Position Test
	SA_SourceRelative();  // Source Relative Test
	SA_ListenerOrientation();  // Listener Orientation Test
	SA_SourceCone();  // Source Cone Test
	SA_MinMaxGain();  // MIN/MAX Gain Test
	SA_ReferenceDistance();  // Reference Distance Test
	SA_RolloffFactor();  // Rolloff Factor Test
	SA_DistanceModel();  // Distance Model Test
	SA_Doppler();  // Doppler Test
	SA_Frequency();  // Frequency Test
	SA_Stereo();  // Stereo Test
	SA_Streaming(); // Streaming Test
	SA_QueuingUnderrunPerformance(); // test underrun performance

	printf("\nDone with this series of tests. Going back to the main menu...");
	delay_ms(1000);
}


/** used by gendocs.py
$SECTION Fully Automatic Tests
$SUBTITLE Request Object Names
This test checks that OpenAL responds correctly to the creation of either
zero or -1 objects.  The two object types are Buffers and Sources.  When
zero objects are requested, the call should be the equivalent of a NOP.
when -1 objects are requested, an AL_INVALID_VALUE error should be generated.
*/
ALvoid FA_RequestObjectNames(ALvoid)
{
	bool localResultOK;
	ALuint testSources[4], testBuffers[4];
	ALint error;

	printf("\nRequest Object Names Test. ");
	alGetError(); // clear error state
	localResultOK = true;
	alGenBuffers(0, testBuffers); // should be equivalent to NOP
	error = alGetError();
	if (error != AL_NO_ERROR)
	{
		localResultOK = false;
	}
	alGenSources(0, testSources); // should be equivalent to NOP
	error = alGetError();
	if (error != AL_NO_ERROR)
	{
		localResultOK = false;
	}
	alGenBuffers(-1, testBuffers); // invalid -- make sure error code comes back
	error = alGetError();
	if (error == AL_NO_ERROR)
	{
		localResultOK = false;
	}
	alGenSources(-1, testSources); // invalid -- make sure error code comes back
	error = alGetError();
	if (error == AL_NO_ERROR)
	{
		localResultOK = false;
	}
	if (localResultOK == true)
	{
		printf("PASSED.");
	} else
	{
		printf("FAILED.");
	}
}

/** used by gendocs.py
$SECTION Fully Automatic Tests
$SUBTITLE Release Object Names
This test checks that OpenAL responds correctly to the deletion of -1 objects.
The two object types are Buffers and Sources.  When -1 objects are requested, an
AL_INVALID_VALUE error should be generated.
*/
ALvoid FA_ReleaseObjectNames (ALvoid)
{
	bool localResultOK;
	ALuint testSources[4], testBuffers[4];
	ALint error;

	printf("\nReleasing Object Names Test. ");
	alGetError();
	localResultOK = true;
	alDeleteBuffers(-1, testBuffers); // invalid -- make sure error code comes back
	error = alGetError();
	if (error == AL_NO_ERROR)
	{
		localResultOK = false;
	}
	alDeleteSources(-1, testSources); // invalid -- make sure error code comes back
	error = alGetError();
	if (error == AL_NO_ERROR)
	{
		localResultOK = false;
	}
	if (localResultOK == true)
	{
		printf("PASSED.");
	} else
	{
		printf("FAILED.");
	}
}
	
/** used by gendocs.py
$SECTION Fully Automatic Tests
$SUBTITLE Validating Object Names
This test checks that OpenAL can test the validity of a source or buffer.  A
check is made on valid objects for a positive result, and a check is made on
invalid objects to confirm a negative result.
*/
ALvoid FA_ValidateObjectNames (ALvoid)
{
	bool localResultOK;
	ALuint testSources[4], testBuffers[4];
	ALint error;

	printf("\nValidating Object Names Test. ");
	alGetError();
	localResultOK = true;
	alGenBuffers(1, testBuffers);
	alGenSources(1, testSources);
	error = alGetError();
	if (error != AL_NO_ERROR)
	{
		localResultOK = false;
	} else
	{
		if (alIsBuffer(testBuffers[0]) == AL_FALSE) // this buffer should test as valid
		{
			localResultOK = false;
		}
		if (alIsSource(testSources[0]) == AL_FALSE) // this source should test as valid
		{
			localResultOK = false;
		}
		if (alIsBuffer(testBuffers[0] + 1) == AL_TRUE) // this buffer should be invalid
		{
			localResultOK = false;
		}
		if (alIsSource(testSources[0] + 1) == AL_TRUE) // this source should be invalid
		{
			localResultOK = false;
		}
		alDeleteBuffers(1, testBuffers);
		alDeleteSources(1, testSources);
	}
	if (localResultOK == true)
	{
		printf("PASSED.");
	} else
	{
		printf("FAILED.");
	}
}

/** used by gendocs.py
$SECTION Fully Automatic Tests
$SUBTITLE State Transition Testing
This test checks that OpenAL can monitor the state of a source properly.  The
source is tested to make sure it can run through all its possible states --
AL_INITIAL, AL_PLAYING, AL_PAUSED, and AL_STOPPED.
*/
ALvoid FA_StateTransition (ALvoid)
{
	bool localResultOK;
	ALuint testSources[4];

	printf("\nState Transition Test. ");
	alGetError();
	localResultOK = true;
	alGenSources(1, testSources);
	alSourcei(testSources[0], AL_BUFFER, g_Buffers[0]);
	alSourcei(testSources[0], AL_LOOPING, AL_TRUE);
	ALint sourceState;
	alGetSourcei(testSources[0], AL_SOURCE_STATE, &sourceState);
	if (sourceState != AL_INITIAL)
	{
		localResultOK = false;
	}
	alSourcePlay(testSources[0]);
	delay_ms(500);
	alGetSourcei(testSources[0], AL_SOURCE_STATE, &sourceState);
	if (sourceState != AL_PLAYING)
	{
		localResultOK = false;
	}
	alSourcePause(testSources[0]);
	delay_ms(500);
	alGetSourcei(testSources[0], AL_SOURCE_STATE, &sourceState);
	if (sourceState != AL_PAUSED)
	{
		localResultOK = false;
	}
	alSourcePlay(testSources[0]);
	delay_ms(500);
	alGetSourcei(testSources[0], AL_SOURCE_STATE, &sourceState);
	if (sourceState != AL_PLAYING)
	{
		localResultOK = false;
	}
	alSourceStop(testSources[0]);
	delay_ms(500);
	alGetSourcei(testSources[0], AL_SOURCE_STATE, &sourceState);
	if (sourceState != AL_STOPPED)
	{
		localResultOK = false;
	}
	if (localResultOK == true)
	{
		printf("PASSED.");
	} else
	{
		printf("FAILED.");
	}
	alDeleteSources(1, testSources);
}

/** used by gendocs.py
$SECTION Fully Automatic Tests
$SUBTITLE Vector State Transition Testing
This test checks that OpenAL can monitor the state of multiple sources which
are being controlled using vectors.  The sources are tested to make sure
they properly run through all its possible states -- AL_INITIAL, AL_PLAYING,
AL_PAUSED, and AL_STOPPED.
*/	
ALvoid FA_VectorStateTransition (ALvoid)
{
	bool localResultOK;
	ALuint testSources[4];
	ALenum sourceState;

	printf("\nVector State Transition Test. ");
	alGetError();
	localResultOK = true;
	alGenSources(2, testSources);
	alSourcei(testSources[0], AL_BUFFER, g_Buffers[0]);
	alSourcei(testSources[1], AL_BUFFER, g_Buffers[1]);
	alSourcei(testSources[0], AL_LOOPING, AL_TRUE);
	alSourcei(testSources[1], AL_LOOPING, AL_TRUE);
	alGetSourcei(testSources[0], AL_SOURCE_STATE, &sourceState);
	if (sourceState != AL_INITIAL)
	{
		localResultOK = false;
		printf("FAILED -- AL_INITIAL 0");
	}
	alGetSourcei(testSources[1], AL_SOURCE_STATE, &sourceState);
	if (sourceState != AL_INITIAL)
	{
		localResultOK = false;
		printf("FAILED -- AL_INITIAL 1");
	}
	alSourcePlayv(2, &testSources[0]);
	delay_ms(500);
	alGetSourcei(testSources[0], AL_SOURCE_STATE, &sourceState);
	if (sourceState != AL_PLAYING)
	{
		localResultOK = false;
		printf("FAILED -- AL_PLAYING 0");
	}
	alGetSourcei(testSources[1], AL_SOURCE_STATE, &sourceState);
	if (sourceState != AL_PLAYING)
	{
		localResultOK = false;
		printf("FAILED -- AL_PLAYING 1");
	}
	alSourcePausev(2, &testSources[0]);
	delay_ms(500);
	alGetSourcei(testSources[0], AL_SOURCE_STATE, &sourceState);
	if (sourceState != AL_PAUSED)
	{
		localResultOK = false;
		printf("FAILED -- AL_PAUSED 0");
	}
	alGetSourcei(testSources[1], AL_SOURCE_STATE, &sourceState);
	if (sourceState != AL_PAUSED)
	{
		localResultOK = false;
		printf("FAILED -- AL_PAUSED 1");
	}
	alSourcePlayv(2, &testSources[0]);
	delay_ms(500);
	alGetSourcei(testSources[0], AL_SOURCE_STATE, &sourceState);
	if (sourceState != AL_PLAYING)
	{
		localResultOK = false;
		printf("FAILED -- AL_PLAYING 0A");
	}
	alGetSourcei(testSources[1], AL_SOURCE_STATE, &sourceState);
	if (sourceState != AL_PLAYING)
	{
		localResultOK = false;
		printf("FAILED -- AL_PLAYING 1A");
	}
	alSourceStopv(2, &testSources[0]);
	delay_ms(500);
	alGetSourcei(testSources[0], AL_SOURCE_STATE, &sourceState);
	if (sourceState != AL_STOPPED)
	{
		localResultOK = false;
		printf("FAILED -- AL_STOPPED 0");
	}
	alGetSourcei(testSources[1], AL_SOURCE_STATE, &sourceState);
	if (sourceState != AL_STOPPED)
	{
		localResultOK = false;
		printf("FAILED -- AL_STOPPED 1");
	}
	if (localResultOK == true)
	{
		printf("PASSED.");
	} else
	{
		printf("FAILED.");
	}
	alDeleteSources(2, testSources);
}


/** used by gendocs.py
$SECTION Fully Automatic Tests
$SUBTITLE Get Buffer Properties Test
This test checks that OpenAL can retrieve buffer properties properly.
*/
ALvoid FA_GetBufferProperties(ALvoid)
{
	ALint freq, bits, ch, size;
	bool passNULL;

	printf("\nGet Buffer Properties Test. ");
	alGetBufferi(g_Buffers[0], AL_FREQUENCY, &freq);
	alGetBufferi(g_Buffers[0], AL_BITS, &bits);
	alGetBufferi(g_Buffers[0], AL_CHANNELS, &ch);
	alGetBufferi(g_Buffers[0], AL_SIZE, &size);
	
	passNULL = !(alIsBuffer(NULL));  // the NULL buffer should cause alIsBuffer to be FALSE

	if ((freq == 44100) && (bits == 16) && (ch == 1) && (size == 282626) && (passNULL == true))
	{
		printf("PASSED.");
	} else
	{
		printf("FAILED.");
	}
}


/** used by gendocs.py
$SECTION Fully Automatic Tests
$SUBTITLE Enumeration Value Test
This test checks that the implementation's enumeration values are correct.
*/
ALvoid FA_EnumerationValue(ALvoid)
{
	bool result = true;
	int i = 0;
	int getVal;

	printf("\nEnumeration Value Test. ");

	while (enumeration[i].enumName)
	{
		getVal = alGetEnumValue(enumeration[i].enumName);
		if (getVal != enumeration[i].value)
		{
			printf("\n%s has an invalid enum value.", enumeration[i].enumName);
			result = false;
		}
		i++;
	}
	
	if (result == true)
	{
		printf("PASSED.");
	} else
	{
		printf("FAILED.");
	}
}


/** used by gendocs.py
$SECTION Fully Automatic Tests
$SUBTITLE Queuing Underrun States
This test checks that OpenAL handles state progressions properly during a streaming underrun.
*/
ALvoid FA_QueuingUnderrunStates(ALvoid)
{
	ALuint testSources[1];
	ALuint bufferName;
	ALuint error;
	ALint tempInt;
	ALfloat	listenerOri[]={0.0,0.0,-1.0, 0.0,1.0,0.0};
	bool localResultOK;

	printf("\nQueuing Underrun States Test. ");
	localResultOK = true;
	alGetError();
	alGenSources(1, testSources);
	alSourcei(testSources[0], AL_BUFFER, 0);
	alSourcei(testSources[0], AL_LOOPING, AL_FALSE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "Init error : ", error);
	alSourceQueueBuffers(testSources[0], 1, &g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR) localResultOK = false;
	alSourcePlay(testSources[0]);
	delay_ms(1000);
	alGetSourcei(testSources[0], AL_SOURCE_STATE, &tempInt);
	if (tempInt != AL_STOPPED) localResultOK = false;
	alGetSourcei(testSources[0], AL_BUFFERS_PROCESSED, &tempInt);
	if (tempInt != 1)
	{
		localResultOK = false;
	} else
	{
		alSourceUnqueueBuffers(testSources[0], tempInt, &bufferName);
	}
	alSourceQueueBuffers(testSources[0], 1, &g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR) localResultOK = false;
	alSourcePlay(testSources[0]);
	delay_ms(100);
	alGetSourcei(testSources[0], AL_SOURCE_STATE, &tempInt);
	if (tempInt != AL_PLAYING) localResultOK = false;
		
	// cleanup
	alSourcei(testSources[0], AL_BUFFER, 0);
	alDeleteSources(1, testSources);
	
	// display result
	if (localResultOK == true)
	{
		printf("PASSED.");
	} else
	{
		printf("FAILED.");
	}
}



/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE String Queries Test
This test outputs the renderer, version #, vendor, and extensions string so that
the user can confirm that the string values are correct.
*/	
ALvoid SA_StringQueries(ALvoid)
{
	printf("String Queries Test:");
	if (ContinueOrSkip())
	{
		printf("Check that the following values are reasonable:\n");

#ifdef LINUX
		const ALubyte *tempString;
#else
		unsigned char *tempString;
#endif

		ALCcontext* pContext;
		ALCdevice* pDevice;
		pContext = alcGetCurrentContext();
		pDevice = alcGetContextsDevice(pContext);
		tempString = alcGetString(pDevice, ALC_DEVICE_SPECIFIER);
		printf("OpenAL Context Device Specifier is '%s'\n", tempString);
		tempString = alGetString(AL_RENDERER);
		printf("OpenAL Renderer is '%s'\n", tempString);
		tempString = alGetString(AL_VERSION);
		printf("OpenAL Version is '%s'\n", tempString);
		tempString = alGetString(AL_VENDOR);
		printf("OpenAL Vendor is '%s'\n", tempString);
		tempString = alGetString(AL_EXTENSIONS);
		printf("OpenAL Extensions supported are :\n%s\n", tempString);
		printf("\nError Codes are :-\n");
		printf("AL_NO_ERROR : '%s'\n", tempString = alGetString(AL_NO_ERROR));

#ifdef LINUX
		printf("AL_ILLEGAL_ENUM : '%s'\n", tempString = alGetString(AL_ILLEGAL_ENUM));
#else
// ***** FIXME ***** warning: Check the spec: pretty sure INVALID_ENUM changed to ILLEGAL_ENUM
		printf("AL_INVALID_ENUM : '%s'\n", tempString = alGetString(AL_INVALID_ENUM));
#endif
		printf("AL_INVALID_VALUE : '%s'\n", tempString = alGetString(AL_INVALID_VALUE));

#ifdef LINUX
		printf("AL_ILLEGAL_COMMAND: '%s'\n", tempString = alGetString(AL_ILLEGAL_COMMAND));
#else
// ***** FIXME ***** warning Check the spec: pretty sure INVALID_OPERATION changed to ILLEGAL_COMMAND
		printf("AL_INVALID_OPERATION : '%s'\n", tempString = alGetString(AL_INVALID_OPERATION));
#endif
		printf("AL_OUT_OF_MEMORY : '%s'\n", tempString = alGetString(AL_OUT_OF_MEMORY));
		CRForNextTest();
	}
}



/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Source Gain Test
This test outputs a source at multiple gain values for testing by the user.
*/	
ALvoid SA_SourceGain(ALvoid)
{
	ALuint testSources[2];
	ALfloat	listenerOri[]={0.0,0.0,-1.0, 0.0,1.0,0.0};

	printf("Source Gain Test:");
	if (ContinueOrSkip())
	{
		// load up sources
		alGenSources(1, testSources);
		alSourcei(testSources[0], AL_BUFFER, g_Buffers[1]);
	
		printf("The following sound effect will be played at full source gain (Press Return):\n");
		CRToContinue();
		alSourcef(testSources[0],AL_GAIN,1.0f);
		alSourcePlay(testSources[0]);
		printf("The following sound effect will be played at half source gain (Press Return):\n");
		CRToContinue();
		alSourcef(testSources[0],AL_GAIN,0.5f);
		alSourcePlay(testSources[0]);
		printf("The following sound effect will be played at quarter source gain (Press Return):\n");
		CRToContinue();
		alSourcef(testSources[0],AL_GAIN,0.25f);
		alSourcePlay(testSources[0]);
		printf("The following sound effect will be played at 1/20th source gain (Press Return):\n");
		CRToContinue();
		alSourcef(testSources[0],AL_GAIN,0.05f);
		alSourcePlay(testSources[0]);
		CRForNextTest();
		alSourcef(testSources[0],AL_GAIN,1.0f); 
		
		// dispose of sources
		alSourcei(testSources[0], AL_BUFFER, NULL);
		alDeleteSources(1, testSources);
	}
}



/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Listener Gain Test
This test outputs a source at a fixed gain level, and tests various listener gain levels.
*/
ALvoid SA_ListenerGain(ALvoid)
{	
	ALuint testSources[2];
	ALfloat	listenerOri[]={0.0,0.0,-1.0, 0.0,1.0,0.0};

	printf("Listener Gain Test:");
	if (ContinueOrSkip())
	{
		// load up sources
		alGenSources(1, testSources);
		alSourcei(testSources[0], AL_BUFFER, g_Buffers[1]);
	
		printf("The following sound effect will be played at full listener gain (Press Return):\n");
		CRToContinue();
		alListenerf(AL_GAIN,1.0f);
		alSourcePlay(testSources[0]);
		printf("The following sound effect will be played at half listener gain (Press Return):\n");
		CRToContinue();
		alListenerf(AL_GAIN,0.5f);
		alSourcePlay(testSources[0]);
		printf("The following sound effect will be played at quarter listener gain (Press Return):\n");
		CRToContinue();
		alListenerf(AL_GAIN,0.25f);
		alSourcePlay(testSources[0]);
		printf("The following sound effect will be played at 1/20th listener gain (Press Return):\n");
		CRToContinue();
		alListenerf(AL_GAIN,0.05f);
		alSourcePlay(testSources[0]);
		CRForNextTest();
		alListenerf(AL_GAIN,1.0f);
		float f;
		alGetListenerf(AL_GAIN, &f);
		if (f != 1.0) { printf("ERROR:  alGetListenerf failed.\n"); }
		alSourceStop(testSources[0]);
		
		// dispose of sources
		alSourcei(testSources[0], AL_BUFFER, NULL);
		alDeleteSources(1, testSources);
	} 
}
	

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Position Test
This tests various source/listener positions, as well as the AL_POSITION get functions.
*/	
ALvoid SA_Position(ALvoid)
{
	ALuint testSources[2];
	int i;
	ALfloat	listenerOri[]={0.0,0.0,-1.0, 0.0,1.0,0.0};
	ALfloat tempFVect[6];

	printf("Position Test:");
	if (ContinueOrSkip())
	{
		// load up sources
		alGenSources(1, testSources);
		alSourcei(testSources[0], AL_BUFFER, g_Buffers[1]);
	
		printf("Trying Left-to-Right sweep by moving listener(Press Return):\n");
		CRToContinue();
		alSourcei(testSources[0], AL_LOOPING, true);
		alListener3f(AL_POSITION, 100.0, 0.0, 0.0);
		alGetListener3f(AL_POSITION, &tempFVect[0], &tempFVect[1], &tempFVect[2]);
		if ((tempFVect[0] != 100.0) || (tempFVect[1] != 0.0) || (tempFVect[2] != 0.0))
		{
			printf("ERROR: alGetListener3f(AL_POSITION, ...).\n");
		}
		alGetListenerfv(AL_POSITION, tempFVect);
		if ((tempFVect[0] != 100.0) || (tempFVect[1] != 0.0) || (tempFVect[2] != 0.0))
		{
			printf("ERROR: alGetListenerfv(AL_POSITION, ...).\n");
		}
		alSourcePlay(testSources[0]);
		for (i = -100; i < 100; i++)
		{
			alListener3f(AL_POSITION, (float) -i, 0.0, 0.0);		
			delay_ms(100);
		}
		alListener3f(AL_POSITION, 0.0, 0.0, 0.0);
		alSourceStop(testSources[0]);
		printf("Trying Left-to-Right sweep by moving source (Press Return):\n");
		CRToContinue();
		alSourcei(testSources[0], AL_LOOPING, true);
		alSource3f(testSources[0], AL_POSITION, -100.0, 0.0, 0.0);
		alSourcePlay(testSources[0]);
		for (i = -100; i < 100; i++)
		{
			alSource3f(testSources[0], AL_POSITION, (float) i, 0.0, 0.0);		
			delay_ms(100);
		}
		alSource3f(testSources[0], AL_POSITION, 0.0, 0.0, 0.0);
		alSourceStop(testSources[0]);
		printf("Trying Back-to-Front sweep (Press Return):\n");
		CRToContinue();
		alSourcei(testSources[0], AL_LOOPING, true);
		alSource3f(testSources[0], AL_POSITION, 0.0, 0.0, -100.0);
		alGetSource3f(testSources[0], AL_POSITION, &tempFVect[0], &tempFVect[1], &tempFVect[2]);
		if ((tempFVect[0] != 0.0) || (tempFVect[1] != 0.0) || (tempFVect[2] != -100.0))
		{
			printf("ERROR: alGetSource3f(..., AL_POSITION, ...).\n");
		}
		alGetSourcefv(testSources[0], AL_POSITION, tempFVect);
		if ((tempFVect[0] != 0.0) || (tempFVect[1] != 0.0) || (tempFVect[2] != -100.0))
		{
			printf("ERROR: alGetSourcefv(..., AL_POSITION, ...).\n");
		}
		alSourcePlay(testSources[0]);
		for (i = -100; i < 100; i++)
		{
			alSource3f(testSources[0], AL_POSITION, 0.0, 0.0, (float) -i);		
			delay_ms(100);
		}
		alSource3f(testSources[0], AL_POSITION, 0.0, 0.0, 0.0);
		alSourceStop(testSources[0]);
		CRForNextTest(); 
		
		// dispose of sources
		alSourcei(testSources[0], AL_BUFFER, 0);
		alDeleteSources(1, testSources);
	}
}
	

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Source Relative Test
This tests the source relative mode.
*/	
ALvoid SA_SourceRelative(ALvoid)
{
	ALuint testSources[2];
	int i;
	ALfloat	listenerOri[]={0.0,0.0,-1.0, 0.0,1.0,0.0};

	printf("Source Relative Test:");
	if (ContinueOrSkip())
	{
		// load up sources
		alGenSources(1, testSources);
		alSourcei(testSources[0], AL_BUFFER, g_Buffers[1]);
		
		printf("Placing Listener at (100, 0, 0) and sweeping source from (0, 0, 0) to (100, 0, 0).  The sound should pan from left to center (Press Return):\n");
		CRToContinue();
		alListener3f(AL_POSITION, 100.0, 0.0, 0.0);
		alListenerfv(AL_ORIENTATION, listenerOri);
		alSourcei(testSources[0], AL_LOOPING, true);
		alSource3f(testSources[0], AL_POSITION, -10.0, 0.0, 0.0);
		alSourcePlay(testSources[0]);
		for (i = 00; i < 100; i++)
		{
			alSource3f(testSources[0], AL_POSITION, (float) i, 0.0, 0.0);
			delay_ms(100);
		}
		alSource3f(testSources[0], AL_POSITION, 0.0, 0.0, 0.0);
		alSourceStop(testSources[0]);

		printf("Turning on source relative mode, placing Listener at (100, 0, 0), and sweeping source from (0, 0, 0) to (100, 0, 0).  The sound should pan from center to right (Press Return):\n");
		CRToContinue();
		alSourcei(testSources[0], AL_SOURCE_RELATIVE, AL_TRUE);
		alSourcei(testSources[0], AL_LOOPING, true);
		alSource3f(testSources[0], AL_POSITION, -100.0, 0.0, 0.0);
		alSourcePlay(testSources[0]);
		for (i = 0; i < 100; i++)
		{
			alSource3f(testSources[0], AL_POSITION, (float) i, 0.0, 0.0);
			delay_ms(100);
		}

		alListener3f(AL_POSITION, 0.0, 0.0, 0.0);
		alSourcei(testSources[0], AL_SOURCE_RELATIVE, AL_FALSE);
		alSource3f(testSources[0], AL_POSITION, 0.0, 0.0, 0.0);
		alSourceStop(testSources[0]);
		CRForNextTest(); 
		
		// dispose of sources
		alSourcei(testSources[0], AL_BUFFER, 0);
		alDeleteSources(1, testSources);
	}
}


/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Listener Orientation Test
This test moves and orients the listener around a fixed source.
*/
ALvoid SA_ListenerOrientation (ALvoid)
{	
	ALuint testSources[2];
	ALfloat	listenerOri[]={0.0,0.0,-1.0, 0.0,1.0,0.0};

	printf("Listener Orientation Test:");
	if (ContinueOrSkip())
	{
		// load up sources
		alGenSources(1, testSources);
		alSourcei(testSources[0], AL_BUFFER, g_Buffers[1]);
	
		printf("The listener will be placed at (1, 0, 0) and will face the -X direction.  The sound should be centered. (Press Return):\n");
		CRToContinue();
		alSource3f(testSources[0], AL_POSITION, 0.0, 0.0, 0.0);
		alListenerf(AL_GAIN,1.0f);
		float f;
		alGetSourcef(testSources[0], AL_GAIN, &f);
		if (f != 1.0) { printf("ERROR: alGetSourcef(..., AL_GAIN, ...).\n"); }
		alListener3f(AL_POSITION, 1.0, 0.0, 0.0);
		listenerOri[0]=-1.0;
		listenerOri[1]=0.0;
		listenerOri[2]=0.0;
		listenerOri[3]=0.0;
		listenerOri[4]=1.0;
		listenerOri[5]=0.0;
		alListenerfv(AL_ORIENTATION, listenerOri);
		alSourcei(testSources[0], AL_LOOPING, true);
		alSourcePlay(testSources[0]);
		delay_ms(4000);
		alSourceStop(testSources[0]);
		
		printf("The listener will now be oriented down the -Z axis.  The sound should be to the left. (Press Return):\n");
		CRToContinue();
		listenerOri[0]=0.0;
		listenerOri[1]=0.0;
		listenerOri[2]=-1.0;
		listenerOri[3]=0.0;
		listenerOri[4]=1.0;
		listenerOri[5]=0.0;
		alListenerfv(AL_ORIENTATION, listenerOri);
		alSourcePlay(testSources[0]);
		delay_ms(4000);
		alSourceStop(testSources[0]);
		
		printf("The listener will now be turned upside-down (the 'up' direction will be (0, -1, 0)).  The sound should be to the right. (Press Return):\n");
		CRToContinue();
		listenerOri[0]=0.0;
		listenerOri[1]=0.0;
		listenerOri[2]=-1.0;
		listenerOri[3]=0.0;
		listenerOri[4]=-1.0;
		listenerOri[5]=0.0;
		alListenerfv(AL_ORIENTATION, listenerOri);
		alSourcePlay(testSources[0]);
		delay_ms(4000);
		alSourceStop(testSources[0]);
		
		printf("The listener will now be oriented down the +Z axis (and the 'up' direction is now (0, 1, 0) again).  The sound should be to the right. (Press Return):\n");
		CRToContinue();
		listenerOri[0]=0.0;
		listenerOri[1]=0.0;
		listenerOri[2]=1.0;
		listenerOri[3]=0.0;
		listenerOri[4]=1.0;
		listenerOri[5]=0.0;
		alListenerfv(AL_ORIENTATION, listenerOri);
		alSourcePlay(testSources[0]);
		delay_ms(4000);
		alSourceStop(testSources[0]);
		
		CRForNextTest();
		alListenerf(AL_GAIN,1.0f);
		alListener3f(AL_POSITION, 0.0, 0.0, 0.0);
		listenerOri[0]=0.0;
		listenerOri[1]=0.0;
		listenerOri[2]=-1.0;
		listenerOri[3]=0.0;
		listenerOri[4]=1.0;
		listenerOri[5]=0.0;
		alListenerfv(AL_ORIENTATION, listenerOri);
		
		// dispose of sources
		alSourcei(testSources[0], AL_BUFFER, 0);
		alDeleteSources(1, testSources);
	} 
}


/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Source Cone Test
This test exercises source cones.
*/
ALvoid SA_SourceCone (ALvoid)
{	
	ALuint testSources[2];
	ALfloat	listenerOri[]={0.0,0.0,-1.0, 0.0,1.0,0.0};
	ALfloat	sourceDir[]={0.0,0.0,1.0};
	ALfloat	sourceDir2[]={1.0,0.0,1.0};
	ALfloat	sourceDir3[]={0.0,0.0,-1.0};

	printf("Source Cone Test:");
	if (ContinueOrSkip())
	{
		// load up sources
		alGenSources(1, testSources);
		alSourcei(testSources[0], AL_BUFFER, g_Buffers[1]);
	
		printf("The listener will be at (0,0,0).  The source will be at (0,0,-1).  The source will be directly facing the listener and should be loud. (Press Return):\n");
		CRToContinue();
		alListener3f(AL_POSITION, 0.0, 0.0, 0.0);
		alListenerfv(AL_ORIENTATION, listenerOri);
		alSourcef(testSources[0], AL_CONE_INNER_ANGLE, 10.0);
		alSourcef(testSources[0], AL_CONE_OUTER_ANGLE, 270.0);
		alSourcef(testSources[0], AL_CONE_OUTER_GAIN, (float)0.01);
		alSource3f(testSources[0], AL_POSITION, 0.0, 0.0, -1.0);
		alSourcefv(testSources[0], AL_DIRECTION, sourceDir);
		alSourcePlay(testSources[0]);
		
		printf("The source will now point between the inner and outer cones, and should be at medium volume. (Press Return):\n");
		CRToContinue();
		alSourcefv(testSources[0], AL_DIRECTION, sourceDir2);
		alSourcePlay(testSources[0]);

		printf("The source will now point behind the outer cone and will be at low volume. (Press Return):\n");
		CRToContinue();
		alSourcefv(testSources[0], AL_DIRECTION, sourceDir3);
		alSourcePlay(testSources[0]);
		
		CRForNextTest();
		
		// dispose of sources
		alSourcei(testSources[0], AL_BUFFER, 0);
		alDeleteSources(1, testSources);
	} 
}


/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE MIN/MAX Gain Test
This test checks if minimum and maximum gain settings are working.
*/
ALvoid SA_MinMaxGain(ALvoid)
{	
	ALuint testSources[2];
	ALfloat	listenerOri[]={0.0,0.0,-1.0, 0.0,1.0,0.0};

	printf("MIN/MAX Gain Test:");
	if (ContinueOrSkip())
	{
		// load up sources
		alGenSources(1, testSources);
		alSourcei(testSources[0], AL_BUFFER, g_Buffers[1]);
	
		printf("The source will be played at GAIN 1.0 with MAX gain set to 1.0. This should be high volume. (Press Return):\n");
		CRToContinue();
		alSourcef(testSources[0], AL_GAIN, 1.0);
		alSourcef(testSources[0], AL_MAX_GAIN, 1.0);
		alSourcei(testSources[0], AL_LOOPING, false);
		alSourcePlay(testSources[0]);

		printf("The source will be played at GAIN 0.1 with MIN gain set to 0.6.  This should be at medium volume. (Press Return):\n");
		CRToContinue();
		alSourcef(testSources[0], AL_GAIN, (float) 0.1);
		alSourcef(testSources[0], AL_MIN_GAIN, (float) 0.6);
		alSourcePlay(testSources[0]);
		
		printf("The source will be played at GAIN 1.0 with MAX gain set to 0.1.  This should be low volume. (Press Return):\n");
		CRToContinue();
		alSourcef(testSources[0], AL_GAIN, 1.0);
		alSourcef(testSources[0], AL_MAX_GAIN, (float) 0.1);
		alSourcePlay(testSources[0]);
		
		printf("The source will be played at GAIN 0.1 with MIN gain set to 0.0.  This should be low volume. (Press Return):\n");
		CRToContinue();
		alSourcef(testSources[0], AL_GAIN, (float) 0.1);
		alSourcef(testSources[0], AL_MIN_GAIN, 0.0);
		alSourcePlay(testSources[0]);

		printf("The source will be played at GAIN 1.0 with MAX gain set to 0.0.  This should be zero volume. (Press Return):\n");
		CRToContinue();
		alSourcef(testSources[0], AL_GAIN, (float) 1.0);
		alSourcef(testSources[0], AL_MAX_GAIN, 0.0);
		alSourcePlay(testSources[0]);
		
		CRForNextTest();
		alSourcef(testSources[0], AL_GAIN, 1.0);
		alSourcef(testSources[0], AL_MAX_GAIN, 1.0);
		alSourcef(testSources[0], AL_MIN_GAIN, 0.0);
		
		// dispose of sources
		alSourcei(testSources[0], AL_BUFFER, 0);
		alDeleteSources(1, testSources);
	} 
}
		

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Reference Distance Test
This test exercises a source's reference distance.
*/
ALvoid SA_ReferenceDistance(ALvoid)
{
	ALuint testSources[2];
	ALfloat	listenerOri[]={0.0,0.0,-1.0, 0.0,1.0,0.0};

	printf("Reference Distance Test:");
	if (ContinueOrSkip())
	{
		// load up sources
		alGenSources(1, testSources);
		alSourcei(testSources[0], AL_BUFFER, g_Buffers[1]);
	
		printf("The source will be placed at (0, 0, -10), and the reference distance set at 1.0. This should be low volume. (Press Return):\n");
		CRToContinue();
		alSource3f(testSources[0], AL_POSITION, 0.0, 0.0, -10.0);
		alSourcef(testSources[0], AL_REFERENCE_DISTANCE, 1.0);
		alSourcei(testSources[0], AL_LOOPING, false);
		alSourcePlay(testSources[0]);
		
		printf("The source will be played with the reference distance set to 3.0.  This should be medium volume. (Press Return):\n");
		CRToContinue();
		alSourcef(testSources[0], AL_REFERENCE_DISTANCE, 3.0);
		alSourcePlay(testSources[0]);
		
		printf("The source will be played with the reference distance set to 10.0.  This should be high volume. (Press Return):\n");
		CRToContinue();
		alSourcef(testSources[0], AL_REFERENCE_DISTANCE, 10.0);
		alSourcePlay(testSources[0]);
		
		CRForNextTest();
		alSource3f(testSources[0], AL_POSITION, 0.0, 0.0, 0.0);
		alSourcef(testSources[0], AL_REFERENCE_DISTANCE, 1.0);
		
		// dispose of sources
		alSourcei(testSources[0], AL_BUFFER, 0);
		alDeleteSources(1, testSources);
	} 
}
	

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Rolloff Factor Test
This test exercises a source's rolloff factor.
*/	
ALvoid SA_RolloffFactor(ALvoid)
{
	ALuint testSources[2];
	ALfloat	listenerOri[]={0.0,0.0,-1.0, 0.0,1.0,0.0};

	printf("Rolloff Factor Test:");
	if (ContinueOrSkip())
	{
		// load up sources
		alGenSources(1, testSources);
		alSourcei(testSources[0], AL_BUFFER, g_Buffers[1]);
	
		printf("The source will be played with the rolloff factor set to 0.0.  This should be high volume. (Press Return):\n");
		CRToContinue();
		alSource3f(testSources[0], AL_POSITION, 0.0, 0.0, -10.0);
		alSourcei(testSources[0], AL_LOOPING, false);
		alSourcef(testSources[0], AL_ROLLOFF_FACTOR, 0.0);
		alSourcePlay(testSources[0]);

		printf("The source will be placed at (0, 0, -10), and the rolloff factor set at 1.0. This should be medium volume. (Press Return):\n");
		CRToContinue();
		alSourcef(testSources[0], AL_ROLLOFF_FACTOR, 1.0);
		alSourcePlay(testSources[0]);
		
		printf("The source will be played with the rolloff factor set to 3.0.  This should be low volume. (Press Return):\n");
		CRToContinue();
		alSourcef(testSources[0], AL_ROLLOFF_FACTOR, 3.0);
		alSourcePlay(testSources[0]);
		
		printf("The source will be played with the rolloff factor set to 10.0.  This should be very low volume. (Press Return):\n");
		CRToContinue();
		alSourcef(testSources[0], AL_ROLLOFF_FACTOR, 10.0);
		alSourcePlay(testSources[0]);
		
		CRForNextTest();
		
		// dispose of sources
		alSourcei(testSources[0], AL_BUFFER, 0);
		alDeleteSources(1, testSources);
	} 
}
	

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Distance Model Test
This test exercises the three distance models.
*/
ALvoid SA_DistanceModel(ALvoid)
{	
	ALuint testSources[2];
	ALfloat	listenerOri[]={0.0,0.0,-1.0, 0.0,1.0,0.0};

	printf("Distance Model Test:");
	if (ContinueOrSkip())
	{
		// load up sources
		alGenSources(1, testSources);
		alSourcei(testSources[0], AL_BUFFER, g_Buffers[1]);
	
		printf("The source will be placed at (0, 0, -10). This should be low volume. (Press Return):\n");
		CRToContinue();
		alSource3f(testSources[0], AL_POSITION, 0.0, 0.0, -10.0);
		alDistanceModel(AL_INVERSE_DISTANCE);
		alSourcei(testSources[0], AL_LOOPING, false);
		alSourcePlay(testSources[0]);
		
		printf("The source will be placed at (0, 0, -1). This should be high volume. (Press Return):\n");
		CRToContinue();
		alSource3f(testSources[0], AL_POSITION, 0.0, 0.0, -1.0);
		alSourcePlay(testSources[0]);
		
		printf("The source will be placed at (0, 0, -10) and the distance model will be set to AL_NONE. This should be high volume. (Press Return):\n");
		CRToContinue();
		alSource3f(testSources[0], AL_POSITION, 0.0, 0.0, -10.0);
		alDistanceModel(AL_NONE);
		alSourcePlay(testSources[0]);
		
		printf("The source will be placed at (0, 0, -100) and the distance model will remain AL_NONE. This should be high volume. (Press Return):\n");
		CRToContinue();
		alSource3f(testSources[0], AL_POSITION, 0.0, 0.0, -100.0);
		alSourcePlay(testSources[0]);
		
		printf("The source will be placed at (0, 0, -100) and the distance model will be AL_INVERSE_DISTANCE_CLAMPED. AL_MAX_DISTANCE will be set to 100.0.  This should be low volume. (Press Return):\n");
		CRToContinue();
		alSource3f(testSources[0], AL_POSITION, 0.0, 0.0, -100.0);
		alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
		alSourcef(testSources[0], AL_MAX_DISTANCE, 100.0);
		alSourcePlay(testSources[0]);
		
		printf("The source will be placed at (0, 0, -100) and the distance model will be AL_INVERSE_DISTANCE_CLAMPED. AL_MAX_DISTANCE will be set to 20.0.  This should be louder. (Press Return):\n");
		CRToContinue();
		alSource3f(testSources[0], AL_POSITION, 0.0, 0.0, -100.0);
		alSourcef(testSources[0], AL_MAX_DISTANCE, 20.0);
		alSourcePlay(testSources[0]);
		
		printf("The source will be placed at (0, 0, -100) and the distance model will be AL_INVERSE_DISTANCE_CLAMPED. AL_MAX_DISTANCE will be set to 5.0.  This should be high volume. (Press Return):\n");
		CRToContinue();
		alSource3f(testSources[0], AL_POSITION, 0.0, 0.0, -100.0);
		alSourcef(testSources[0], AL_MAX_DISTANCE, 5.0);
		alSourcePlay(testSources[0]);
		
		CRForNextTest();
		alSource3f(testSources[0], AL_POSITION, 0.0, 0.0, 0.0);
		alDistanceModel(AL_INVERSE_DISTANCE);
		
		// dispose of sources
		alSourcei(testSources[0], AL_BUFFER, 0);
		alDeleteSources(1, testSources);
	} 
}


/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Doppler Test
This tests doppler shift capability.
*/
ALvoid SA_Doppler(ALvoid)
{	
	ALuint testSources[2];
	int i;
	ALfloat	listenerOri[]={0.0,0.0,-1.0, 0.0,1.0,0.0};
	ALfloat	sourceDir[]={1.0,0.0,0.0};

	printf("Doppler Test:");
	if (ContinueOrSkip())
	{
		// load up sources
		alGenSources(1, testSources);
		alSourcei(testSources[0], AL_BUFFER, g_Buffers[1]);
	
		printf("Trying Left-to-Right sweep with doppler shift (Press Return):\n");
		CRToContinue();
		alListenerfv(AL_ORIENTATION, listenerOri);
		alSourcei(testSources[0], AL_LOOPING, true);
		alSource3f(testSources[0], AL_POSITION, -100.0, 0.0, 0.0);
		alSource3f(testSources[0], AL_VELOCITY, 10.0, 0.0, 0.0);
		alSourcefv(testSources[0], AL_DIRECTION, sourceDir);
		alSourcePlay(testSources[0]);
		for (i = -100; i < 100; i++)
		{
			alSource3f(testSources[0], AL_POSITION, (float) i, 0.0, 0.0);		
			delay_ms(100);
		}
		alSourceStop(testSources[0]);
		printf("Trying Left-to-Right sweep with DopplerFactor set to 4.0 -- should be more extreme (Press Return):\n");
		CRToContinue();
		alSource3f(testSources[0], AL_POSITION, -100.0, 0.0, 0.0);
		alSource3f(testSources[0], AL_VELOCITY, 10.0, 0.0, 0.0);
		alDopplerFactor(4.0);
		if (alGetFloat(AL_DOPPLER_FACTOR) != 4.0) { printf(" alGetFloat(AL_DOPPLER_FACTOR) error.\n"); }
		alSourcePlay(testSources[0]);
		for (i = -100; i < 100; i++)
		{
			alSource3f(testSources[0], AL_POSITION, (float) i, 0.0, 0.0);		
			delay_ms(100);
		}
		alSourceStop(testSources[0]);
		alDopplerFactor(1.0);
		printf("Trying Left-to-Right sweep with DopplerVelocity set to 86 -- should remain extreme (Press Return):\n");
		CRToContinue();
		alSource3f(testSources[0], AL_POSITION, -100.0, 0.0, 0.0);
		alSource3f(testSources[0], AL_VELOCITY, 10.0, 0.0, 0.0);
		alDopplerVelocity(86);
		if (alGetFloat(AL_DOPPLER_VELOCITY) != 86) { printf(" alGetFloat(AL_DOPPLER_VELOCITY) error.\n"); }
		alSourcePlay(testSources[0]);
		for (i = -100; i < 100; i++)
		{
			alSource3f(testSources[0], AL_POSITION, (float) i, 0.0, 0.0);		
			delay_ms(100);
		}
		alDopplerVelocity(343);
		alSource3f(testSources[0], AL_POSITION, 0.0, 0.0, 0.0);
		alSourceStop(testSources[0]);
		CRForNextTest();
		
		// dispose of sources
		alSourcei(testSources[0], AL_BUFFER, 0);
		alDeleteSources(1, testSources);
	} 
}


/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Frequency Test
This test alters the frequency of a playing source.
*/
ALvoid SA_Frequency(ALvoid)
{	
	ALuint testSources[2];
	int i;
	ALfloat	listenerOri[]={0.0,0.0,-1.0, 0.0,1.0,0.0};

	printf("Frequency Test:");
	if (ContinueOrSkip())
	{
		// load up sources
		alGenSources(1, testSources);
		alSourcei(testSources[0], AL_BUFFER, g_Buffers[1]);
	
		printf("A source will be played eight times -- going from one-half to double it's native frequency (Press Return):\n");
		CRToContinue();
		alSourcei(testSources[0], AL_LOOPING, false);
		for (i = 0; i < 8; i++)
		{
			alSourcef(testSources[0], AL_PITCH, (float) (0.5 + (float) i * 0.2));
			alSourcePlay(testSources[0]);
			delay_ms(2000);
		}
		alSourceStop(testSources[0]);
		alSourcef(testSources[0], AL_PITCH, 1.0);
		CRForNextTest();
		
		// dispose of sources
		alSourcei(testSources[0], AL_BUFFER, 0);
		alDeleteSources(1, testSources);
	} 
}
	

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Stereo Test
This test plays a stereo buffer.
*/
ALvoid SA_Stereo(ALvoid)
{	
	ALuint testSources[2];
	ALuint error;
	ALfloat	listenerOri[]={0.0,0.0,-1.0, 0.0,1.0,0.0};

	printf("Stereo Test:");
	if (ContinueOrSkip())
	{
		// clear error state
		alGetError();
	
		// load up sources
		alGenSources(1, testSources);
		alSourcei(testSources[0], AL_BUFFER, g_Buffers[1]);
	
		printf("A stereo buffer will play twice in succession (Press Return):\n");
		CRToContinue();
		alSourcei(testSources[0], AL_BUFFER, 0);
		alSourcei(testSources[0], AL_LOOPING, AL_FALSE);
		if ((error = alGetError()) != AL_NO_ERROR)
			DisplayALError((ALbyte *) "Init error : ", error);
		alSourceQueueBuffers(testSources[0], 1, &g_Buffers[6]);
		if ((error = alGetError()) != AL_NO_ERROR)
			DisplayALError((ALbyte *) "alSourceQueueBuffers 1 (stereo) : ", error);
		alSourceQueueBuffers(testSources[0], 1, &g_Buffers[6]);
		if ((error = alGetError()) != AL_NO_ERROR)
			DisplayALError((ALbyte *) "alSourceQueueBuffers 1 (stereo) : ", error);
		alSourcePlay(testSources[0]);
		CRForNextTest();
		
		// dispose of sources
		alSourcei(testSources[0], AL_BUFFER, 0);
		alDeleteSources(1, testSources);
	}
}
		

/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Streaming Test
This test streams audio from a file.
*/
ALvoid SA_Streaming(ALvoid)
{
	printf("Streaming Test:");
	if (ContinueOrSkip())
	{
		printf("A stereo audio file will now be streamed from a file (Press Return):\n");
		CRToContinue();
		I_StreamingTest();
		CRForNextTest();
	}
}


/** used by gendocs.py
$SECTION Semi Automatic Tests
$SUBTITLE Queuing Underrun Performance
This test checks the performance of OpenAL during a buffer underrun.
*/
ALvoid SA_QueuingUnderrunPerformance(ALvoid)
{	
	ALuint testSources[1];
	ALuint bufferName;
	ALuint error;
	ALint tempInt;
	ALfloat	listenerOri[]={0.0,0.0,-1.0, 0.0,1.0,0.0};

	printf("Queuing Underrun Performance Test:");
	if (ContinueOrSkip())
	{
		printf("A stereo buffer will play once, there will be a brief pause, and then the buffer will play again (Press Return):\n");
		CRToContinue();
		alGetError();
		alGenSources(1, testSources);
		alSourcei(testSources[0], AL_BUFFER, 0);
		alSourcei(testSources[0], AL_LOOPING, AL_FALSE);
		if ((error = alGetError()) != AL_NO_ERROR)
			DisplayALError((ALbyte *) "Init error : ", error);
		alSourceQueueBuffers(testSources[0], 1, &g_Buffers[6]);
		if ((error = alGetError()) != AL_NO_ERROR)
			DisplayALError((ALbyte *) "alSourceQueueBuffers 1 (stereo) : ", error);
		alSourcePlay(testSources[0]);
		delay_ms(4000);
		alGetSourcei(testSources[0], AL_SOURCE_STATE, &tempInt);
		if (tempInt != AL_STOPPED)
		    printf("Wrong underrun state -- should be AL_STOPPED. ");
		alGetSourcei(testSources[0], AL_BUFFERS_PROCESSED, &tempInt);
		if (tempInt != 1)
		{
		    printf("Wrong underrun state -- should have one buffer processed. ");
		} else
		{
			alSourceUnqueueBuffers(testSources[0], tempInt, &bufferName);
		}
		alSourceQueueBuffers(testSources[0], 1, &g_Buffers[6]);
		if ((error = alGetError()) != AL_NO_ERROR)
			DisplayALError((ALbyte *) "alSourceQueueBuffers 1 (stereo) : ", error);
		alSourcePlay(testSources[0]);
		delay_ms(3000);
		printf("The stereo buffer will now play twice with no pause (Press Return):\n");
		CRToContinue();
		alSourceQueueBuffers(testSources[0], 1, &g_Buffers[6]);
		alSourcePlay(testSources[0]);
		delay_ms(4000);
		CRForNextTest();
		
		// dispose of sources
		alSourcei(testSources[0], AL_BUFFER, 0);
		alDeleteSources(1, testSources);
	}
}


// Buffer Test
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Buffer Test
This test allows the user to dynamically attach and unattach different buffers
to a single source.
*/
ALvoid I_BufferTest(ALvoid)
{
	ALuint	source[1];
	ALint	error;
	char ch;

	ALfloat source0Pos[]={ 2.0, 0.0,-2.0};	// Front and right of the listener
	ALfloat source0Vel[]={ 0.0, 0.0, 0.0};
	
	alGenSources(1,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alGenSources 2 : ", error);
		return;
	}

	alSourcef(source[0],AL_PITCH,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcef 0 AL_PITCH : \n", error);

	alSourcef(source[0],AL_GAIN,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcef 0 AL_GAIN : \n", error);
	
	alSourcefv(source[0],AL_POSITION,source0Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcefv 0 AL_POSITION : \n", error);
	
	alSourcefv(source[0],AL_VELOCITY,source0Vel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcefv 0 AL_VELOCITY : \n", error);

	alSourcei(source[0],AL_LOOPING,AL_FALSE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcei 0 AL_LOOPING true: \n", error);

	printf("Buffer Test\n");
	printf("Press '1' to play buffer 0 on source 0\n");
	printf("Press '2' to play buffer 1 on source 0\n");
	printf("Press '3' to stop source 0\n");
	printf("Press 'q' to quit\n");

	do
	{
		ch = getUpperCh();
 		
		switch (ch)
		{
			case '1':
				// Stop source
				alSourceStop(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourceStop 0 : ", error);
				// Attach buffer 0 to source
				alSourcei(source[0], AL_BUFFER, g_Buffers[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcei AL_BUFFER 0 : ", error);
				// Play
				alSourcePlay(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcePlay 0 : ", error);
				break;
			case '2':
				// Stop source
				alSourceStop(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourceStop 0 : ", error);
				// Attach buffer 0 to source
				alSourcei(source[0], AL_BUFFER, g_Buffers[1]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcei AL_BUFFER 1 : ", error);
				// Play
				alSourcePlay(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcePlay 0 : ", error);
				break;
			case '3':
				// Stop source
				alSourceStop(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourceStop 0 : ", error);
				break;
		}
	} while (ch != 'Q');

	// Release resources
	alSourceStopv(1, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourceStopv 1 : ", error);

	alDeleteSources(1, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alDeleteSources 1 : ", error);

	return;
}


// Position Test
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Position Test
This test creates 2 Sources - one to the front right of the listener, and one to
the rear left of the listener
*/
ALvoid I_PositionTest(ALvoid)
{	
	ALint	error;
	
	ALuint	source[2];
	ALbyte	ch;

	ALfloat source0Pos[]={ -2.0, 0.0, 2.0};	// Behind and to the left of the listener
	ALfloat source0Vel[]={ 0.0, 0.0, 0.0};

	ALfloat source1Pos[]={ 2.0, 0.0,-2.0};	// Front and right of the listener
	ALfloat source1Vel[]={ 0.0, 0.0, 0.0};

	alGenSources(2,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alGenSources 2 : ", error);
		return;
	}
	
	alSourcef(source[0],AL_PITCH,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcef 0 AL_PITCH : \n", error);

	alSourcef(source[0],AL_GAIN,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcef 0 AL_GAIN : \n", error);
	
	alSourcefv(source[0],AL_POSITION,source0Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcefv 0 AL_POSITION : \n", error);
	
	alSourcefv(source[0],AL_VELOCITY,source0Vel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcefv 0 AL_VELOCITY : \n", error);

	alSourcei(source[0],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcei 0 AL_BUFFER buffer 0 : \n", error);

	alSourcei(source[0],AL_LOOPING,AL_TRUE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcei 0 AL_LOOPING true: \n", error);


	alSourcef(source[1],AL_PITCH,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcef 1 AL_PITCH : \n", error);

	alSourcef(source[1],AL_GAIN,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcef 1 AL_GAIN : \n", error);

	alSourcefv(source[1],AL_POSITION,source1Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcefv 1 AL_POSITION : \n", error);

	alSourcefv(source[1],AL_VELOCITY,source1Vel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcefv 1 AL_VELOCITY : \n", error);

	alSourcei(source[1],AL_BUFFER, g_Buffers[1]); 
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcei 1 AL_BUFFER buffer 1 : \n", error);

	alSourcei(source[1],AL_LOOPING,AL_FALSE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcei 1 AL_LOOPING false: \n", error);

	printf("Position Test\n");
	printf("Press '1' to play source 0 (looping) rear left of listener\n");
	printf("Press '2' to play source 1 once (single shot) front right of listener\n");
	printf("Press '3' to stop source 0\n");
	printf("Press '4' to stop source 1\n");
	printf("Press 'q' to quit\n");

	do
	{
		ch = getUpperCh();
 		
		switch (ch)
		{
			case '1':
				alSourcePlay(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcePlay source 0 : ", error);
				break;
			case '2':
				alSourcePlay(source[1]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcePlay source 1 : ", error);
				break;
			case '3':
				alSourceStop(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourceStop source 0 : ", error);
				break;
			case '4':
				alSourceStop(source[1]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourceStop source 1 : ", error);
				break;
		}
	} while (ch != 'Q');

	// Release resources
	alSourceStopv(2, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourceStopv 2 : ", error);

	alDeleteSources(2, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alDeleteSources 2 : ", error);

	return;
}


// Looping Test
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Looping Test
This test checks the ability to switch Looping ON and OFF for a particular source, either before
or during Playback.  (If looping is switched off during playback, the buffer should finish playing
until the end of the sample.)
*/
ALvoid I_LoopingTest(ALvoid)
{
	ALint	error;
	ALuint	source[2];
	ALbyte	ch;
	ALboolean bLooping0 = AL_FALSE;
	ALboolean bLooping1 = AL_FALSE;

	ALfloat source0Pos[]={ -2.0, 0.0, -2.0};	// Front left of the listener
	ALfloat source0Vel[]={ 0.0, 0.0, 0.0};

	ALfloat source1Pos[]={ 2.0, 0,0, -2.0};		// Front right of the listener
	ALfloat source1Vel[]={ 0.0, 0.0, 0,0};

	// Clear Error Code
	alGetError();

	alGenSources(2,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alGenSources 1 : ", error);
		return;
	}
	
	alSourcef(source[0],AL_PITCH,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcef 0 AL_PITCH : \n", error);

	alSourcef(source[0],AL_GAIN,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcef 0 AL_GAIN : \n", error);
	
	alSourcefv(source[0],AL_POSITION,source0Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcefv 0 AL_POSITION : \n", error);
	
	alSourcefv(source[0],AL_VELOCITY,source0Vel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcefv 0 AL_VELOCITY : \n", error);

	alSourcei(source[0],AL_BUFFER, g_Buffers[0]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcei 0 AL_BUFFER buffer 0 : \n", error);

	alSourcei(source[0],AL_LOOPING,AL_FALSE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcei 0 AL_LOOPING false : \n", error);


	alSourcef(source[1],AL_PITCH,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcef 1 AL_PITCH : \n", error);

	alSourcef(source[1],AL_GAIN,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcef 1 AL_GAIN : \n", error);

	alSourcefv(source[1],AL_POSITION,source1Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcefv 1 AL_POSITION : \n", error);

	alSourcefv(source[1],AL_VELOCITY,source1Vel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcefv 1 AL_VELOCITY : \n", error);

	alSourcei(source[1],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcei 1 AL_BUFFER buffer 1 : \n", error);

	alSourcei(source[1],AL_LOOPING,AL_FALSE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcei 1 AL_LOOPING false: \n", error);

	printf("Looping Test\n");
	printf("Press '1' to play source 0 once (single shot)\n");
	printf("Press '2' to toggle looping on source 0\n");
	printf("Press '3' to play source 1 once (single shot)\n");
	printf("Press '4' to toggle looping on source 1\n");
	printf("Press 'q' to quit\n");
	printf("\nSource 0 : Not looping Source 1 : Not looping\n");
	do
	{
		ch = getUpperCh();
 		
		switch (ch)
		{
			case '1':
				alSourcePlay(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcePlay source 0 : ", error);
				break;
			case '2':
				if (bLooping0 == AL_FALSE)
				{
					bLooping0 = AL_TRUE;
					if (bLooping1)
						printf("Source 0 : Looping     Source 1 : Looping    \n");
					else
						printf("Source 0 : Looping     Source 1 : Not looping\n");
				}
				else
				{
					bLooping0 = AL_FALSE;
					if (bLooping1)
						printf("Source 0 : Not looping Source 1 : Looping    \n");
					else
						printf("Source 0 : Not looping Source 1 : Not looping\n");
				}
				alSourcei(source[0], AL_LOOPING, bLooping0);
				break;
			case '3':
				alSourcePlay(source[1]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcePlay source 1 : ", error);
				break;
			case '4':
				if (bLooping1 == AL_FALSE)
				{
					bLooping1 = AL_TRUE;
					if (bLooping0)
						printf("Source 0 : Looping     Source 1 : Looping    \n");
					else
						printf("Source 0 : Not looping Source 1 : Looping    \n");
				}
				else
				{
					bLooping1 = AL_FALSE;
					if (bLooping0)
						printf("Source 0 : Looping     Source 1 : Not looping\n");
					else
						printf("Source 0 : Not looping Source 1 : Not looping\n");
				}
				alSourcei(source[1], AL_LOOPING, bLooping1);
				break;
		}
	} while (ch != 'Q');

	printf("\n");

	// Release resources
	alSourceStop(source[0]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourceStop source 1 : ", error);

	alDeleteSources(2, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alDeleteSources 1 : ", error);

	return;
}

#ifdef TEST_EAX
// EAX Test
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE EAX Test
This test Uses 2 Sources to test out EAX 2.0 Reverb, Occlusion and Obstruction.  Also tests the use
of the DEFERRED flag in EAX.
*/
ALvoid I_EAXTest(ALvoid)
{
        ALint	error;
	ALuint	source[2];
	ALbyte	ch;
	ALuint	Env;
	ALint	Room;
	ALint	Occlusion;
	ALint	Obstruction;
	EAXBUFFERPROPERTIES eaxBufferProp0;

	ALfloat source0Pos[]={ -2.0, 0.0, 2.0};	// Behind and to the left of the listener
	ALfloat source0Vel[]={ 0.0, 0.0, 0.0};

	ALfloat source1Pos[]={ 2.0, 0.0,-2.0};	// Front and right of the listener
	ALfloat source1Vel[]={ 0.0, 0.0, 0.0};

	// Clear Error Code
	alGetError();

	alGenSources(2,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alGenSources 2 : ", error);
		return;
	}
	
	alSourcef(source[0],AL_PITCH,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcef 0 AL_PITCH : \n", error);

	alSourcef(source[0],AL_GAIN,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcef 0 AL_GAIN : \n", error);
	
	alSourcefv(source[0],AL_POSITION,source0Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcefv 0 AL_POSITION : \n", error);
	
	alSourcefv(source[0],AL_VELOCITY,source0Vel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcefv 0 AL_VELOCITY : \n", error);

	alSourcei(source[0],AL_BUFFER, g_Buffers[0]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcei 0 AL_BUFFER buffer 0 : \n", error);

	alSourcei(source[0],AL_LOOPING,AL_TRUE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcei 0 AL_LOOPING true: \n", error);


	alSourcef(source[1],AL_PITCH,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcef 1 AL_PITCH : \n", error);

	alSourcef(source[1],AL_GAIN,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcef 1 AL_GAIN : \n", error);

	alSourcefv(source[1],AL_POSITION,source1Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcefv 1 AL_POSITION : \n", error);

	alSourcefv(source[1],AL_VELOCITY,source1Vel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcefv 1 AL_VELOCITY : \n", error);

	alSourcei(source[1],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcei 1 AL_BUFFER buffer 1 : \n", error);

	alSourcei(source[1],AL_LOOPING,AL_FALSE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcei 1 AL_LOOPING false: \n", error);

	printf("EAX Test\n\n");
	printf("Press '1' to play source 0 (looping)\n");
	printf("Press '2' to play source 1 once (single shot)\n");
	printf("Press '3' to stop source 0\n");
	printf("Press '4' to stop source 1\n");
	printf("Press '5' to add Hangar reverb (DEFERRED)\n");
	printf("Press '6' to remove reverb (DEFERRED)\n");
	printf("Press '7' to occlude source 0 (DEFERRED)\n");
	printf("Press '8' to remove occlusion from source 0 (DEFERRED)\n");
	printf("Press '9' to obstruct source 1 (IMMEDIATE)\n");
	printf("Press '0' to remove obstruction from source 1 (IMMEDIATE)\n");
	printf("Press 'c' to COMMIT EAX settings\n");
	printf("Press 'q' to quit\n\n");

	do
	{
		ch = getUpperCh();
 		
		switch (ch)
		{
			case '1':
				alSourcePlay(source[0]);
				break;
			case '2':
				alSourcePlay(source[1]);
				break;
			case '3':
				alSourceStop(source[0]);
				break;
			case '4':
				alSourceStop(source[1]);
				break;
			case '5':
				Env = EAX_ENVIRONMENT_HANGAR;
				eaxSet(&DSPROPSETID_EAX20_ListenerProperties, DSPROPERTY_EAXLISTENER_ENVIRONMENT | DSPROPERTY_EAXLISTENER_DEFERRED,
					NULL, &Env, sizeof(ALuint));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "eaxSet EAXLISTENER_ENVIRONMENT | EAXLISTENER_DEFERRED : \n", error);
				break;

			case '6':
				Room = -10000;
				eaxSet(&DSPROPSETID_EAX20_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOM | DSPROPERTY_EAXLISTENER_DEFERRED, NULL,
					&Room, sizeof(ALint));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "eaxSet EAXLISTENER_ROOM | EAXLISTENER_DEFERRED : \n", error);
				break;

			case '7':
				eaxGet(&DSPROPSETID_EAX20_BufferProperties, DSPROPERTY_EAXBUFFER_ALLPARAMETERS, source[0],
					&eaxBufferProp0, sizeof(EAXBUFFERPROPERTIES));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "eaxGet EAXBUFFER_ALLPARAMETERS : \n", error);
				eaxBufferProp0.lOcclusion = -5000;
				eaxSet(&DSPROPSETID_EAX20_BufferProperties, DSPROPERTY_EAXBUFFER_ALLPARAMETERS | DSPROPERTY_EAXBUFFER_DEFERRED, source[0],
					&eaxBufferProp0, sizeof(EAXBUFFERPROPERTIES));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "eaxSet EAXBUFFER_ALLPARAMETERS | EAXBUFFER_DEFERRED : \n", error);
				break;

			case '8':
				Occlusion = 0;
				eaxSet(&DSPROPSETID_EAX20_BufferProperties, DSPROPERTY_EAXBUFFER_OCCLUSION | DSPROPERTY_EAXBUFFER_DEFERRED, source[0],
					&Occlusion, sizeof(ALint));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "eaxSet EAXBUFFER_OCCLUSION | EAXBUFFER_DEFERRED : \n", error);
				break;

			case '9':
				Obstruction = -5000;
				eaxSet(&DSPROPSETID_EAX20_BufferProperties, DSPROPERTY_EAXBUFFER_OBSTRUCTION, source[1],
					&Obstruction, sizeof(ALint));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "eaxSet EAXBUFFER_OBSTRUCTION : \n", error);
				break;

			case '0':
				Obstruction = 0;
				eaxSet(&DSPROPSETID_EAX20_BufferProperties, DSPROPERTY_EAXBUFFER_OBSTRUCTION, source[1],
					&Obstruction, sizeof(ALint));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "eaxSet EAXBUFFER_OBSTRUCTION : \n", error);
				break;

			case 'C':
				// Commit settings on source 0
				eaxSet(&DSPROPSETID_EAX20_BufferProperties, DSPROPERTY_EAXBUFFER_COMMITDEFERREDSETTINGS,
					source[0], NULL, 0);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "eaxSet EAXBUFFER_COMMITDEFERREDSETTINGS : \n", error);

				// Commit Listener settings
				eaxSet(&DSPROPSETID_EAX20_ListenerProperties, DSPROPERTY_EAXLISTENER_COMMITDEFERREDSETTINGS,
					NULL, NULL, 0);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "eaxSet EAXLISTENER_COMMITDEFERREDSETTINGSENVIRONMENT : \n", error);
				break;
		}
	} while (ch != 'Q');

	// reset EAX level
	Room = -10000;
	eaxSet(&DSPROPSETID_EAX20_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOM, NULL, &Room, sizeof(ALint));
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "eaxSet EAXLISTENER_ROOM : \n", error);

	// Release resources
	alSourceStopv(2, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourceStopv 2 : ", error);

	alDeleteSources(2, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alDeleteSources 2 : ", error);
	return;
}
#endif


// Queue Test
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Queue Test
This test checks the ability to queue and unqueue a sequence of buffers on a Source. (Buffers
can only be	unqueued if they have been PROCESSED by a Source.)
*/
ALvoid I_QueueTest(ALvoid)
{
	ALint	error;
	ALuint	source[1];
	ALbyte	ch;
	ALuint  buffers[5];
	ALuint  *buffersremoved;
	ALboolean bLooping;
	ALint	BuffersInQueue, BuffersProcessed;
	ALfloat source0Pos[]={ 0.0, 0.0, -2.0};	// Immediately in front of listener
	ALfloat source0Vel[]={ 0.0, 0.0, 0.0};
	ALuint	Buffer;
	ALint	i;

	// Clear Error Code
	alGetError();

	alGenSources(1,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenSources 1 : ", error);
		return;
	}
	
	alSourcef(source[0],AL_PITCH,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 0 AL_PITCH : ", error);

	alSourcef(source[0],AL_GAIN,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 0 AL_GAIN : ", error);
	
	alSourcefv(source[0],AL_POSITION,source0Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_POSITION : ", error);
	
	alSourcefv(source[0],AL_VELOCITY,source0Vel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_VELOCITY : ", error);

	alSourcei(source[0],AL_LOOPING,AL_FALSE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_LOOPING false: ", error);

	bLooping = false;

	printf("Queue Test\n\n");
	printf("Press '1' to start playing source 0\n");
	printf("Press '2' to stop source 0\n");
	printf("Press '3' to toggle looping on source 0\n");
	printf("Press '4' to queue 4 buffers on source 0\n");
	printf("Press '5' to queue 1st buffer on source 0\n");
	printf("Press '6' to queue 2nd buffer on source 0\n");
	printf("Press '7' to queue 3rd buffer on source 0\n");
	printf("Press '8' to queue 4th buffer on source 0\n");
	printf("Press '9' to queue 5th buffer (Buffer 0) on source 0\n");
	printf("Press '0' to display stats\n");

	printf("Press 'a' to unqueue first Buffer\n");
	printf("Press 'b' to unqueue first 2 Buffers\n");
	printf("Press 'c' to unqueue first 3 Buffers\n");
	printf("Press 'd' to unqueue first 4 Buffers\n");
	printf("Press 'e' to unqueue first 5 Buffers\n");
	printf("Press 'f' to unqueue all buffers\n");

	printf("Press 'q' to quit\n");

	printf("Source 0 not looping\n");

	buffers[0] = g_Buffers[2];
	buffers[1] = g_Buffers[3];
	buffers[2] = g_Buffers[4];
	buffers[3] = g_Buffers[5];
	buffers[4] = 0;
	
	do
	{
		ch = getUpperCh();
		switch (ch)
		{
			case '1':
				alSourcePlay(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcePlay source 0 : ", error);
				break;
			case '2':
				alSourceStop(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceStop source 0 : ", error);
				break;
			case '3':
				if (bLooping == AL_TRUE)
				{
					bLooping = AL_FALSE;
					printf("Source 0 not looping\n");
				}
				else
				{
					bLooping = AL_TRUE;
					printf("Source 0 looping    \n");
				}
				alSourcei(source[0], AL_LOOPING, bLooping);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcei AL_LOOPING : ", error);
				break;
			case '4':
				alSourceQueueBuffers(source[0], 4, buffers);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceQueueBuffers 4 : ", error);
				break;
			case '5':
				alSourceQueueBuffers(source[0], 1, &buffers[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceQueueBuffers 1 : ", error);
				break;
			case '6':
				alSourceQueueBuffers(source[0], 1, &buffers[1]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceQueueBuffers 1 : ", error);
				break;
			case '7':
				alSourceQueueBuffers(source[0], 1, &buffers[2]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceQueueBuffers 1 : ", error);
				break;
			case '8':
				alSourceQueueBuffers(source[0], 1, &buffers[3]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceQueueBuffers 1 : ", error);
				break;
			case '9':
				// Queue buffer 0
				alSourceQueueBuffers(source[0], 1, &buffers[4]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceQueueBuffers 1 (buffer 0) : ", error);
				break;
			case 'A':
				// Unqueue first Buffer
				buffersremoved = new ALuint[1];
				alSourceUnqueueBuffers(source[0], 1, buffersremoved);
				
				if ((error = alGetError()) != AL_NO_ERROR)
				{
					DisplayALError("alSourceUnqueueBuffers 1 : ", error);
				} else
				{
					if (buffersremoved[0] == buffers[0])
						buffersremoved[0] = 1;
					else if (buffersremoved[0] == buffers[1])
						buffersremoved[0] = 2;
					else if (buffersremoved[0] == buffers[2])
						buffersremoved[0] = 3;
					else if (buffersremoved[0] == buffers[3])
						buffersremoved[0] = 4;
					else
						buffersremoved[0] = 0;
				
					printf("\nRemoved Buffer %d from queue\n", buffersremoved[0]);
				}
				
				delete buffersremoved;
				break;
			case 'B':
				// Unqueue first 2 Buffers
				buffersremoved = new ALuint[2];
				alSourceUnqueueBuffers(source[0], 2, buffersremoved);
				
				if ((error = alGetError()) != AL_NO_ERROR)
				{
					DisplayALError("alSourceUnqueueBuffers 2 : ", error);
				} else
				{
					for (i = 0; i < 2; i++)
					{
						if (buffersremoved[i] == buffers[0])
							buffersremoved[i] = 1;
						else if (buffersremoved[i] == buffers[1])
							buffersremoved[i] = 2;
						else if (buffersremoved[i] == buffers[2])
							buffersremoved[i] = 3;
						else if (buffersremoved[i] == buffers[3])
							buffersremoved[i] = 4;
						else
							buffersremoved[i] = 0;
					}

					printf("\nRemoved Buffers %d and %d from queue\n", buffersremoved[0], buffersremoved[1]);
				}

				delete buffersremoved;
				break;
			case 'C':
				// Unqueue first 3 Buffers
				buffersremoved = new ALuint[3];
				alSourceUnqueueBuffers(source[0], 3, buffersremoved);
				if ((error = alGetError()) != AL_NO_ERROR)
				{
					DisplayALError("alSourceUnqueueBuffers 3 : ", error);
				} else
				{
					for (i = 0; i < 3; i++)
					{
						if (buffersremoved[i] == buffers[0])
							buffersremoved[i] = 1;
						else if (buffersremoved[i] == buffers[1])
							buffersremoved[i] = 2;
						else if (buffersremoved[i] == buffers[2])
							buffersremoved[i] = 3;
						else if (buffersremoved[i] == buffers[3])
							buffersremoved[i] = 4;
						else
							buffersremoved[i] = 0;
					}

					printf("\nRemoved Buffers %d, %d and %d from queue\n", buffersremoved[0], buffersremoved[1],
						buffersremoved[2]);
				}

				delete buffersremoved;
				break;
			case 'D':
				// Unqueue first 4 Buffers
				buffersremoved = new ALuint[4];
				alSourceUnqueueBuffers(source[0], 4, buffersremoved);
				
				if ((error = alGetError()) != AL_NO_ERROR)
				{
					DisplayALError("alSourceUnqueueBuffers 1 : ", error);
				} else
				{
					for (i = 0; i < 4; i++)
					{
						if (buffersremoved[i] == buffers[0])
							buffersremoved[i] = 1;
						else if (buffersremoved[i] == buffers[1])
							buffersremoved[i] = 2;
						else if (buffersremoved[i] == buffers[2])
							buffersremoved[i] = 3;
						else if (buffersremoved[i] == buffers[3])
							buffersremoved[i] = 4;
						else
							buffersremoved[i] = 0;
					}

					printf("\nRemoved Buffers %d, %d, %d and %d from queue\n", buffersremoved[0], buffersremoved[1],
						buffersremoved[2], buffersremoved[3]);
				}

				delete buffersremoved;
				break;
			case 'E':
				// Unqueue first 5 Buffers
				buffersremoved = new ALuint[5];
				alSourceUnqueueBuffers(source[0], 5, buffersremoved);
				
				if ((error = alGetError()) != AL_NO_ERROR)
				{
					DisplayALError("alSourceUnqueueBuffers 1 : ", error);
				} else
				{
					for (i = 0; i < 5; i++)
					{
						if (buffersremoved[i] == buffers[0])
							buffersremoved[i] = 1;
						else if (buffersremoved[i] == buffers[1])
							buffersremoved[i] = 2;
						else if (buffersremoved[i] == buffers[2])
							buffersremoved[i] = 3;
						else if (buffersremoved[i] == buffers[3])
							buffersremoved[i] = 4;
						else
							buffersremoved[i] = 0;
					}

					printf("\nRemoved Buffers %d, %d, %d, %d and %d from queue\n", buffersremoved[0], buffersremoved[1],
						buffersremoved[2], buffersremoved[3], buffersremoved[4]);
				}

				delete buffersremoved;
				break;
			case 'F':
				alSourcei(source[0], AL_BUFFER, 0);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSource AL_BUFFER NULL : ", error);
				break;
			case '0':
				// Retrieve number of buffers in queue
#ifndef LINUX
		        alGetSourcei(source[0], AL_BUFFERS_QUEUED, &BuffersInQueue);
#else
		        alGetSourceiv(source[0], AL_BUFFERS_QUEUED, &BuffersInQueue);
#endif
				// Retrieve number of processed buffers
#ifndef LINUX
		        alGetSourcei(source[0], AL_BUFFERS_PROCESSED, &BuffersProcessed);
#else
		       alGetSourceiv(source[0], AL_BUFFERS_PROCESSED, &BuffersProcessed);
#endif
				// Retrieve current buffer
#ifndef LINUX
		        alGetSourcei(source[0], AL_BUFFER, (ALint*)&Buffer);
#else
		       alGetSourceiv(source[0], AL_BUFFER, (ALint*)&Buffer);
#endif
				if (Buffer == buffers[0])
					Buffer = 1;
				else if (Buffer == buffers[1])
					Buffer = 2;
				else if (Buffer == buffers[2])
					Buffer = 3;
				else if (Buffer == buffers[3])
					Buffer = 4;
				else
					Buffer = 0;
				
				printf("Current Buffer is %d, %d Buffers in queue, %d Processed\n", Buffer, BuffersInQueue, BuffersProcessed);
				
				break;
		}
	} while (ch != 'Q');

	// Release resources
	alSourceStop(source[0]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourceStop : ", error);

	alDeleteSources(1, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alDeleteSources 1 : ", error);

	return;
}

// Frequency Test
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Frequency Test
This test exercises AL_PITCH functionality
*/
ALvoid I_FreqTest(ALvoid)
{
	ALint	error;
	ALuint	source[1];
	ALbyte	ch;
	ALfloat source0Pos[]={ 2.0, 0.0,-2.0};	// Front and right of the listener
	ALfloat source0Vel[]={ 0.0, 0.0, 0.0};


	alGenSources(1,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alGenSources 1 : ", error);
		return;
	}
	
	alSourcef(source[0],AL_PITCH,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcef 0 AL_PITCH : \n", error);

	alSourcef(source[0],AL_GAIN,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcef 0 AL_GAIN : \n", error);
	
	alSourcefv(source[0],AL_POSITION,source0Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcefv 0 AL_POSITION : \n", error);
	
	alSourcefv(source[0],AL_VELOCITY,source0Vel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcefv 0 AL_VELOCITY : \n", error);

	alSourcei(source[0],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcei 0 AL_BUFFER buffer 1 : \n", error);

	alSourcei(source[0],AL_LOOPING,AL_TRUE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcei 0 AL_LOOPING true: \n", error);

	printf("Frequency Test\n");
	printf("Press '1' to play source 0 (looping)\n");
	printf("Press '2' to Double Frequency\n");
	printf("Press '3' to Halve Frequency\n");
	printf("Press 'q' to quit\n");

	do
	{
		ch = getUpperCh();
 		
		switch (ch)
		{
			case '1':
				alSourcef(source[0], AL_PITCH, 1.0f);
				alSourcePlay(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcePlay source 0 : ", error);
				break;
			case '2':
				alSourcef(source[0], AL_PITCH, 2.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcef source 0 AL_PITCH 2.0 : ", error);
				break;
			case '3':
				alSourcef(source[0], AL_PITCH, 0.5f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcef source 0 AL PITCH 0.5: ", error);
				break;
		}
	} while (ch != 'Q');

	// Release resources
	alSourceStopv(1, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourceStopv 2 : ", error);

	alDeleteSources(1, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alDeleteSources 2 : ", error);
	
	return;
}


// Stereo Test
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Stereo Test
This test plays a stereo buffer.
*/
ALvoid I_StereoTest(ALvoid)
{
	ALint	error;
	ALuint	source[1];
	ALuint  buffers[2];
	ALuint	Buffer;
	ALint	BuffersInQueue, BuffersProcessed;
	ALbyte	ch;
	ALboolean bLoop = true;
	ALfloat source0Pos[]={ 2.0, 0.0,-2.0};	// Front and right of the listener
	ALfloat source0Vel[]={ 0.0, 0.0, 0.0};

	alGenSources(1,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alGenSources 1 : ", error);
		return;
	}

	buffers[0] = g_Buffers[6];
	buffers[1] = g_Buffers[6];

	printf("Stereo Test\n");
	printf("Press '1' to play a stereo buffer on source 0 (looping)\n");
	printf("Press '2' to play a mono buffer on source 0 (looping)\n");
	printf("Press '3' to stop source 0\n");
	printf("Press '4' to queue 2 stereo buffers on source 0 and start playing\n");
	printf("Press '5' to unqueue the 2 stereo buffers on source 0\n");
	printf("Press '6' to toggle looping on / off\n");
	printf("Press '0' to display stats\n");
	printf("Press 'q' to quit\n");
	printf("Looping is on\n");

	do
	{
		ch = getUpperCh();
 		
		switch (ch)
		{
			case '1':
				// Stop source
				alSourceStop(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourceStop source 0 : ", error);

				// Attach new buffer
				alSourcei(source[0],AL_BUFFER, g_Buffers[6]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcei 0 AL_BUFFER buffer 6 (stereo) : \n", error);

				// Set volume
				alSourcef(source[0],AL_GAIN,0.5f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcef 0 AL_GAIN : \n", error);

				// Set looping
				alSourcei(source[0],AL_LOOPING,bLoop);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcei 0 AL_LOOPING true: \n", error);

				// Play source
				alSourcePlay(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcePlay source 0 : ", error);
				
				break;
			case '2':
				// Stop source
				alSourceStop(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourceStop source 0 : ", error);

				// Attach new buffer
				alSourcei(source[0],AL_BUFFER, g_Buffers[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcei 0 AL_BUFFER buffer 0 (mono) : \n", error);

				// Set 3D position
				alSourcefv(source[0],AL_POSITION,source0Pos);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcefv 0 AL_POSITION : \n", error);
	
				// Set 3D velocity
				alSourcefv(source[0],AL_VELOCITY,source0Vel);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcefv 0 AL_VELOCITY : \n", error);

				// Set volume to full
				alSourcef(source[0],AL_GAIN,1.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcef 0 AL_GAIN : \n", error);

				// Set Looping
				alSourcei(source[0],AL_LOOPING,bLoop);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcei 0 AL_LOOPING : \n", error);

				// Play source
				alSourcePlay(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcePlay source 0 : ", error);
				break;
			case '3':
				alSourceStop(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourceStop source 0 : ", error);
				break;
			case '4':
				alSourceStop(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourceStop Source 0 : ", error);

				// Attach NULL buffer to source to clear everything
				alSourcei(source[0], AL_BUFFER, 0);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcei AL_BUFFER (NULL) : ", error);

				alSourceQueueBuffers(source[0], 2, buffers);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourceQueueBuffers 2 (stereo) : ", error);

				// Set Looping
				alSourcei(source[0],AL_LOOPING,bLoop);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcei 0 AL_LOOPING : \n", error);

				alSourcePlay(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcePlay Source 0 : ", error);
				break;
			case '5':
				alSourceUnqueueBuffers(source[0], 2, buffers);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourceUnqueueBuffers 2 (stereo) : ", error);
				break;
			case '6':
				if (bLoop)
				{
					printf("Looping is off\n");
					bLoop = AL_FALSE;
				}
				else
				{
					printf("Looping is on  \n");
					bLoop = AL_TRUE;
				}
				alSourcei(source[0], AL_LOOPING, bLoop);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcei 0 AL_LOOPING : \n", error);
				break;
			case '0':
#ifndef LINUX
		                // Retrieve number of buffers in queue
				alGetSourcei(source[0], AL_BUFFERS_QUEUED, &BuffersInQueue);
				// Retrieve number of processed buffers
				alGetSourcei(source[0], AL_BUFFERS_PROCESSED, &BuffersProcessed);
				// Retrieve current buffer
				alGetSourcei(source[0], AL_BUFFER, (ALint*)&Buffer);
#else
		                // Retrieve number of buffers in queue
				alGetSourceiv(source[0], AL_BUFFERS_QUEUED, &BuffersInQueue);
				// Retrieve number of processed buffers
				alGetSourceiv(source[0], AL_BUFFERS_PROCESSED, &BuffersProcessed);
				// Retrieve current buffer
				alGetSourceiv(source[0], AL_BUFFER, (ALint*)&Buffer);
		   
#endif
		                if (Buffer == buffers[0])
					Buffer = 6;
				else if (Buffer == buffers[1])
					Buffer = 6;
				else
					Buffer = 0;
				
				printf("Current Buffer is %d, %d Buffers in queue, %d Processed\n", Buffer, BuffersInQueue, BuffersProcessed);
				
				break;
		}
	} while (ch != 'Q');

	// Release resources
	alSourceStop(source[0]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourceStop : ", error);

	alDeleteSources(1, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alDeleteSources 2 : ", error);
	
	return;
}


// Gain Test
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Gain Test
This test plays two sources, allowing the control of source and listener gain.
*/
ALvoid I_GainTest(ALvoid)
{
	ALint	error;
	ALuint	source[2];
	ALbyte	ch;

	ALfloat source0Pos[]={ 2.0, 0.0,-2.0};	// Front and right of the listener
	ALfloat source0Vel[]={ 0.0, 0.0, 0.0};

	ALfloat source1Pos[]={-2.0, 0.0,-2.0};
	ALfloat source1Vel[]={ 0.0, 0.0, 0.0};

	alGenSources(2,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alGenSources 2 : ", error);
		return;
	}

	alSourcef(source[0],AL_PITCH,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcef 0 AL_PITCH : \n", error);

	alSourcef(source[0],AL_GAIN,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcef 0 AL_GAIN : \n", error);
	
	alSourcefv(source[0],AL_POSITION,source0Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcefv 0 AL_POSITION : \n", error);
	
	alSourcefv(source[0],AL_VELOCITY,source0Vel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcefv 0 AL_VELOCITY : \n", error);

	alSourcei(source[0],AL_BUFFER, g_Buffers[0]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcei 0 AL_BUFFER buffer 0 : \n", error);

	alSourcei(source[0],AL_LOOPING,AL_TRUE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcei 0 AL_LOOPING true: \n", error);


	alSourcef(source[1],AL_PITCH,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcef 1 AL_PITCH : \n", error);

	alSourcef(source[1],AL_GAIN,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcef 1 AL_GAIN : \n", error);

	alSourcefv(source[1],AL_POSITION,source1Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcefv 1 AL_POSITION : \n", error);

	alSourcefv(source[1],AL_VELOCITY,source1Vel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcefv 1 AL_VELOCITY : \n", error);

	alSourcei(source[1],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcei 1 AL_BUFFER buffer 1 : \n", error);

	alSourcei(source[1],AL_LOOPING,AL_TRUE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourcei 1 AL_LOOPING true: \n", error);

	printf("Gain Test\n");
	printf("Press '1' to play source 0 (looping)\n");
	printf("Press '2' to play source 1 (looping)\n");
	printf("Press '3' to stop source 0\n");
	printf("Press '4' to stop source 1\n");
	printf("Press '5' to set source 0 gain to 1.0\n");
	printf("Press '6' to set source 0 gain to 0.5\n");
	printf("Press '7' to set source 0 gain to 0.25\n");
	printf("Press '8' to set source 0 gain to 0\n");
	printf("Press 'a' to set Listener Gain to 1.0\n");
	printf("Press 'b' to set Listener Gain to 0.5\n");
	printf("Press 'c' to set Listener Gain to 0.25\n");
	printf("Press 'd' to set Listener Gain to 0.0\n");
	printf("Press 'q' to quit\n");

	do
	{
		ch = getUpperCh();
 		
		switch (ch)
		{
			case '1':
				alSourcePlay(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcePlay source 0 : ", error);
				break;
			case '2':
				alSourcePlay(source[1]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcePlay source 1 : ", error);
				break;
			case '3':
				alSourceStop(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourceStop source 0 : \n", error);
				break;
			case '4':
				alSourceStop(source[1]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourceStop source 1 : \n", error);
				break;
			case '5':
				alSourcef(source[0],AL_GAIN,1.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcef 0 AL_GAIN 1.0 : \n", error);
				break;
			case '6':
				alSourcef(source[0],AL_GAIN,0.5f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcef 0 AL_GAIN 0.5 : \n", error);
				break;
			case '7':
				alSourcef(source[0],AL_GAIN,0.25f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcef 0 AL_GAIN 0.25 : \n", error);
				break;
			case '8':
				alSourcef(source[0],AL_GAIN,0.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcef 0 AL_GAIN 0.0 : \n", error);
				break;
			case 'A':
				alListenerf(AL_GAIN,1.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alListenerf AL_GAIN 1.0 : \n", error);
				break;
			case 'B':
				alListenerf(AL_GAIN,0.5f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alListenerf AL_GAIN 0.5 : \n", error);
				break;
			case 'C':
				alListenerf(AL_GAIN,0.25f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alListenerf AL_GAIN 0.25 : \n", error);
				break;
			case 'D':
				alListenerf(AL_GAIN,0.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alListenerf AL_GAIN 0.0 : \n", error);
				break;
		}
	} while (ch != 'Q');

	// Reset & Release resources
	alListenerf(AL_GAIN,1.0f);
	alSourceStopv(2,source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourceStop : ", error);

	alDeleteSources(2, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alDeleteSources 2 : ", error);
	
	return;
}

#ifdef __MACOS__
void SwapWords(unsigned int *puint)
{
    unsigned int tempint;
	char *pChar1, *pChar2;
	
	tempint = *puint;
	pChar2 = (char *)&tempint;
	pChar1 = (char *)puint;
	
	pChar1[0]=pChar2[3];
	pChar1[1]=pChar2[2];
	pChar1[2]=pChar2[1];
	pChar1[3]=pChar2[0];
}

void SwapBytes(unsigned short *pshort)
{
    unsigned short tempshort;
    char *pChar1, *pChar2;
    
    tempshort = *pshort;
    pChar2 = (char *)&tempshort;
    pChar1 = (char *)pshort;
    
    pChar1[0]=pChar2[1];
    pChar1[1]=pChar2[0];
}
#endif
#ifdef MAC_OS_X
void SwapWords(unsigned int *puint)
{
    unsigned int tempint;
	char *pChar1, *pChar2;
	
	tempint = *puint;
	pChar2 = (char *)&tempint;
	pChar1 = (char *)puint;
	
	pChar1[0]=pChar2[3];
	pChar1[1]=pChar2[2];
	pChar1[2]=pChar2[1];
	pChar1[3]=pChar2[0];
}

void SwapBytes(unsigned short *pshort)
{
    unsigned short tempshort;
    char *pChar1, *pChar2;
    
    tempshort = *pshort;
    pChar2 = (char *)&tempshort;
    pChar1 = (char *)pshort;
    
    pChar1[0]=pChar2[1];
    pChar1[1]=pChar2[0];
}
#endif

#define BSIZE 20000
#define NUMBUFFERS	4


// Streaming Test
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Streaming Test
This test streams a long stereo wave file from harddisc - using AL Queuing
*/
ALvoid I_StreamingTest(ALvoid)
{
	FILE			*fp;
	WAVE_Struct		wave;

	ALbyte			data[BSIZE];
	ALuint			Buffers[NUMBUFFERS];
	ALuint   		UnqueueID[NUMBUFFERS];
	ALuint			BufferID;
	ALuint			Sources[1];
	ALint			error;
	ALuint			DataSize;
	ALuint			DataToRead;
	ALint			processed;
	ALboolean		bFinished = AL_FALSE;
	ALuint			Format;
	ALuint			loop;
	ALint			state;
#ifdef _WIN32
	ALubyte			ch;
#endif

	printf("Streaming Test\n");

#ifndef __MACOS__
	fp = fopen("stereo.wav", "rb");
#else
	fp = fopen("STEREO.WAV", "rb"); // not sure why this is needed on MacOS -- the file isn't really in upper-case
#endif

	if (fp == NULL)
	{
		printf("Failed to open stereo.wav\n");
		return;
	}

	// We are going to be queuing x buffers on our source

	// To start, fill all the buffers with audio data from the wave file

	// Read in WAVE Header
	if (fread(&wave, 1, sizeof(WAVE_Struct), fp) != sizeof(WAVE_Struct))
	{
		printf("Invalid wave file\n");
		fclose(fp);
		return;
	}
	
#ifdef __MACOS__
    SwapWords(&wave.dataSize);
    SwapBytes(&wave.Channels);
    SwapWords(&wave.SamplesPerSec);
#endif
#ifdef MAC_OS_X
    SwapWords(&wave.dataSize);
    SwapBytes(&wave.Channels);
    SwapWords(&wave.SamplesPerSec);
#endif

	DataSize = wave.dataSize;

	if (wave.Channels == 1)
		Format = AL_FORMAT_MONO16;
	else
		Format = AL_FORMAT_STEREO16;

	alGetError(); // clear error state
	alGenBuffers(NUMBUFFERS, Buffers);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenBuffers : ", error);
		fclose(fp);
		return;
	}

	alGetError(); // clear error state
	for (loop = 0; loop < NUMBUFFERS; loop++)
	{
		fread(data, 1, BSIZE, fp);
		DataSize -= BSIZE;
#ifndef SWAPBYTES
		alBufferData(Buffers[loop], Format, data, BSIZE, wave.SamplesPerSec);
#else
		for (int i = 0; i < BSIZE; i=i+2)
		{
			SwapBytes((unsigned short *)(data+i));
		}
		alBufferData(Buffers[loop], (ALenum) Format, data, BSIZE, wave.SamplesPerSec);
#endif
		if ((error = alGetError()) != AL_NO_ERROR)
			DisplayALError("alBufferData : ", error);
	}

	alGenSources(1, Sources);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenSources 1 : ", error);
		alDeleteBuffers(NUMBUFFERS, Buffers);
		fclose(fp);
		return;
	}

	alSourcef(Sources[0], AL_GAIN, 0.5f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef AL_GAIN : ", error);

	// Queue the buffers on the source
	alSourceQueueBuffers(Sources[0], NUMBUFFERS, Buffers);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourceQueueBuffers : ", error);

	alSourcei(Sources[0], AL_LOOPING, AL_FALSE);

	// Start playing source
	alSourcePlay(Sources[0]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcePlay source 0 : ", error);

	printf("Press a key to quit\n");

	ALuint count = 0;
	ALuint buffersreturned = 0;
	ALboolean bFinishedPlaying = AL_FALSE;
	ALuint buffersinqueue = NUMBUFFERS;

#ifdef _WIN32
	while (!_kbhit() && !bFinishedPlaying)
#else
    while (!bFinishedPlaying)
#endif
	{
		// Get status
#ifndef LINUX
	        alGetSourcei(Sources[0], AL_BUFFERS_PROCESSED, &processed);
#else
	        alGetSourceiv(Sources[0], AL_BUFFERS_PROCESSED, &processed);
#endif

		// If some buffers have been played, unqueue them and load new audio into them, then add them to the queue
		if (processed > 0)
		{
			buffersreturned += processed;
#ifndef __MACOS__ // don't use SIOUX routines during streaming on MacOS -- messes with interrupts
			printf("Buffers Completed is %d\n", buffersreturned);
#endif

			// Pseudo code for Streaming a buffer with Open AL
			// while (processed)
			//		Unqueue a buffer
			//		Load audio data into buffer (returned by UnQueueBuffers)
			//		if successful
			//			Queue buffer
			//			processed--
			//		else
			//			buffersinqueue--
			//			if buffersinqueue == 0
			//				finished playing !

			while (processed)
			{
				alSourceUnqueueBuffers(Sources[0], 1, &BufferID);
				if ((error = alGetError()) != AL_NO_ERROR)
				{
					DisplayALError("alSourceUnqueueBuffers 1 : ", error);
				}

				if (!bFinished)
				{
					DataToRead = (DataSize > BSIZE) ? BSIZE : DataSize;

					if (DataToRead == DataSize)
						bFinished = AL_TRUE;
					
					fread(data, 1, DataToRead, fp);
					DataSize -= DataToRead;
					
					if (bFinished == AL_TRUE)
					{
						memset(data + DataToRead, 0, BSIZE - DataToRead);
					}

#ifndef SWAPBYTES
					alBufferData(BufferID, Format, data, DataToRead, wave.SamplesPerSec);
#else
					for (int i = 0; i < DataToRead; i = i+2)
					{
						SwapBytes((unsigned short *)(data+i));
					}
					alBufferData(BufferID, (ALenum) Format, (void *) data, DataToRead, wave.SamplesPerSec);
#endif
					if ((error = alGetError()) != AL_NO_ERROR)
						DisplayALError("alBufferData : ", error);

					// Queue buffer
					alSourceQueueBuffers(Sources[0], 1, &BufferID);
					if ((error = alGetError()) != AL_NO_ERROR)
						DisplayALError("alSourceQueueBuffers 1 : ", error);

					processed--;
				}
				else
				{
					buffersinqueue--;
					processed--;

					if (buffersinqueue == 0)
					{
						bFinishedPlaying = true;
						break;
					}
				}
			}
		} else // processed == 0
		{
			// check play status -- if stopped then queue is not being filled fast enough
			alGetSourcei(Sources[0], AL_SOURCE_STATE, &state);
			if (state != AL_PLAYING)
			{
#ifndef __MACOS__ // don't use SIOUX routines during streaming on MacOS -- messes with interrupts
				printf("Queuing underrun detected.\n");
#endif
#ifndef LINUX
				alGetSourcei(Sources[0], AL_BUFFERS_PROCESSED, &processed);
#else
				alGetSourceiv(Sources[0], AL_BUFFERS_PROCESSED, &processed);
#endif
				alSourcePlay(Sources[0]);
			}
		}
	}

#ifdef _WIN32
	if (!bFinishedPlaying)
		ch = getch();
#endif

	alSourceStopv(1, Sources);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourceStopv 1 : ", error);

	alDeleteSources(1, Sources);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alDeleteSources 1 : ", error);

	alDeleteBuffers(NUMBUFFERS, Buffers);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alDeleteBuffers : ", error);

	fclose(fp);

	return;
}


// Multiple Sources Test
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Multiple Sources Test
This test generates a large number of sources, positions, and plays them.
*/
ALvoid I_MultipleSourcesTest()
{
	ALuint	numSources = 0;
	ALuint	Sources[64] = { 0 };
	ALint	error;
	ALuint	i;
	char	ch;
	ALfloat radius;
	double anglestep;
	ALfloat pos[3];

	// Generate as many sources as possible (up to 64)
	for (i = 0; i < 64; i++)
	{
		alGenSources(1, &Sources[i]);
		if ((error = alGetError()) != AL_NO_ERROR)
			break;
		else
			numSources++;
	}

	printf("Multiple Sources Test\n\n");
	printf("Generated %d Sources\n", numSources);

	// Set sources to located in a circle around the listener

	anglestep = (2 * 3.1416) / (ALfloat)numSources;
	radius = 2.0f;

	for (i = 0; i < numSources; i++)
	{
		// Attach buffer
		alSourcei(Sources[i], AL_BUFFER, g_Buffers[0]);

		// Set position
		pos[0] = (float)(cos(anglestep*i) * radius);
		pos[1] = 0.0f;
		pos[2] = (float)(sin(anglestep*i) * radius);
		alSourcefv(Sources[i], AL_POSITION, pos);
		printf("Source %d at %.3f, %.3f, %.3f\n", i, pos[0], pos[1], pos[2]);

		// Enable looping
		alSourcei(Sources[i], AL_LOOPING, AL_TRUE);
	}


	printf("Press '1' to start playing Sources seperately\n");
	printf("Press '2' to stop playing Sources seperately\n");
	printf("Press 'q' to quit\n");

	do
	{
		ch = getUpperCh();
		switch (ch)
		{
			case '1':
				for (i = 0; i < numSources; i++)
				{
					alSourcePlay(Sources[i]);
					if ((error = alGetError()) != AL_NO_ERROR)
						DisplayALError("alSourcePlay : ", error);

					// Delay a little
					delay_ms(100);
				}
				break;
			case '2':
				alSourceStopv(numSources, Sources);
					if ((error = alGetError()) != AL_NO_ERROR)
						DisplayALError("alSourceStopv : ", error);
				break;
		}
	} while (ch != 'Q');

	// Delete the Sources
	alDeleteSources(numSources, Sources);
}

#ifdef TEST_VORBIS
// Vorbis Test
/** used by gendocs.py
$SECTION Interactive Tests
$SUBTITLE Vorbis Test
This test exercises Ogg Vorbis playback functionality.
*/
ALvoid I_VorbisTest()
{
	ALint	error;
	ALuint	source[1];
	ALuint  buffers[2];
	ALuint	Buffer;
	ALint	BuffersInQueue, BuffersProcessed;
	ALbyte	ch;
	ALboolean bLoop = false;
	ALfloat source0Pos[]={ 2.0, 0.0,-2.0};	// Front and right of the listener
	ALfloat source0Vel[]={ 0.0, 0.0, 0.0};

	alGenSources(1,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError((ALbyte *) "alGenSources 1 : ", error);
		return;
	}

	buffers[0] = g_Buffers[7];
	buffers[1] = g_Buffers[7];

	printf("Vorbis Test\n");
	printf("Press '1' to play an Ogg Vorbis buffer on source 0\n");
	printf("Press '2' to toggle looping on / off\n");
	printf("Press 'q' to quit\n");
	printf("Looping is off\n");

	do
	{
		ch = getUpperCh();
 		
		switch (ch)
		{
			case '1':
				// Stop source
				alSourceStop(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourceStop source 0 : ", error);

				// Attach new buffer
				alSourcei(source[0],AL_BUFFER, g_Buffers[7]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcei 0 AL_BUFFER buffer 6 (stereo) : \n", error);

				// Set volume
				alSourcef(source[0],AL_GAIN,0.5f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcef 0 AL_GAIN : \n", error);

				// Set looping
				alSourcei(source[0],AL_LOOPING,bLoop);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcei 0 AL_LOOPING true: \n", error);

				// Play source
				alSourcePlay(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcePlay source 0 : ", error);
		   
		                // delay until done playing
/*				ALint state;
				alGetSourceiv(source[0], AL_SOURCE_STATE, &state);
		                while (state == AL_PLAYING) {
			          sleep(1);
		                } */
				
				break;
			case '2':
				if (bLoop)
				{
					printf("Looping is off\n");
					bLoop = AL_FALSE;
				}
				else
				{
					printf("Looping is on  \n");
					bLoop = AL_TRUE;
				}
				alSourcei(source[0], AL_LOOPING, bLoop);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError((ALbyte *) "alSourcei 0 AL_LOOPING : \n", error);
				break;
				printf("Current Buffer is %d, %d Buffers in queue, %d Processed\n", Buffer, BuffersInQueue, BuffersProcessed);
		}
	} while (ch != 'Q');

	// Release resources
	alSourceStop(source[0]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alSourceStop : ", error);

	alDeleteSources(1, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError((ALbyte *) "alDeleteSources 2 : ", error);
	
	return;
}
#endif
