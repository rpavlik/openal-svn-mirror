// OpenALDemo.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include "al\al.h"
#include "al\alc.h"
#include "al\alut.h"
#include <memory.h>
#include <windows.h>
#include "eax.h"
#include <math.h>

#define PI 3.141592654

// EAX 2.0 GUIDs
const GUID DSPROPSETID_EAX20_ListenerProperties
				= { 0x306a6a8, 0xb224, 0x11d2, { 0x99, 0xe5, 0x0, 0x0, 0xe8, 0xd8, 0xc7, 0x22 } };

const GUID DSPROPSETID_EAX20_BufferProperties
				= { 0x306a6a7, 0xb224, 0x11d2, {0x99, 0xe5, 0x0, 0x0, 0xe8, 0xd8, 0xc7, 0x22 } };

#define NUM_BUFFERS 7	// Number of buffers to be Generated

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


// Global variables

EAXSet	eaxSet;						// EAXSet function, retrieved if EAX Extension is supported
EAXGet	eaxGet;						// EAXGet function, retrieved if EAX Extension is supported
ALboolean g_bEAX;					// Boolean variable to indicate presence of EAX 2.0 Extension
ALuint	g_Buffers[NUM_BUFFERS];		// Array of Buffer IDs

// Function prototypes
ALvoid DisplayALError(ALubyte *szText, ALint errorCode);

// Test Function prototypes
ALvoid PositionTest(ALvoid);
ALvoid LoopingTest(ALvoid);
ALvoid EAXTest(ALvoid);
ALvoid QueueTest(ALvoid);
ALvoid BufferTest(ALvoid);
ALvoid FreqTest(ALvoid);
ALvoid StereoTest(ALvoid);
ALvoid GainTest(ALvoid);
ALvoid StreamingTest(ALvoid);
ALvoid RelativeTest(ALvoid);
ALvoid CreationDeletionTest(ALvoid);
ALvoid MultipleSourcesTest(ALvoid);
ALvoid SourceRollOffTest(ALvoid);

/*
	Display AL Error message
*/
ALvoid DisplayALError(ALbyte *szText, ALint errorcode)
{
	printf("%s%s", szText, alGetString(errorcode));
}

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
	ALCcontext *Context;
	ALCdevice *Device;
	char deviceName[256];
	char *defaultDevice;
	char *deviceList;
	char *devices[12];
	int numDevices, numDefaultDevice, i;

	ALfloat listenerPos[]={0.0,0.0,0.0};
	ALfloat listenerVel[]={0.0,0.0,0.0};
	ALfloat	listenerOri[]={0.0,0.0,-1.0, 0.0,1.0,0.0};	// Listener facing into the screen

	printf("OpenAL Test application\n\n");

	// Initialize Open AL manually

	//Open device
	strcpy(deviceName, "");
	if (alcIsExtensionPresent(NULL, (ALubyte*)"ALC_ENUMERATION_EXT") == AL_TRUE) { // try out enumeration extension
		defaultDevice = (char *)alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
		deviceList = (char *)alcGetString(NULL, ALC_DEVICE_SPECIFIER);
		for (numDevices = 0; numDevices < 12; numDevices++) {devices[numDevices] = NULL;}
		for (numDevices = 0; numDevices < 12; numDevices++) {
			devices[numDevices] = deviceList;
			if (strcmp(devices[numDevices], defaultDevice) == 0) {
				numDefaultDevice = numDevices;
			}
			deviceList += strlen(deviceList);
			if (deviceList[0] == 0) {
				if (deviceList[1] == 0) {
					break;
				} else {
					deviceList += 1;
				}
			}
		}
		if (devices[numDevices] != NULL) {
			numDevices++;
			printf("\nEnumeration extension found -- select an output device:\n");
			printf("0. NULL Device (Default)\n");
			for (i = 0; i < numDevices; i++) {
				printf("%d. %s\n", i + 1, devices[i]);
			}
			printf("\n\n");
			do {
				ch = _getch();
				i = atoi(&ch);
			} while ((i < 0) || (i > numDevices));
			if ((i != 0) && (strlen(devices[i-1]) < 256)) {
				strcpy(deviceName, devices[i-1]);
			}
		}
	}

	if (strlen(deviceName) == 0) {
		Device = alcOpenDevice(NULL); // this is supposed to select the "preferred device"
	} else {
		Device = alcOpenDevice((ALubyte*)deviceName); // have a name from enumeration process above, so use it...
	}

	if (Device == NULL)
	{
		printf("Failed to Initialize Open AL\n");
		exit(-1);
	}

	//Create context(s)
	Context=alcCreateContext(Device,NULL);
	if (Context == NULL)
	{
		printf("Failed to initialize Open AL\n");
		exit(-1);
	}

	//Set active context
	alcGetError(Device);
	alcMakeContextCurrent(Context);
	if (alcGetError(Device) != ALC_NO_ERROR)
	{
		printf("Failed to Make Context Current\n");
		exit(-1);
	}

	// Clear Error Code
	alGetError();
	alcGetError(Device);

	// Set Listener attributes

	// Position ...
	alListenerfv(AL_POSITION,listenerPos);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alListenerfv POSITION : ", error);
		exit(-1);
	}

	// Velocity ...
	alListenerfv(AL_VELOCITY,listenerVel);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alListenerfv VELOCITY : ", error);
		exit(-1);
	}

	// Orientation ...
	alListenerfv(AL_ORIENTATION,listenerOri);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alListenerfv ORIENTATION : ", error);
		exit(-1);
	}

	// Generate Buffers
	alGenBuffers(NUM_BUFFERS, g_Buffers);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenBuffers :", error);
		exit(-1);
	}

	// Load in samples to be used by Test functions

	// Load footsteps.wav
	alutLoadWAVFile("footsteps.wav",&format,&data,&size,&freq,&loop);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alutLoadWAVFile footsteps.wav : ", error);
		// Delete Buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Copy footsteps.wav data into AL Buffer 0
	alBufferData(g_Buffers[0],format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alBufferData buffer 0 : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Unload footsteps.wav
	alutUnloadWAV(format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alutUnloadWAV : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Load ding.wav
	alutLoadWAVFile("ding.wav",&format,&data,&size,&freq,&loop);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alutLoadWAVFile ding.wav : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit (-1);
	}

	// Copy ding.wav audio data into AL Buffer 1
	alBufferData(g_Buffers[1],format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alBufferData buffer 1 : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}
	
	// Unload ding.wav
	alutUnloadWAV(format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alutUnloadWAV : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Load wave1.wav
	alutLoadWAVFile("wave1.wav",&format,&data,&size,&freq,&loop);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alutLoadWAVFile wave1.wav : ", error);
		// Delete Buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Copy wave1.wav data into AL Buffer 2
	alBufferData(g_Buffers[2],format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alBufferData buffer 2 : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Unload wave1.wav
	alutUnloadWAV(format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alutUnloadWAV : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Load wave2.wav
	alutLoadWAVFile("wave2.wav",&format,&data,&size,&freq,&loop);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alutLoadWAVFile wave2.wav : ", error);
		// Delete Buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Copy wave2.wav data into AL Buffer 3
	alBufferData(g_Buffers[3],format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alBufferData buffer 3 : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Unload wave2.wav
	alutUnloadWAV(format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alutUnloadWAV : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

		// Load wave3.wav
	alutLoadWAVFile("wave3.wav",&format,&data,&size,&freq,&loop);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alutLoadWAVFile wave3.wav : ", error);
		// Delete Buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Copy wave3.wav data into AL Buffer 4
	alBufferData(g_Buffers[4],format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alBufferData buffer 4 : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Unload wave3.wav
	alutUnloadWAV(format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alutUnloadWAV : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Load wave4.wav
	alutLoadWAVFile("wave4.wav",&format,&data,&size,&freq,&loop);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alutLoadWAVFile wave4.wav : ", error);
		// Delete Buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Copy wave4.wav data into AL Buffer 5
	alBufferData(g_Buffers[5],format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alBufferData buffer 5 : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Unload wave4.wav
	alutUnloadWAV(format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alutUnloadWAV : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Load stereo.wav
	alutLoadWAVFile("stereo.wav",&format,&data,&size,&freq,&loop);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alutLoadWAVFile stereo.wav : ", error);
		// Delete Buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Copy stereo.wav data into AL Buffer 6
	alBufferData(g_Buffers[6],format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alBufferData buffer 6 : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Unload stereo.wav
	alutUnloadWAV(format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alutUnloadWAV : ", error);
		// Delete buffers
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

	// Check for EAX 2.0 support
	g_bEAX = alIsExtensionPresent((ALubyte*)"EAX2.0");

	if (g_bEAX)
		printf("EAX 2.0 Extension available\n");

	sprintf((char*)szFnName, "EAXSet");

	eaxSet = (EAXSet)alGetProcAddress(szFnName);

	if (eaxSet == NULL)
		g_bEAX = false;
	
	sprintf((char*)szFnName,"EAXGet");

	eaxGet = (EAXGet)alGetProcAddress(szFnName);

	if (eaxGet == NULL)
		g_bEAX = false;

	do
	{
		printf("\nSelect a test from the following options\n\n");
		printf("1 Position Test\n");
		printf("2 Looping Test\n");
		printf("3 EAX 2.0 Test\n");
		printf("4 Queue Test\n");
		printf("5 Buffer Test\n");
		printf("6 Frequency Test\n");
		printf("7 Stereo Test\n");
		printf("8 Gain Test\n");
		printf("9 Streaming Test\n");
		printf("A Source Relative Test\n");
		printf("B Generate / Delete Sources Test\n");
		printf("C Multiple Sources Test\n");
		printf("D Source Roll-off Test\n");
		printf("Q to quit\n\n\n");

		ch = _getch();
		ch = toupper( ch );

		switch (ch)
		{
			case '1':
				PositionTest();
				break;
			case '2':
				LoopingTest();
				break;
			case '3':
				if (g_bEAX)
					EAXTest();
				else
					printf("EAX Extensions NOT available.  Please select another test\n");
				break;
			case '4':
				QueueTest();
				break;
			case '5':
				BufferTest();
				break;
			case '6':
				FreqTest();
				break;
			case '7':
				StereoTest();
				break;
			case '8':
				GainTest();
				break;
			case '9':
				StreamingTest();
				break;
			case 'A':
				RelativeTest();
				break;
			case 'B':
				CreationDeletionTest();
				break;
			case 'C':
				MultipleSourcesTest();
				break;
			case 'D':
				SourceRollOffTest();
				break;
			default:
				break;
		}
	} while (ch != 'Q');

	alDeleteBuffers(NUM_BUFFERS, g_Buffers);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alDeleteBuffers : ", error);
		exit(-1);
	}

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

	return 0;
}


/*
	Buffer Test
	Dynamically attach and unattach different buffers to a single Source
*/
ALvoid BufferTest(ALvoid)
{
	ALuint	source[1];
	ALint	error;
	char ch;

	ALfloat source0Pos[]={ 1.0, 0.0,-1.0};	// Front and right of the listener
	ALfloat source0Vel[]={ 0.0, 0.0, 0.0};
	
	alGenSources(1,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenSources 2 : ", error);
		return;
	}

	alSourcef(source[0],AL_PITCH,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 0 AL_PITCH : \n", error);

	alSourcef(source[0],AL_GAIN,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 0 AL_GAIN : \n", error);
	
	alSourcefv(source[0],AL_POSITION,source0Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_POSITION : \n", error);
	
	alSourcefv(source[0],AL_VELOCITY,source0Vel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_VELOCITY : \n", error);

	alSourcei(source[0],AL_LOOPING,AL_FALSE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_LOOPING true: \n", error);

	printf("Buffer Test\n");
	printf("Press '1' to play buffer 0 on source 0\n");
	printf("Press '2' to play buffer 1 on source 0\n");
	printf("Press '3' to stop source 0\n");
	printf("Press 'q' to quit\n");

	do
	{
		ch = _getch();
		ch = toupper( ch );
		switch (ch)
		{
			case '1':
				// Stop source
				alSourceStop(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceStop 0 : ", error);
				// Attach buffer 0 to source
				alSourcei(source[0], AL_BUFFER, g_Buffers[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcei AL_BUFFER 0 : ", error);
				// Play
				alSourcePlay(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcePlay 0 : ", error);
				break;
			case '2':
				// Stop source
				alSourceStop(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceStop 0 : ", error);
				// Attach buffer 0 to source
				alSourcei(source[0], AL_BUFFER, g_Buffers[1]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcei AL_BUFFER 1 : ", error);
				// Play
				alSourcePlay(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcePlay 0 : ", error);
				break;
			case '3':
				// Stop source
				alSourceStop(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceStop 0 : ", error);
				break;
		}
	} while (ch != 'Q');

	// Release resources
	alSourceStopv(1, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourceStopv 1 : ", error);

	alDeleteSources(1, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alDeleteSources 1 : ", error);

	return;
}


/*
	Position Test
	Creates 2 Sources - one to the front right of the listener, and one to the rear left of the listener
*/
ALvoid PositionTest(ALvoid)
{	
	ALint	error;
	
	ALuint	source[2];
	ALbyte	ch;

	ALfloat source0Pos[]={ -1.0, 0.0, 1.0};	// Behind and to the left of the listener
	ALfloat source0Vel[]={ 0.0, 0.0, 0.0};

	ALfloat source1Pos[]={ 1.0, 0.0, -1.0};	// Front and to the right of the listener
	ALfloat source1Vel[]={ 0.0, 0.0, 0.0};

	alGenSources(2,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenSources 2 : ", error);
		return;
	}

	alSourcef(source[0],AL_PITCH,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 0 AL_PITCH : \n", error);

	alSourcef(source[0],AL_GAIN,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 0 AL_GAIN : \n", error);

	alSourcefv(source[0],AL_POSITION,source0Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_POSITION : \n", error);
	
	alSourcefv(source[0],AL_VELOCITY,source0Vel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_VELOCITY : \n", error);

	alSourcei(source[0],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_BUFFER buffer 0 : \n", error);

	alSourcei(source[0],AL_LOOPING,AL_TRUE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_LOOPING true: \n", error);

	alSourcef(source[1],AL_PITCH,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 1 AL_PITCH : \n", error);

	alSourcef(source[1],AL_GAIN,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 1 AL_GAIN : \n", error);

	alSourcefv(source[1],AL_POSITION,source1Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 1 AL_POSITION : \n", error);

	alSourcefv(source[1],AL_VELOCITY,source1Vel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 1 AL_VELOCITY : \n", error);

	alSourcei(source[1],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 1 AL_BUFFER buffer 1 : \n", error);

	alSourcei(source[1],AL_LOOPING,AL_FALSE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 1 AL_LOOPING false: \n", error);

	printf("Position Test\n");
	printf("Press '1' to play source 0 rear left of listener\n");
	printf("Press '2' to play source 1 once (single shot) front right of listener\n");
	printf("Press '3' to stop source 0\n");
	printf("Press '4' to stop source 1\n");
	printf("Press '5' to set Source 0 Reference Distance to 2\n");
	printf("Press '6' to set Source 0 Reference Distance to 1\n");
	printf("Press 'q' to quit\n");

	do
	{
		ch = _getch();
		ch = toupper( ch );
		switch (ch)
		{
			case '1':
				alSourcePlay(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcePlay source 0 : ", error);
				break;
			case '2':
				alSourcePlay(source[1]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcePlay source 1 : ", error);
				break;
			case '3':
				alSourceStop(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceStop source 0 : ", error);
				break;
			case '4':
				alSourceStop(source[1]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceStop source 1 : ", error);
				break;
			case '5':
				alSourcef(source[0], AL_REFERENCE_DISTANCE, 2.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcef REFERENCE_DISTANCE 2 : ", error);
				break;
			case '6':
				alSourcef(source[0], AL_REFERENCE_DISTANCE, 1.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcef REFERENCE_DISTANCE 1 : ", error);
				break;

		}
	} while (ch != 'Q');

	// Release resources
	alSourceStopv(2, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourceStopv 2 : ", error);

	alDeleteSources(2, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alDeleteSources 2 : ", error);

	return;
}


/*
	Looping Test
	Tests the ability to switch Looping ON and OFF for a particular source, either before or during
	Playback.  (If looping is switched off during playback, the buffer should finish playing until
	the end of the sample.)
*/
ALvoid LoopingTest(ALvoid)
{
	ALint	error;
	ALuint	source[2];
	ALbyte	ch;
	ALboolean bLooping0 = AL_FALSE;
	ALboolean bLooping1 = AL_FALSE;

	ALfloat source0Pos[]={ -1.0, 0.0, -1.0};	// Front left of the listener
	ALfloat source0Vel[]={ 0.0, 0.0, 0.0};

	ALfloat source1Pos[]={ 1.0, 0,0, -1.0};		// Front right of the listener
	ALfloat source1Vel[]={ 0.0, 0.0, 0,0};

	// Clear Error Code
	alGetError();

	alGenSources(2,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenSources 1 : ", error);
		return;
	}
	
	alSourcef(source[0],AL_PITCH,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 0 AL_PITCH : \n", error);

	alSourcef(source[0],AL_GAIN,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 0 AL_GAIN : \n", error);
	
	alSourcefv(source[0],AL_POSITION,source0Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_POSITION : \n", error);
	
	alSourcefv(source[0],AL_VELOCITY,source0Vel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_VELOCITY : \n", error);

	alSourcei(source[0],AL_BUFFER, g_Buffers[0]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_BUFFER buffer 0 : \n", error);

	alSourcei(source[0],AL_LOOPING,AL_FALSE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_LOOPING false : \n", error);


	alSourcef(source[1],AL_PITCH,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 1 AL_PITCH : \n", error);

	alSourcef(source[1],AL_GAIN,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 1 AL_GAIN : \n", error);

	alSourcefv(source[1],AL_POSITION,source1Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 1 AL_POSITION : \n", error);

	alSourcefv(source[1],AL_VELOCITY,source1Vel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 1 AL_VELOCITY : \n", error);

	alSourcei(source[1],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 1 AL_BUFFER buffer 1 : \n", error);

	alSourcei(source[1],AL_LOOPING,AL_FALSE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 1 AL_LOOPING false: \n", error);

	printf("Looping Test\n");
	printf("Press '1' to play source 0 once (single shot)\n");
	printf("Press '2' to toggle looping on source 0\n");
	printf("Press '3' to play source 1 once (single shot)\n");
	printf("Press '4' to toggle looping on source 1\n");
	printf("Press 'q' to quit\n");
	printf("\nSource 0 : Not looping Source 1 : Not looping\r");
	do
	{
		ch = _getch();
		ch = toupper( ch );
		switch (ch)
		{
			case '1':
				alSourcePlay(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcePlay source 0 : ", error);
				break;
			case '2':
				if (bLooping0 == AL_FALSE)
				{
					bLooping0 = AL_TRUE;
					if (bLooping1)
						printf("Source 0 : Looping     Source 1 : Looping    \r");
					else
						printf("Source 0 : Looping     Source 1 : Not looping\r");
				}
				else
				{
					bLooping0 = AL_FALSE;
					if (bLooping1)
						printf("Source 0 : Not looping Source 1 : Looping    \r");
					else
						printf("Source 0 : Not looping Source 1 : Not looping\r");
				}
				alSourcei(source[0], AL_LOOPING, bLooping0);
				break;
			case '3':
				alSourcePlay(source[1]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcePlay source 1 : ", error);
				break;
			case '4':
				if (bLooping1 == AL_FALSE)
				{
					bLooping1 = AL_TRUE;
					if (bLooping0)
						printf("Source 0 : Looping     Source 1 : Looping    \r");
					else
						printf("Source 0 : Not looping Source 1 : Looping    \r");
				}
				else
				{
					bLooping1 = AL_FALSE;
					if (bLooping0)
						printf("Source 0 : Looping     Source 1 : Not looping\r");
					else
						printf("Source 0 : Not looping Source 1 : Not looping\r");
				}
				alSourcei(source[1], AL_LOOPING, bLooping1);
				break;
		}
	} while (ch != 'Q');

	printf("\n");

	// Release resources
	alSourceStop(source[0]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourceStop source 1 : ", error);

	alDeleteSources(2, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alDeleteSources 1 : ", error);

	return;
}

/*
	EAXTest (Only runs if the EAX Extension is present)
	Uses 2 Sources to test out EAX 2.0 Reverb, Occlusion and Obstruction.  Also tests the use of the
	DEFERRED flag in EAX.
*/
ALvoid EAXTest(ALvoid)
{
	ALint	error;
	ALuint	source[2];
	ALbyte	ch;
	ALuint	Env;
	ALint	Room;
	ALint	Occlusion;
	ALint	Obstruction;
	EAXBUFFERPROPERTIES eaxBufferProp0;

	ALfloat source0Pos[]={ -1.0, 0.0, 1.0};	// Behind and to the left of the listener
	ALfloat source0Vel[]={ 0.0, 0.0, 0.0};

	ALfloat source1Pos[]={ 1.0, 0.0,-1.0};	// Front and right of the listener
	ALfloat source1Vel[]={ 0.0, 0.0, 0.0};

	alGenSources(2,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenSources 2 : ", error);
		return;
	}
	
	alSourcef(source[0],AL_PITCH,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 0 AL_PITCH : \n", error);

	alSourcef(source[0],AL_GAIN,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 0 AL_GAIN : \n", error);
	
	alSourcefv(source[0],AL_POSITION,source0Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_POSITION : \n", error);
	
	alSourcefv(source[0],AL_VELOCITY,source0Vel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_VELOCITY : \n", error);

	alSourcei(source[0],AL_BUFFER, g_Buffers[0]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_BUFFER buffer 0 : \n", error);

	alSourcei(source[0],AL_LOOPING,AL_TRUE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_LOOPING true: \n", error);


	alSourcef(source[1],AL_PITCH,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 1 AL_PITCH : \n", error);

	alSourcef(source[1],AL_GAIN,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 1 AL_GAIN : \n", error);

	alSourcefv(source[1],AL_POSITION,source1Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 1 AL_POSITION : \n", error);

	alSourcefv(source[1],AL_VELOCITY,source1Vel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 1 AL_VELOCITY : \n", error);

	alSourcei(source[1],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 1 AL_BUFFER buffer 1 : \n", error);

	alSourcei(source[1],AL_LOOPING,AL_FALSE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 1 AL_LOOPING false: \n", error);

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
		ch = _getch();
		ch = toupper( ch );
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
					DisplayALError("eaxSet EAXLISTENER_ENVIRONMENT | EAXLISTENER_DEFERRED : \n", error);
				break;

			case '6':
				Room = -10000;
				eaxSet(&DSPROPSETID_EAX20_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOM | DSPROPERTY_EAXLISTENER_DEFERRED, NULL,
					&Room, sizeof(ALint));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("eaxSet EAXLISTENER_ROOM | EAXLISTENER_DEFERRED : \n", error);
				break;

			case '7':
				eaxGet(&DSPROPSETID_EAX20_BufferProperties, DSPROPERTY_EAXBUFFER_ALLPARAMETERS, source[0],
					&eaxBufferProp0, sizeof(EAXBUFFERPROPERTIES));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("eaxGet EAXBUFFER_ALLPARAMETERS : \n", error);
				eaxBufferProp0.lOcclusion = -5000;
				eaxSet(&DSPROPSETID_EAX20_BufferProperties, DSPROPERTY_EAXBUFFER_ALLPARAMETERS | DSPROPERTY_EAXBUFFER_DEFERRED, source[0],
					&eaxBufferProp0, sizeof(EAXBUFFERPROPERTIES));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("eaxSet EAXBUFFER_ALLPARAMETERS | EAXBUFFER_DEFERRED : \n", error);
				break;

			case '8':
				Occlusion = 0;
				eaxSet(&DSPROPSETID_EAX20_BufferProperties, DSPROPERTY_EAXBUFFER_OCCLUSION | DSPROPERTY_EAXBUFFER_DEFERRED, source[0],
					&Occlusion, sizeof(ALint));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("eaxSet EAXBUFFER_OCCLUSION | EAXBUFFER_DEFERRED : \n", error);
				break;

			case '9':
				Obstruction = -5000;
				eaxSet(&DSPROPSETID_EAX20_BufferProperties, DSPROPERTY_EAXBUFFER_OBSTRUCTION, source[1],
					&Obstruction, sizeof(ALint));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("eaxSet EAXBUFFER_OBSTRUCTION : \n", error);
				break;

			case '0':
				Obstruction = 0;
				eaxSet(&DSPROPSETID_EAX20_BufferProperties, DSPROPERTY_EAXBUFFER_OBSTRUCTION, source[1],
					&Obstruction, sizeof(ALint));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("eaxSet EAXBUFFER_OBSTRUCTION : \n", error);
				break;

			case 'C':
				// Commit settings on source 0
				eaxSet(&DSPROPSETID_EAX20_BufferProperties, DSPROPERTY_EAXBUFFER_COMMITDEFERREDSETTINGS,
					source[0], NULL, 0);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("eaxSet EAXBUFFER_COMMITDEFERREDSETTINGS : \n", error);

				// Commit Listener settings
				eaxSet(&DSPROPSETID_EAX20_ListenerProperties, DSPROPERTY_EAXLISTENER_COMMITDEFERREDSETTINGS,
					NULL, NULL, 0);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("eaxSet EAXLISTENER_COMMITDEFERREDSETTINGSENVIRONMENT : \n", error);
				break;
		}
	} while (ch != 'Q');

	// Release resources
	alSourceStopv(2, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourceStopv 2 : ", error);

	alDeleteSources(2, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alDeleteSources 2 : ", error);

	return;
}

/*
	QueueTest
	Tests the ability to queue and unqueue a sequence of buffers on a Source. (Buffers can only be
	unqueued if they have been PROCESSED by a Source.)
*/
ALvoid QueueTest(ALvoid)
{
	ALint	error;
	ALuint	source[1];
	ALbyte	ch;
	ALuint  buffers[5];
	ALuint  *buffersremoved;
	ALboolean bLooping;
	ALint	BuffersInQueue, BuffersProcessed;
	ALfloat source0Pos[]={ 0.0, 0.0, -1.0};	// Immediately in front of listener
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

	printf("EAX Test\n\n");
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

	printf("Source 0 not looping\r");

	buffers[0] = g_Buffers[2];
	buffers[1] = g_Buffers[3];
	buffers[2] = g_Buffers[4];
	buffers[3] = g_Buffers[5];
	buffers[4] = NULL;
	
	do
	{
		ch = _getch();
		ch = toupper( ch );
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
					printf("Source 0 not looping\r");
				}
				else
				{
					bLooping = AL_TRUE;
					printf("Source 0 looping    \r");
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
					DisplayALError("alSourceUnqueueBuffers 1 : ", error);
				else
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
				
					printf("\nRemoved Buffer %d from queue\r", buffersremoved[0]);
				}

				delete buffersremoved;
				break;
			case 'B':
				// Unqueue first 2 Buffers
				buffersremoved = new ALuint[2];
				alSourceUnqueueBuffers(source[0], 2, buffersremoved);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceUnqueueBuffers 2 : ", error);
				else
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
					printf("\nRemoved Buffers %d and %d from queue\r", buffersremoved[0], buffersremoved[1]);
				}

				delete buffersremoved;
				break;
			case 'C':
				// Unqueue first 3 Buffers
				buffersremoved = new ALuint[3];
				alSourceUnqueueBuffers(source[0], 3, buffersremoved);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceUnqueueBuffers 3 : ", error);
				else
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
					printf("\nRemoved Buffers %d, %d and %d from queue\r", buffersremoved[0], buffersremoved[1],
						buffersremoved[2]);
				}

				delete buffersremoved;
				break;
			case 'D':
				// Unqueue first 4 Buffers
				buffersremoved = new ALuint[4];
				alSourceUnqueueBuffers(source[0], 4, buffersremoved);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceUnqueueBuffers 1 : ", error);
				else
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

					printf("\nRemoved Buffers %d, %d, %d and %d from queue\r", buffersremoved[0], buffersremoved[1],
						buffersremoved[2], buffersremoved[3]);
				}

				delete buffersremoved;
				break;
			case 'E':
				// Unqueue first 5 Buffers
				buffersremoved = new ALuint[5];
				alSourceUnqueueBuffers(source[0], 5, buffersremoved);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceUnqueueBuffers 1 : ", error);
				else
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

					printf("\nRemoved Buffers %d, %d, %d, %d and %d from queue\r", buffersremoved[0], buffersremoved[1],
						buffersremoved[2], buffersremoved[3], buffersremoved[4]);
				}

				delete buffersremoved;
				break;
			case 'F':
				alSourcei(source[0], AL_BUFFER, NULL);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSource AL_BUFFER NULL : ", error);
				break;
			case '0':
				// Retrieve number of buffers in queue
				alGetSourcei(source[0], AL_BUFFERS_QUEUED, &BuffersInQueue);
				// Retrieve number of processed buffers
				alGetSourcei(source[0], AL_BUFFERS_PROCESSED, &BuffersProcessed);
				// Retrieve current buffer
				alGetSourcei(source[0], AL_BUFFER, (ALint*)&Buffer);
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
				
				printf("Current Buffer is %d, %d Buffers in queue, %d Processed\r", Buffer, BuffersInQueue, BuffersProcessed);
				
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

/*
	FreqTest
	Test AL_PITCH functionality
*/
ALvoid FreqTest(ALvoid)
{
	ALint	error;
	ALuint	source[1];
	ALbyte	ch;
	ALfloat source0Pos[]={ 1.0, 0.0,-1.0};	// Front and right of the listener
	ALfloat source0Vel[]={ 0.0, 0.0, 0.0};


	alGenSources(1,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenSources 1 : ", error);
		return;
	}
	
	alSourcef(source[0],AL_PITCH,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 0 AL_PITCH : \n", error);

	alSourcef(source[0],AL_GAIN,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 0 AL_GAIN : \n", error);
	
	alSourcefv(source[0],AL_POSITION,source0Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_POSITION : \n", error);
	
	alSourcefv(source[0],AL_VELOCITY,source0Vel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_VELOCITY : \n", error);

	alSourcei(source[0],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_BUFFER buffer 1 : \n", error);

	alSourcei(source[0],AL_LOOPING,AL_TRUE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_LOOPING true: \n", error);

	printf("Frequency Test\n");
	printf("Press '1' to play source 0 (looping)\n");
	printf("Press '2' to Double Frequency\n");
	printf("Press '3' to Halve Frequency\n");
	printf("Press 'q' to quit\n");

	do
	{
		ch = _getch();
		ch = toupper( ch );
		switch (ch)
		{
			case '1':
				alSourcePlay(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcePlay source 0 : ", error);
				break;
			case '2':
				alSourcef(source[0], AL_PITCH, 2.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcef source 0 AL_PITCH 2.0 : ", error);
				break;
			case '3':
				alSourcef(source[0], AL_PITCH, 0.5f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcef source 0 AL PITCH 0.5: ", error);
				break;
		}
	} while (ch != 'Q');

	// Release resources
	alSourceStopv(1, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourceStopv 2 : ", error);

	alDeleteSources(1, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alDeleteSources 2 : ", error);
	
	return;
}

/*
	Stereo Test
	Plays a stereo buffer
*/
ALvoid StereoTest(ALvoid)
{
	ALint	error;
	ALuint	source[1];
	ALuint  buffers[2];
	ALuint	Buffer;
	ALint	BuffersInQueue, BuffersProcessed;
	ALbyte	ch;
	ALboolean bLoop = true;
	ALfloat source0Pos[]={ 1.0, 0.0,-1.0};	// Front and right of the listener
	ALfloat source0Vel[]={ 0.0, 0.0, 0.0};

	alGenSources(1,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenSources 1 : ", error);
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
	printf("Looping is on\r");

	do
	{
		ch = _getch();
		ch = toupper( ch );
		switch (ch)
		{
			case '1':
				// Stop source
				alSourceStop(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceStop source 0 : ", error);

				// Attach new buffer
				alSourcei(source[0],AL_BUFFER, g_Buffers[6]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcei 0 AL_BUFFER buffer 6 (stereo) : \n", error);

				// Set volume
				alSourcef(source[0],AL_GAIN,1.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcef 0 AL_GAIN : \n", error);

				// Set looping
				alSourcei(source[0],AL_LOOPING,bLoop);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcei 0 AL_LOOPING true: \n", error);

				// Play source
				alSourcePlay(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcePlay source 0 : ", error);
				
				break;
			case '2':
				// Stop source
				alSourceStop(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceStop source 0 : ", error);

				// Attach new buffer
				alSourcei(source[0],AL_BUFFER, g_Buffers[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcei 0 AL_BUFFER buffer 0 (mono) : \n", error);

				// Set 3D position
				alSourcefv(source[0],AL_POSITION,source0Pos);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcefv 0 AL_POSITION : \n", error);
	
				// Set 3D velocity
				alSourcefv(source[0],AL_VELOCITY,source0Vel);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcefv 0 AL_VELOCITY : \n", error);

				// Set volume to full
				alSourcef(source[0],AL_GAIN,1.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcef 0 AL_GAIN : \n", error);

				// Set Looping
				alSourcei(source[0],AL_LOOPING,bLoop);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcei 0 AL_LOOPING : \n", error);

				// Play source
				alSourcePlay(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcePlay source 0 : ", error);
				break;
			case '3':
				alSourceStop(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceStop source 0 : ", error);
				break;
			case '4':
				alSourceStop(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceStop Source 0 : ", error);

				// Attach NULL buffer to source to clear everything
				alSourcei(source[0], AL_BUFFER, 0);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcei AL_BUFFER (NULL) : ", error);

				alSourceQueueBuffers(source[0], 2, buffers);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceQueueBuffers 2 (stereo) : ", error);

				// Set Looping
				alSourcei(source[0],AL_LOOPING,bLoop);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcei 0 AL_LOOPING : \n", error);

				alSourcePlay(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcePlay Source 0 : ", error);
				break;
			case '5':
				alSourceUnqueueBuffers(source[0], 2, buffers);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceUnqueueBuffers 2 (stereo) : ", error);
				break;
			case '6':
				if (bLoop)
				{
					printf("Looping is off\r");
					bLoop = AL_FALSE;
				}
				else
				{
					printf("Looping is on  \r");
					bLoop = AL_TRUE;
				}
				alSourcei(source[0], AL_LOOPING, bLoop);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcei 0 AL_LOOPING : \n", error);
				break;
			case '0':
				// Retrieve number of buffers in queue
				alGetSourcei(source[0], AL_BUFFERS_QUEUED, &BuffersInQueue);
				// Retrieve number of processed buffers
				alGetSourcei(source[0], AL_BUFFERS_PROCESSED, &BuffersProcessed);
				// Retrieve current buffer
				alGetSourcei(source[0], AL_BUFFER, (ALint*)&Buffer);
				if (Buffer == buffers[0])
					Buffer = 6;
				else if (Buffer == buffers[1])
					Buffer = 6;
				else
					Buffer = 0;
				
				printf("Current Buffer is %d, %d Buffers in queue, %d Processed\r", Buffer, BuffersInQueue, BuffersProcessed);
				
				break;
		}
	} while (ch != 'Q');

	// Release resources
	alSourceStop(source[0]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourceStop : ", error);

	alDeleteSources(1, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alDeleteSources 2 : ", error);
	
	return;
}

/*
	GainTest
	Play 2 Sources - control gain of each, and control Listener Gain
*/
ALvoid GainTest(ALvoid)
{
	ALint	error;
	ALuint	source[2];
	ALbyte	ch;

	ALfloat source0Pos[]={ 1.0, 0.0,-1.0};	// Front and right of the listener
	ALfloat source0Vel[]={ 0.0, 0.0, 0.0};

	ALfloat source1Pos[]={-1.0, 0.0,-1.0};
	ALfloat source1Vel[]={ 0.0, 0.0, 0.0};

	alGenSources(2,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenSources 2 : ", error);
		return;
	}

	alSourcef(source[0],AL_PITCH,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 0 AL_PITCH : \n", error);

	alSourcef(source[0],AL_GAIN,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 0 AL_GAIN : \n", error);
	
	alSourcefv(source[0],AL_POSITION,source0Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_POSITION : \n", error);
	
	alSourcefv(source[0],AL_VELOCITY,source0Vel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_VELOCITY : \n", error);

	alSourcei(source[0],AL_BUFFER, g_Buffers[0]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_BUFFER buffer 0 : \n", error);

	alSourcei(source[0],AL_LOOPING,AL_TRUE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_LOOPING true: \n", error);


	alSourcef(source[1],AL_PITCH,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 1 AL_PITCH : \n", error);

	alSourcef(source[1],AL_GAIN,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 1 AL_GAIN : \n", error);

	alSourcefv(source[1],AL_POSITION,source1Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 1 AL_POSITION : \n", error);

	alSourcefv(source[1],AL_VELOCITY,source1Vel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 1 AL_VELOCITY : \n", error);

	alSourcei(source[1],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 1 AL_BUFFER buffer 1 : \n", error);

	alSourcei(source[1],AL_LOOPING,AL_TRUE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 1 AL_LOOPING true: \n", error);

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
		ch = _getch();
		ch = toupper( ch );
		switch (ch)
		{
			case '1':
				alSourcePlay(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcePlay source 0 : ", error);
				break;
			case '2':
				alSourcePlay(source[1]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcePlay source 1 : ", error);
				break;
			case '3':
				alSourceStop(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceStop source 0 : \n", error);
				break;
			case '4':
				alSourceStop(source[1]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceStop source 1 : \n", error);
				break;
			case '5':
				alSourcef(source[0],AL_GAIN,1.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcef 0 AL_GAIN 1.0 : \n", error);
				break;
			case '6':
				alSourcef(source[0],AL_GAIN,0.5f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcef 0 AL_GAIN 0.5 : \n", error);
				break;
			case '7':
				alSourcef(source[0],AL_GAIN,0.25f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcef 0 AL_GAIN 0.25 : \n", error);
				break;
			case '8':
				alSourcef(source[0],AL_GAIN,0.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcef 0 AL_GAIN 0.0 : \n", error);
				break;
			case 'A':
				alListenerf(AL_GAIN,1.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alListenerf AL_GAIN 1.0 : \n", error);
				break;
			case 'B':
				alListenerf(AL_GAIN,0.5f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alListenerf AL_GAIN 0.5 : \n", error);
				break;
			case 'C':
				alListenerf(AL_GAIN,0.25f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alListenerf AL_GAIN 0.25 : \n", error);
				break;
			case 'D':
				alListenerf(AL_GAIN,0.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alListenerf AL_GAIN 0.0 : \n", error);
				break;
		}
	} while (ch != 'Q');

	// Release resources
	alSourceStopv(2,source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourceStop : ", error);

	alDeleteSources(2, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alDeleteSources 2 : ", error);
	
	return;
}

#define BSIZE		(4608*2)
#define NUMBUFFERS	4

/*
	Streaming Test
	Stream a long stereo wave file from harddisc - using AL Queuing 
*/
ALvoid StreamingTest(ALvoid)
{
	FILE			*fp;
	WAVE_Struct		wave;

	ALbyte			data[BSIZE];
	ALuint			Buffers[NUMBUFFERS];
	ALuint			BufferID;
	ALuint			Sources[1];
	ALint			error;
	ALuint			DataSize;
	ALuint			DataToRead;
	ALubyte			ch;
	ALint			processed;
	ALboolean		bFinished = AL_FALSE;
	ALuint			Format;
	ALuint			loop;
	ALint			i, state;

	printf("Streaming Test\n");

	fp = fopen("stereo.wav", "rb");

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

	DataSize = wave.dataSize;

	if (wave.Channels == 1)
		Format = AL_FORMAT_MONO16;
	else
		Format = AL_FORMAT_STEREO16;

	alGenBuffers(NUMBUFFERS, Buffers);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenBuffers : ", error);
		fclose(fp);
		return;
	}

	for (loop = 0; loop < NUMBUFFERS; loop++)
	{
		fread(data, 1, BSIZE, fp);
		DataSize -= BSIZE;
		alBufferData(Buffers[loop], Format, data, BSIZE, wave.SamplesPerSec);
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

	alSourcef(Sources[0], AL_GAIN, 1.0f);
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

	int starttime = timeGetTime();
	int curtime = starttime;

	printf("Started playing at %d\n", starttime);

	printf("Press a key to quit\n");

	ALuint count = 0;
	ALuint buffersreturned = 0;
	ALboolean bFinishedPlaying = AL_FALSE;
	ALuint buffersinqueue = NUMBUFFERS;

	while (!_kbhit() && !bFinishedPlaying)
	{
		// Get status
		alGetSourcei(Sources[0], AL_BUFFERS_PROCESSED, &processed);

		// If some buffers have been played, unqueue them and load new audio into them, then add them to the queue
		if (processed > 0)
		{
			buffersreturned += processed;
			
			curtime = timeGetTime() - starttime;
			printf("Time is %d ... Buffers Completed is %d   \n", curtime, buffersreturned);
	
			// Pseudo code for Streaming with Open AL
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

					alBufferData(BufferID, Format, data, BSIZE, wave.SamplesPerSec);
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
		}

		// Get state
		alGetSourcei(Sources[0], AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING)
		{
			printf("Buffer stopped .. need to restart it\n\n\n");

			// Unqueue all buffers
			alGetSourcei(Sources[0], AL_BUFFERS_PROCESSED, &processed);

			printf("Source stopped, unqueuing %d buffers\n", processed);

			for (i = 0; i < processed; i++)
			{
				alSourceUnqueueBuffers(Sources[0], 1, &BufferID);

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

					alBufferData(BufferID, Format, data, BSIZE, wave.SamplesPerSec);
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
			// Restart playback
			alSourcePlay(Sources[0]);
		}
	}

	if (!bFinishedPlaying)
		ch = getch();

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


ALvoid RelativeTest(ALvoid)
{
	ALint	error;
	
	ALuint	source[1];
	ALbyte	ch;
	ALboolean bRelative = AL_FALSE;

	ALfloat Pos[3];
	ALfloat LPos[3];
	ALfloat Vel[]={ 0.0, 0.0, 0.0};

	alGenSources(1, source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenSources 1 : ", error);
		return;
	}
	
	alSourcef(source[0],AL_PITCH,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 0 AL_PITCH : \n", error);

	alSourcef(source[0],AL_GAIN,1.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 0 AL_GAIN : \n", error);
	
	alSourcefv(source[0],AL_VELOCITY, Vel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_VELOCITY : \n", error);

	alSourcei(source[0],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_BUFFER buffer 0 : \n", error);

	alSourcei(source[0],AL_LOOPING,AL_TRUE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_LOOPING true: \n", error);

	alSourcei(source[0], AL_SOURCE_RELATIVE, AL_FALSE);	
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_SOURCE_RELATIVE AL_FALSE : \n", error);

	printf("Source Relative Test\n");
	printf("Press '1' to play source 0 at 1, 0, 0\n");
	printf("Press '2' to move listener  to 2, 0, 0\n");
	printf("Press '3' to toggle SOURCE RELATIVE Mode\n");
	printf("Press 'q' to quit\n");

	do
	{
		ch = _getch();
		ch = toupper( ch );
		switch (ch)
		{
			case '1':
				Pos[0] = 1.f;
				Pos[1] = 0.f;
				Pos[2] = 0.f;
				alSourcefv(source[0],AL_POSITION,Pos);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcefv 0 AL_POSITION : \n", error);
				
				alSourcePlay(source[0]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcePlay source 0 : ", error);
				break;
			case '2':
				LPos[0] = 2.f;
				LPos[1] = 0.0f;
				LPos[2] = 0.0f;
				alListenerfv(AL_POSITION, LPos);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alListenerfv  AL_POSITION : ", error);
				break;
			case '3':
				if (bRelative)
				{
					printf("Source Relative == FALSE\r");
					alSourcei(source[0], AL_SOURCE_RELATIVE, AL_FALSE);
					bRelative = AL_FALSE;
				}
				else
				{
					alSourcei(source[0], AL_SOURCE_RELATIVE, AL_TRUE);
					printf("Source Relative == TRUE \r");
					bRelative = AL_TRUE;
				}
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcei source 0 AL_SOURCE_RELATIVE : ", error);
				break;
		}
	} while (ch != 'Q');

	// Release resources
	alSourceStopv(1, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourceStopv 1 : ", error);

	alDeleteSources(1, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alDeleteSources 1 : ", error);

	return;
}

/*
	Test that Source can be Generated and Deleted
*/
ALvoid CreationDeletionTest(ALvoid)
{
	ALuint Sources[4] = { 0 };
	ALint	error;
	char ch;

	printf("Source Creation and Deletion Test\n");
	printf("Press '1' to Generate Source 0\n");
	printf("Press '2' to Generate Source 1\n");
	printf("Press '3' to Generate Source 2\n");
	printf("Press '4' to Generate Source 3\n");
	printf("Press '5' to Delete Source 0\n");
	printf("Press '6' to Delete Source 1\n");
	printf("Press '7' to Delete Source 2\n");
	printf("Press '8' to Delete Source 3\n");
	printf("Press 'q' to quit\n");

	do
	{
		ch = _getch();
		ch = toupper( ch );
		switch (ch)
		{
			case '1':
				alGenSources(1, &(Sources[0]));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alGenerateSources 1 : ", error);
				break;
			case '2':
				alGenSources(1, &(Sources[1]));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alGenerateSources 1 : ", error);
				break;
			case '3':
				alGenSources(1, &(Sources[2]));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alGenerateSources 1 : ", error);
				break;
			case '4':
				alGenSources(1, &(Sources[3]));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alGenerateSources 1 : ", error);
				break;
			case '5':
				alDeleteSources(1, &(Sources[0]));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alDeleteSources 1 : ", error);
				break;
			case '6':
				alDeleteSources(1, &(Sources[1]));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alDeleteSources 1 : ", error);
				break;
			case '7':
				alDeleteSources(1, &(Sources[2]));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alDeleteSources 1 : ", error);
				break;
			case '8':
				alDeleteSources(1, &(Sources[3]));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alDeleteSources 1 : ", error);
				break;
		}
	} while (ch != 'Q');
}


ALvoid MultipleSourcesTest()
{
	ALuint	numSources = 0;
	ALuint	Sources[64] = { 0 };
	ALuint	SourceStopped[64] = { 0 };
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

	anglestep = (2 * PI) / (ALfloat)numSources;
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

		// Enable looping
		alSourcei(Sources[i], AL_LOOPING, AL_TRUE);
	}


	printf("Press '1' to start playing Sources seperately\n");
	printf("Press '2' to stop playing Sources seperately\n");
	printf("Press 'q' to quit\n");

	do
	{
		ch = _getch();
		ch = toupper( ch );
		switch (ch)
		{
			case '1':
				for (i = 0; i < numSources; i++)
				{
					alSourcePlay(Sources[i]);
					if ((error = alGetError()) != AL_NO_ERROR)
						DisplayALError("alSourcePlay : ", error);

					// Delay a little
					Sleep(100);
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

ALvoid SourceRollOffTest()
{
	ALint	error;
	
	ALuint	source[2];
	ALbyte	ch;

	ALfloat source0Pos[]={ -3.0, 0.0, 3.0};	// Behind and to the left of the listener
	ALfloat source1Pos[]={ 3.0, 0.0, -3.0};	// Front and to the right of the listener

	alGenSources(2,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenSources 2 : ", error);
		return;
	}

	alSourcefv(source[0],AL_POSITION,source0Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_POSITION : \n", error);
	
	alSourcei(source[0],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_BUFFER buffer 0 : \n", error);

	alSourcei(source[0],AL_LOOPING,AL_TRUE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_LOOPING true: \n", error);

	alSourcefv(source[1],AL_POSITION,source1Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 1 AL_POSITION : \n", error);

	alSourcei(source[1],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 1 AL_BUFFER buffer 1 : \n", error);

	alSourcei(source[1],AL_LOOPING,AL_TRUE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 1 AL_LOOPING false: \n", error);

	printf("Source Roll-off Test\n");
	printf("Press '1' to play source 0 rear left of listener\n");
	printf("Press '2' to stop source 0\n");
	printf("Press '3' to play source 1 front right of listener\n");
	printf("Press '4' to stop source 1\n");
	printf("Press '5' to set Source 0 Roff-off Factor to 0.5\n");
	printf("Press '6' to set Source 0 Roll-off Factor to 1.0\n");
	printf("Press '7' to set Source 0 Roll-off Factor to 2.0\n");
	printf("Press '8' to set Source 1 Roff-off Factor to 0.5\n");
	printf("Press '9' to set Source 1 Roll-off Factor to 1.0\n");
	printf("Press 'A' to set Source 1 Roll-off Factor to 2.0\n");

	printf("Press 'q' to quit\n");

	do
	{
		ch = _getch();
		ch = toupper( ch );
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
				alSourcePlay(source[1]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcePlay source 1 : ", error);
				break;
			case '4':
				alSourceStop(source[1]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceStop source 1 : ", error);
				break;
			case '5':
				alSourcef(source[0], AL_ROLLOFF_FACTOR, 0.5f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcef ROLLOFF_FACTOR 0.5 : ", error);
				break;
			case '6':
				alSourcef(source[0], AL_ROLLOFF_FACTOR, 1.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcef ROLLOFF_FACTOR 1.0 : ", error);
				break;
			case '7':
				alSourcef(source[0], AL_ROLLOFF_FACTOR, 2.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcef ROLLOFF_FACTOR 2.0 : ", error);
				break;
			case '8':
				alSourcef(source[1], AL_ROLLOFF_FACTOR, 0.5f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcef ROLLOFF_FACTOR 0.5 : ", error);
				break;
			case '9':
				alSourcef(source[1], AL_ROLLOFF_FACTOR, 1.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcef ROLLOFF_FACTOR 1.0 : ", error);
				break;
			case 'A':
				alSourcef(source[1], AL_ROLLOFF_FACTOR, 2.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcef ROLLOFF_FACTOR 2.0 : ", error);
				break;
		}
	} while (ch != 'Q');

	// Release resources
	alSourceStopv(2, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourceStopv 2 : ", error);

	alDeleteSources(2, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alDeleteSources 2 : ", error);

	return;
}