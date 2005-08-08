// OpenALDemo.cpp : Defines the entry point for the console application.
//

// Uncomment this line if you don't have the EAX 2.0 SDK installed
#define _USEEAX

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include "al\al.h"
#include "al\alc.h"
#include "al\alut.h"
#include <memory.h>
#include <windows.h>
#include <math.h>

#ifdef _USEEAX
#include "eax.h"

// EAX 2.0 GUIDs
const GUID DSPROPSETID_EAX20_ListenerProperties
				= { 0x306a6a8, 0xb224, 0x11d2, { 0x99, 0xe5, 0x0, 0x0, 0xe8, 0xd8, 0xc7, 0x22 } };

const GUID DSPROPSETID_EAX20_BufferProperties
				= { 0x306a6a7, 0xb224, 0x11d2, {0x99, 0xe5, 0x0, 0x0, 0xe8, 0xd8, 0xc7, 0x22 } };

EAXSet		eaxSet;						// EAXSet function, retrieved if EAX Extension is supported
EAXGet		eaxGet;						// EAXGet function, retrieved if EAX Extension is supported

#endif // _USEEAX


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
ALboolean	g_bEAX = AL_FALSE;			// Boolean variable to indicate presence of EAX 2.0 Extension
ALuint		g_Buffers[NUM_BUFFERS];		// Array of Buffer IDs

// AL Version Number
ALint lMajor, lMinor;

// Function prototypes
ALvoid DisplayALError(ALchar *szText, ALint errorCode);
ALboolean LoadWave(char *szWaveFile, ALuint BufferID);

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
ALvoid SourceRollOffTest(ALvoid);
ALvoid ADPCMTest(ALvoid);
ALvoid GetSourceOffsetTest(ALvoid);
ALvoid SetSourceOffsetTest(ALvoid);
ALvoid VelocityTest(ALvoid);
ALvoid DistanceModelTest(ALvoid);
ALvoid CaptureTest(ALvoid);
ALvoid CapturePlayTest(ALvoid);

/*
	Displays the AL Error string for the AL error code
*/
ALvoid DisplayALError(ALbyte *szText, ALint errorcode)
{
	printf("%s%s\n", szText, alGetString(errorcode));
}

/*
	Loads the wave file into the given Buffer ID
*/
ALboolean LoadWave(char *szWaveFile, ALuint BufferID)
{
	ALint	error;
	ALsizei size,freq;
	ALenum	format;
	ALvoid	*data;
	ALboolean loop;

	if (!szWaveFile)
		return AL_FALSE;

	alutLoadWAVFile(szWaveFile,&format,&data,&size,&freq,&loop);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		printf("Failed to load %s\n", szWaveFile);
		DisplayALError("alutLoadWAVFile : ", error);
		return AL_FALSE;
	}

	// Copy data into ALBuffer
	alBufferData(BufferID,format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alBufferData : ", error);
		return AL_FALSE;
	}

	// Unload wave file
	alutUnloadWAV(format,data,size,freq);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alutUnloadWAV : ", error);
		return AL_FALSE;
	}

	return AL_TRUE;
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
	ALbyte	ch;
	ALint	error;
	ALCcontext *Context;
	ALCdevice *Device;
	char deviceName[256];
	char *defaultDevice;
	char *deviceList;
	char *devices[12];
	int numDevices, numDefaultDevice, i;
	ALboolean bOffsetExt = AL_FALSE;
	ALboolean bNewDistModels = AL_FALSE;
	ALboolean bCaptureExt = AL_FALSE;

	ALfloat listenerPos[]={0.0,0.0,0.0};
	ALfloat listenerVel[]={0.0,0.0,0.0};
	ALfloat	listenerOri[]={0.0,0.0,-1.0, 0.0,1.0,0.0};	// Listener facing into the screen

	printf("OpenAL Test application\n\n");

	// Initialize Open AL manually

	// Open device
	strcpy(deviceName, "");
	if (alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT") == AL_TRUE) { // try out enumeration extension
		defaultDevice = (char *)alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
		if ((defaultDevice != NULL) && (strlen(defaultDevice) > 0)) { 
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
				printf("0. NULL Device\n");
				for (i = 0; i < numDevices; i++) {
					printf("%d. %s", i + 1, devices[i]);
					if (i == numDefaultDevice) { printf("  (default device)"); }
					printf("\n");
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
	}

	if (strlen(deviceName) == 0) {
		Device = alcOpenDevice(NULL); // this is supposed to select the "preferred device"
	} else {
		Device = alcOpenDevice(deviceName); // have a name from enumeration process above, so use it...
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

	// Check what version of Open AL we are using
	alcGetIntegerv(Device, ALC_MAJOR_VERSION, 1, &lMajor);
	alcGetIntegerv(Device, ALC_MINOR_VERSION, 1, &lMinor);
	printf("\nOpen AL Version %d.%d\n", lMajor, lMinor);

	// Check for all the AL 1.1 Extensions (they may be present on AL 1.0 implementations too)
	bOffsetExt = alIsExtensionPresent("al_ext_offset");
	if (bOffsetExt)
		printf("AL_EXT_OFFSET support found !\n");

	bNewDistModels = AL_TRUE;
	bNewDistModels &= alIsExtensionPresent("AL_EXT_LINEAR_DISTANCE");
	bNewDistModels &= alIsExtensionPresent("AL_EXT_EXPONENT_DISTANCE");
	if (bNewDistModels)
		printf("AL_EXT_LINEAR_DISTANCE and AL_EXT_EXPONENT_DISTANCE support found !\n");
	
	bCaptureExt = alcIsExtensionPresent(Device, "alc_EXT_capTure");
	if (bCaptureExt)
		printf("ALC_EXT_CAPTURE support found !\n");

#ifdef _USEEAX
	// Check for EAX 2.0 support
	g_bEAX = alIsExtensionPresent("EAX2.0");
	if (g_bEAX)
		printf("EAX 2.0 Extension available\n");

	eaxSet = (EAXSet)alGetProcAddress("EAXSet");
	if (eaxSet == NULL)
		g_bEAX = false;

	eaxGet = (EAXGet)alGetProcAddress("EAXGet");
	if (eaxGet == NULL)
		g_bEAX = false;
#endif // _USEEAX

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
	if ((!LoadWave("footsteps.wav", g_Buffers[0])) ||
		(!LoadWave("ding.wav", g_Buffers[1])) ||
		(!LoadWave("wave1.wav", g_Buffers[2])) ||
		(!LoadWave("wave2.wav", g_Buffers[3])) ||
		(!LoadWave("wave3.wav", g_Buffers[4])) ||
		(!LoadWave("wave4.wav", g_Buffers[5])) ||
		(!LoadWave("stereo.wav", g_Buffers[6])))
	{
		alDeleteBuffers(NUM_BUFFERS, g_Buffers);
		exit(-1);
	}

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
		printf("B Source Roll-off Test\n");
		printf("C ADPCM Play Test\n");
		printf("D Velocity Test\n");
		printf("E Get Source Offset Test\n");
		printf("F Set Source Offset Test\n");
		printf("G Distance Model Test\n");
		printf("H Capture Test\n");
		printf("I Capture and Play Test\n");
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
					printf("EAX Extensions NOT available\n");
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
				SourceRollOffTest();
				break;
			case 'C':
				ADPCMTest();
				break;
			case 'D':
				VelocityTest();
				break;
			case 'E':
				if (bOffsetExt)
					GetSourceOffsetTest();
				else
					printf("Offset Extension NOT available\n");
				break;
			case 'F':
				if (bOffsetExt)
					SetSourceOffsetTest();
				else
					printf("Offset Extension NOT available\n");
				break;
			case 'G':
				if (bNewDistModels)
					DistanceModelTest();
				else
					printf("New Distance Model Extensions NOT available\n");
				break;
			case 'H':
				if (bCaptureExt)
					CaptureTest();
				else
					printf("Capture Extension NOT available\n");
				break;
			case 'I':
				if (bCaptureExt)
					CapturePlayTest();
				else
					printf("Capture Extension NOT available\n");
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
	char	ch;

	alGetError();
	alGenSources(1,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenSources 2 : ", error);
		return;
	}
	
	printf("Buffer Test\n");
	printf("Press '1' to play buffer 0 on source 0\n");
	printf("Press '2' to play buffer 1 on source 0\n");
	printf("Press '3' to stop source 0\n");
	printf("Press 'P' to pause source 0\n");
	printf("Press 'R' to resume source 0\n");
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
				DisplayALError("alSourceStop : ", error);

			// Attach buffer 0 to source
			alSourcei(source[0], AL_BUFFER, g_Buffers[0]);
			if ((error = alGetError()) != AL_NO_ERROR)
				DisplayALError("alSourcei AL_BUFFER : ", error);
			
			// Play
			alSourcePlay(source[0]);
			if ((error = alGetError()) != AL_NO_ERROR)
				DisplayALError("alSourcePlay : ", error);
			break;

		case '2':
			// Stop source
			alSourceStop(source[0]);
			if ((error = alGetError()) != AL_NO_ERROR)
				DisplayALError("alSourceStop : ", error);

			// Attach buffer 1 to source
			alSourcei(source[0], AL_BUFFER, g_Buffers[1]);
			if ((error = alGetError()) != AL_NO_ERROR)
				DisplayALError("alSourcei AL_BUFFER : ", error);

			// Play
			alSourcePlay(source[0]);
			if ((error = alGetError()) != AL_NO_ERROR)
				DisplayALError("alSourcePlay : ", error);
			break;

		case '3':
			// Stop source
			alSourceStop(source[0]);
			if ((error = alGetError()) != AL_NO_ERROR)
				DisplayALError("alSourceStop : ", error);
			break;

		case 'P':
			alSourcePause(source[0]);
			if ((error = alGetError()) != AL_NO_ERROR)
				DisplayALError("alSourcePause : ", error);
			break;

		case 'R':
			alSourcePlay(source[0]);
			if ((error = alGetError()) != AL_NO_ERROR)
				DisplayALError("alSourcePlay : ", error);
			break;
		}
	} while (ch != 'Q');

	// Release resources
	alSourceStopv(1, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourceStopv : ", error);

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

	ALfloat source0Pos[]={ -2.0, 0.0, 2.0};	// Behind and to the left of the listener
	ALfloat source1Pos[]={ 2.0, 0.0, -2.0};	// Front and to the right of the listener

	alGetError();

	alGenSources(2,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenSources 2 : ", error);
		return;
	}

	alSourcefv(source[0],AL_POSITION,source0Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_POSITION : ", error);
	
	alSourcei(source[0],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_BUFFER buffer 0 : ", error);

	alSourcei(source[0],AL_LOOPING,AL_TRUE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_LOOPING true: ", error);

	alSourcefv(source[1],AL_POSITION,source1Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 1 AL_POSITION : ", error);

	alSourcei(source[1],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 1 AL_BUFFER buffer 1 : ", error);

	alSourcei(source[1],AL_LOOPING,AL_FALSE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 1 AL_LOOPING false: ", error);

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

	ALfloat source0Pos[]={ 0.0, 0.0, 0.0};	// Front left of the listener
	ALfloat source1Pos[]={ 2.0, 0,0, -2.0};	// Front right of the listener

	// Clear Error Code
	alGetError();

	alGenSources(2,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenSources 1 : ", error);
		return;
	}
		
	alSourcefv(source[0],AL_POSITION,source0Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_POSITION : ", error);
	
	alSourcei(source[0],AL_BUFFER, g_Buffers[0]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_BUFFER buffer 0 : ", error);

	alSourcei(source[0],AL_LOOPING,AL_FALSE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_LOOPING false : ", error);

	alSourcefv(source[1],AL_POSITION,source1Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 1 AL_POSITION : ", error);

	alSourcei(source[1],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 1 AL_BUFFER buffer 1 : ", error);

	alSourcei(source[1],AL_LOOPING,AL_FALSE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 1 AL_LOOPING false: ", error);

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

#ifdef _USEEAX

	ALint	error;
	ALuint	source[2];
	ALbyte	ch;
	ALuint	Env;
	ALint	Room;
	ALint	Occlusion;
	ALint	Obstruction;
	EAXBUFFERPROPERTIES eaxBufferProp;

	ALfloat source0Pos[]={ -2.0, 0.0, 2.0};	// Behind and to the left of the listener
	ALfloat source1Pos[]={ 2.0, 0.0,-2.0};	// Front and right of the listener

	alGetError();

	alGenSources(2,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenSources 2 : ", error);
		return;
	}
	
	alSourcefv(source[0],AL_POSITION,source0Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_POSITION : ", error);
	
	alSourcei(source[0],AL_BUFFER, g_Buffers[0]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_BUFFER buffer 0 : ", error);

	alSourcei(source[0],AL_LOOPING,AL_TRUE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_LOOPING true: ", error);

	alSourcefv(source[1],AL_POSITION,source1Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 1 AL_POSITION : ", error);

	alSourcei(source[1],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 1 AL_BUFFER buffer 1 : ", error);

	alSourcei(source[1],AL_LOOPING,AL_FALSE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 1 AL_LOOPING false: ", error);

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
					DisplayALError("eaxSet EAXLISTENER_ENVIRONMENT | EAXLISTENER_DEFERRED : ", error);
				break;

			case '6':
				Room = -10000;
				eaxSet(&DSPROPSETID_EAX20_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOM | DSPROPERTY_EAXLISTENER_DEFERRED, NULL,
					&Room, sizeof(ALint));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("eaxSet EAXLISTENER_ROOM | EAXLISTENER_DEFERRED : ", error);
				break;

			case '7':
				eaxGet(&DSPROPSETID_EAX20_BufferProperties, DSPROPERTY_EAXBUFFER_ALLPARAMETERS, source[0],
					&eaxBufferProp, sizeof(EAXBUFFERPROPERTIES));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("eaxGet EAXBUFFER_ALLPARAMETERS : ", error);
				eaxBufferProp.lOcclusion = -5000;
				eaxSet(&DSPROPSETID_EAX20_BufferProperties, DSPROPERTY_EAXBUFFER_ALLPARAMETERS | DSPROPERTY_EAXBUFFER_DEFERRED, source[0],
					&eaxBufferProp, sizeof(EAXBUFFERPROPERTIES));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("eaxSet EAXBUFFER_ALLPARAMETERS | EAXBUFFER_DEFERRED : ", error);
				break;

			case '8':
				Occlusion = 0;
				eaxSet(&DSPROPSETID_EAX20_BufferProperties, DSPROPERTY_EAXBUFFER_OCCLUSION | DSPROPERTY_EAXBUFFER_DEFERRED, source[0],
					&Occlusion, sizeof(ALint));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("eaxSet EAXBUFFER_OCCLUSION | EAXBUFFER_DEFERRED : ", error);
				break;

			case '9':
				Obstruction = -5000;
				eaxSet(&DSPROPSETID_EAX20_BufferProperties, DSPROPERTY_EAXBUFFER_OBSTRUCTION, source[1],
					&Obstruction, sizeof(ALint));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("eaxSet EAXBUFFER_OBSTRUCTION : ", error);
				break;

			case '0':
				Obstruction = 0;
				eaxSet(&DSPROPSETID_EAX20_BufferProperties, DSPROPERTY_EAXBUFFER_OBSTRUCTION, source[1],
					&Obstruction, sizeof(ALint));
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("eaxSet EAXBUFFER_OBSTRUCTION : ", error);
				break;

			case 'C':
				// Commit settings on source 0
				eaxSet(&DSPROPSETID_EAX20_BufferProperties, DSPROPERTY_EAXBUFFER_COMMITDEFERREDSETTINGS,
					source[0], NULL, 0);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("eaxSet EAXBUFFER_COMMITDEFERREDSETTINGS : ", error);

				// Commit Listener settings
				eaxSet(&DSPROPSETID_EAX20_ListenerProperties, DSPROPERTY_EAXLISTENER_COMMITDEFERREDSETTINGS,
					NULL, NULL, 0);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("eaxSet EAXLISTENER_COMMITDEFERREDSETTINGSENVIRONMENT : ", error);
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

#endif // _USEEAX

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
	ALfloat source0Pos[]={ 0.0, 0.0, -2.0};	// Immediately in front of listener
	ALuint	Buffer;
	ALint	i, lByteOffset;
	ALboolean bOffsetExt = AL_FALSE;

	bOffsetExt = alIsExtensionPresent("al_ext_offset");

	// Clear Error Code
	alGetError();

	alGenSources(1,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenSources 1 : ", error);
		return;
	}
	
	alSourcefv(source[0],AL_POSITION,source0Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_POSITION : ", error);

	alSourcei(source[0],AL_LOOPING,AL_FALSE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_LOOPING false: ", error);

	// Reduce the volume of these samples (as they much louder than the others !)
	alSourcef(source[0], AL_GAIN, 0.5f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcef 0 AL_GAIN :", error);

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

	printf("Press 'm' to set offset to 200000 bytes\n");
	printf("Press 'n' to set offset to 300000 bytes\n");
	printf("Press 'o' to set offset to 0 bytes\n");

	printf("Press 'q' to quit\n");

	printf("Source 0 not looping\r");

	buffers[0] = g_Buffers[2];
	buffers[1] = g_Buffers[3];
	buffers[2] = g_Buffers[4];
	buffers[3] = g_Buffers[5];
	buffers[4] = NULL;
	
	while (1)
	{
		if (_kbhit())
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

			case 'M':
				if (bOffsetExt)
				{
					alSourcei(source[0], AL_BYTE_OFFSET, 200000);
					if ((error = alGetError()) != AL_NO_ERROR)
						DisplayALError("alSourcei AL_BYTE_OFFSET : ", error);
				}
				break;

			case 'N':
				if (bOffsetExt)
				{
					alSourcei(source[0], AL_BYTE_OFFSET, 300000);
					if ((error = alGetError()) != AL_NO_ERROR)
						DisplayALError("alSourcei AL_BYTE_OFFSET : ", error);
				}
				break;

			case 'O':
				if (bOffsetExt)
				{
					alSourcei(source[0], AL_BYTE_OFFSET, 0);
					if ((error = alGetError()) != AL_NO_ERROR)
						DisplayALError("alSourcei AL_BYTE_OFFSET : ", error);
				}
				break;
			}

			if (ch == 'Q')
				break;
		}
		else
		{
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

			if (bOffsetExt)
				alGetSourcei(source[0], AL_BYTE_OFFSET, &lByteOffset);
			else
				lByteOffset = 0;

			printf("Offset is %d, Current Buffer is %d, %d Buffers in queue, %d Processed\r", lByteOffset, Buffer, BuffersInQueue, BuffersProcessed);
		}
	}

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
	ALfloat flPitch = 1.0f;

	alGetError();

	alGenSources(1,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenSources 1 : ", error);
		return;
	}
	
	alSourcei(source[0],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_BUFFER buffer 1 : ", error);

	alSourcei(source[0],AL_LOOPING,AL_TRUE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_LOOPING true: ", error);

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
				flPitch *= 2.0f;
				alSourcef(source[0], AL_PITCH, flPitch);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcef source 0 AL_PITCH : ", error);
				break;
			case '3':
				flPitch /= 2.0f;
				alSourcef(source[0], AL_PITCH, flPitch);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcef source 0 AL PITCH : ", error);
				break;
		}
		printf("Current Pitch is %.3f\r", flPitch);
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
					DisplayALError("alSourcei 0 AL_BUFFER buffer 6 (stereo) : ", error);

				// Set looping
				alSourcei(source[0],AL_LOOPING,bLoop);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcei 0 AL_LOOPING true: ", error);

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
					DisplayALError("alSourcei 0 AL_BUFFER buffer 0 (mono) : ", error);

				// Set 3D position
				alSourcefv(source[0],AL_POSITION,source0Pos);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcefv 0 AL_POSITION : ", error);

				// Set Looping
				alSourcei(source[0],AL_LOOPING,bLoop);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcei 0 AL_LOOPING : ", error);

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
					DisplayALError("alSourcei 0 AL_LOOPING : ", error);

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
					DisplayALError("alSourcei 0 AL_LOOPING : ", error);
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

	ALfloat source0Pos[]={ 1.0, 0.0,-1.0};
	ALfloat source1Pos[]={-1.0, 0.0,-1.0};

	alGenSources(2,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenSources 2 : ", error);
		return;
	}
	
	alSourcefv(source[0],AL_POSITION,source0Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_POSITION : ", error);
	
	alSourcei(source[0],AL_BUFFER, g_Buffers[0]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_BUFFER buffer 0 : ", error);

	alSourcei(source[0],AL_LOOPING,AL_TRUE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_LOOPING true: ", error);

	alSourcefv(source[1],AL_POSITION,source1Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 1 AL_POSITION : ", error);

	alSourcei(source[1],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 1 AL_BUFFER buffer 1 : ", error);

	alSourcei(source[1],AL_LOOPING,AL_TRUE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 1 AL_LOOPING true: ", error);

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
					DisplayALError("alSourceStop source 0 : ", error);
				break;
			case '4':
				alSourceStop(source[1]);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourceStop source 1 : ", error);
				break;
			case '5':
				alSourcef(source[0],AL_GAIN,1.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcef 0 AL_GAIN 1.0 : ", error);
				break;
			case '6':
				alSourcef(source[0],AL_GAIN,0.5f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcef 0 AL_GAIN 0.5 : ", error);
				break;
			case '7':
				alSourcef(source[0],AL_GAIN,0.25f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcef 0 AL_GAIN 0.25 : ", error);
				break;
			case '8':
				alSourcef(source[0],AL_GAIN,0.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alSourcef 0 AL_GAIN 0.0 : ", error);
				break;
			case 'A':
				alListenerf(AL_GAIN,1.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alListenerf AL_GAIN 1.0 : ", error);
				break;
			case 'B':
				alListenerf(AL_GAIN,0.5f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alListenerf AL_GAIN 0.5 : ", error);
				break;
			case 'C':
				alListenerf(AL_GAIN,0.25f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alListenerf AL_GAIN 0.25 : ", error);
				break;
			case 'D':
				alListenerf(AL_GAIN,0.0f);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alListenerf AL_GAIN 0.0 : ", error);
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

#define BSIZE		16384
#define NUMBUFFERS	4

/*
	Streaming Test
	Stream a wave file from the harddrive - using Open AL Queueing
*/
ALvoid StreamingTest(ALvoid)
{
	FILE			*fp;
	WAVE_Struct		wave;
	ALbyte			data[BSIZE];
	ALuint			SourceID;
	ALuint			BufferID[NUMBUFFERS];
	ALuint			TempBufferID;
	ALuint			ulBuffersAvailable;
	ALuint			ulUnqueueCount, ulQueueCount, ulDataSize, ulDataToRead;
	ALint			error, lProcessed, lPlaying, lLoop;

	printf("Streaming Test\n");

	fp = fopen("stereo.wav", "rb");
	if (fp == NULL)
	{
		printf("Failed to open stereo.wav\n");
		return;
	}

	// Read in WAVE Header
	if (fread(&wave, 1, sizeof(WAVE_Struct), fp) != sizeof(WAVE_Struct))
	{
		printf("Invalid wave file\n");
		fclose(fp);
		return;
	}

	// Generate a number of buffers to be used to queue data on to Source
	alGenBuffers(NUMBUFFERS, BufferID);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenBuffers : ", error);
		fclose(fp);
		return;
	}
	
	// Generate a Source
	alGenSources(1, &SourceID);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenSources 1 : ", error);
		alDeleteBuffers(NUMBUFFERS, BufferID);
		fclose(fp);
		return;
	}

	ulUnqueueCount = 0;
	ulQueueCount = 0;
	ulBuffersAvailable = NUMBUFFERS;
	ulDataSize = wave.dataSize;

	while (!_kbhit())
	{
		// Check how many Buffers have been processed
		alGetSourcei(SourceID, AL_BUFFERS_PROCESSED, &lProcessed);
		while (lProcessed)
		{
			// Unqueue the buffer
			alSourceUnqueueBuffers(SourceID, 1, &TempBufferID);

			// Update unqueue count
			if (++ulUnqueueCount == NUMBUFFERS)
				ulUnqueueCount = 0;

			// Increment buffers available
			ulBuffersAvailable++;

			// Decrement lProcessed count
			lProcessed--;
		}

		// If there is more data to read, and available buffers ... read in more data and fill the buffers !
		if ((ulDataSize) && (ulBuffersAvailable))
		{
			ulDataToRead = (ulDataSize > BSIZE) ? BSIZE : ulDataSize;
					
			fread(data, ulDataToRead, 1, fp);
			ulDataSize -= ulDataToRead;
					
			// Copy to Buffer
			alBufferData(BufferID[ulQueueCount], AL_FORMAT_STEREO16, data, ulDataToRead, wave.SamplesPerSec);

			// Queue the buffer
			alSourceQueueBuffers(SourceID, 1, &BufferID[ulQueueCount]);

			if (++ulQueueCount == NUMBUFFERS)
				ulQueueCount = 0;

			// Decrement buffers available
			ulBuffersAvailable--;
		}

		// If all the Buffers are available then we must have finished
		if (ulBuffersAvailable == NUMBUFFERS)
			break;

		// If the Source has stopped (been starved of data) it will need to be restarted
		alGetSourcei(SourceID, AL_SOURCE_STATE, &lPlaying);
		if (lPlaying != AL_PLAYING)
		{	
			// If the Source has stopped prematurely, all the processed buffers must be unqueued
			// before re-playing (otherwise they will be heard twice).
			// Any buffers queued after the Source stopped will not be processed, so they won't
			// be unqueued by this step
			alGetSourcei(SourceID, AL_BUFFERS_PROCESSED, &lProcessed);
			while (lProcessed)
			{
				alSourceUnqueueBuffers(SourceID, 1, &TempBufferID);
				if (++ulUnqueueCount == NUMBUFFERS)
					ulUnqueueCount = 0;
				ulBuffersAvailable++;
				lProcessed--;
			}

			alSourcePlay(SourceID);
		}
	}

	// Clean-up
	alSourceStop(SourceID);

	alDeleteSources(1, &SourceID);
	
	for (lLoop = 0; lLoop < NUMBUFFERS; lLoop++)
		alDeleteBuffers(1, &BufferID[lLoop]);
}


ALvoid RelativeTest(ALvoid)
{
	ALuint	source[1];
	ALint	error;
	ALbyte	ch;
	ALboolean bRelative = AL_FALSE;

	alGenSources(1, source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenSources 1 : ", error);
		return;
	}

	alSourcei(source[0],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_BUFFER buffer 0 : ", error);

	alSourcei(source[0],AL_LOOPING,AL_TRUE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_LOOPING true: ", error);

	alSourcei(source[0], AL_SOURCE_RELATIVE, AL_FALSE);	
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_SOURCE_RELATIVE AL_FALSE : ", error);

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
			alSource3f(source[0],AL_POSITION,1.0f,0.0f,0.0f);
			if ((error = alGetError()) != AL_NO_ERROR)
				DisplayALError("alSourcefv 0 AL_POSITION : ", error);
			
			alSourcePlay(source[0]);
			if ((error = alGetError()) != AL_NO_ERROR)
				DisplayALError("alSourcePlay source 0 : ", error);
			break;
		case '2':
			alListener3f(AL_POSITION,2.0f,0.0f,0.0f);
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

	// Restore Listener Position
	alListener3f(AL_POSITION,0.0f,0.0f,0.0f);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alListenerfv  AL_POSITION : ", error);

	return;
}


/*
	SourceRollOffTest
	Test the Open AL Source RollOff Factor
*/
ALvoid SourceRollOffTest()
{
	ALint	error;
	
	ALuint	source[2];
	ALbyte	ch;

	ALfloat source0Pos[]={-4.0, 0.0,  4.0};	// Behind and to the left of the listener
	ALfloat source1Pos[]={ 4.0, 0.0, -4.0};	// Front and to the right of the listener

	alGenSources(2,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenSources 2 : ", error);
		return;
	}

	alSourcefv(source[0],AL_POSITION,source0Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_POSITION : ", error);
	
	alSourcei(source[0],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_BUFFER buffer 0 : ", error);

	alSourcei(source[0],AL_LOOPING,AL_TRUE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_LOOPING true: ", error);

	alSourcefv(source[1],AL_POSITION,source1Pos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 1 AL_POSITION : ", error);

	alSourcei(source[1],AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 1 AL_BUFFER buffer 1 : ", error);

	alSourcei(source[1],AL_LOOPING,AL_TRUE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 1 AL_LOOPING false: ", error);

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


ALvoid ADPCMTest(ALvoid)
{
	ALuint uiSourceID, uiBufferID;
	ALenum eMonoADPCM = 0, eStereoADPCM = 0;
	ALsizei size,freq;
	ALenum	format;
	ALvoid	*data;
	ALboolean loop;
	char ch;

	// Check for support for ADPCM
	eMonoADPCM = alGetEnumValue("AL_FORMAT_MONO_IMA4");
	eStereoADPCM = alGetEnumValue("AL_FORMAT_STEREO_IMA4");

	if (eMonoADPCM && eStereoADPCM)
	{
		// Load footadpcm.wav
		alutLoadWAVFile("footadpcm.wav",&format,&data,&size,&freq,&loop);
		if (alGetError() != AL_NO_ERROR)
		{
			printf("Failed to load footadpcm.wav\n");
			return;
		}

		alGetError();

		// Generate an AL Buffer and an AL Source
		alGenBuffers(1, &uiBufferID);
		// Copy data to AL buffer
		alBufferData(uiBufferID,format,data,size,freq);
		// Release wave data
		alutUnloadWAV(format,data,size,freq);

		if (alGetError() != AL_NO_ERROR)
		{
			printf("Failed to generate and copy data to buffer\n");
			return;
		}

		// Generate a Source
		alGenSources(1, &uiSourceID);
		// Attach buffer to Source
		alSourcei(uiSourceID, AL_BUFFER, uiBufferID);
		// Enable Looping
		alSourcei(uiSourceID, AL_LOOPING, AL_TRUE);
		// Play Source
		alSourcePlay(uiSourceID);

		printf("Press a key to stop test\n");
		ch = _getch();

		// Stop the Source
		alSourceStop(uiSourceID);
		// Attach NULL Buffer
		alSourcei(uiSourceID, AL_BUFFER, 0);
		// Delete Source
		alDeleteSources(1, &uiSourceID);
		// Delete Buffer
		alDeleteBuffers(1, &uiBufferID);
	}
	else
	{
		printf("No ADPCM support found !\n");
	}
}


ALvoid GetSourceOffsetTest(ALvoid)
{
	ALuint Source;
	char ch, oldch;
	ALint lOffset;
	ALfloat flOffset;

	alGenSources(1, &Source);
	alSourcei(Source, AL_BUFFER, g_Buffers[6]);

	printf("Get Source Offset Test\n");
	printf("Press 1 to Play Stereo.wav\n");
	printf("Press 2 to Use AL_BYTE_OFFSET (int) to track progress\n");
	printf("Press 3 to Use AL_SAMPLE_OFFSET (int) to track progress\n");
	printf("Press 4 to Use AL_SEC_OFFSET (int) to track progress\n");
	printf("Press 5 to Use AL_BYTE_OFFSET (float) to track progress\n");
	printf("Press 6 to Use AL_SAMPLE_OFFSET (float) to track progress\n");
	printf("Press 7 to Use AL_SEC_OFFSET (float) to track progress\n");
	printf("Press Q to quit\n");
	while (1)
	{
		if (_kbhit())
		{
			ch = _getch();
			ch = toupper( ch );

			if (ch == '1')
			{
				alSourcePlay(Source);
				ch = oldch;
			}
			else if (ch == 'Q')
			{
				break;
			}

			oldch = ch;
		}
		else
		{
			switch (ch)
			{
			case '2':
			default:
				alGetSourcei(Source, AL_BYTE_OFFSET, &lOffset);
				printf("ByteOffset is %d                   \r", lOffset);
				break;
			case '3':
				alGetSourcei(Source, AL_SAMPLE_OFFSET, &lOffset);
				printf("SampleOffset is %d                 \r", lOffset);
				break;
			case '4':
				alGetSourcei(Source, AL_SEC_OFFSET, &lOffset);
				printf("SecondOffset is %d                 \r", lOffset);
				break;
			case '5':
				alGetSourcef(Source, AL_BYTE_OFFSET, &flOffset);
				printf("ByteOffset is %.3f                 \r", flOffset);
				break;
			case '6':
				alGetSourcef(Source, AL_SAMPLE_OFFSET, &flOffset);
				printf("SampleOffset is %.3f               \r", flOffset);
				break;
			case '7':
				alGetSourcef(Source, AL_SEC_OFFSET, &flOffset);
				printf("SecondOffset is %.3f               \r", flOffset);
				break;
			}
		}
	}

	alSourceStop(Source);
	alDeleteSources(1, &Source);
}


ALvoid SetSourceOffsetTest(ALvoid)
{
	ALuint Source;
	char ch;
	ALint lOffset = 0;
	ALfloat flOffset = 0.0f;
	ALint error;
	ALint iType = 0;

	alGenSources(1, &Source);
	alSourcei(Source, AL_BUFFER, g_Buffers[0]);

	printf("Set Source Offset Test\n");
	printf("Press 1 to Stop and Play 1 Footstep\n");
	printf("Press 2 to Stop and Play 2 Footsteps\n");
	printf("Press 3 to Stop and Play 3 Footsteps\n");
	printf("Press 4 to Stop and Play 4 Footsteps\n");
	printf("Press 5 to Stop and Play with bad offset\n");
	printf("Press 6 to Set Offset to play 1 Footstep\n");
	printf("Press 7 to Set Offset to play 2 Footsteps\n");
	printf("Press 8 to Set Offset to play 3 Footsteps\n");
	printf("Press 9 to Set Offset to play 4 Footsteps\n");
	printf("Press 0 to Set Offset to bad offset\n");
	printf("Press A to Play complete footstep sample\n");
	printf("Press B to Play (Looping) complete footstep sample\n");
	printf("Press C to Pause Source\n");
	printf("Press D to Stop Source\n");
	printf("Press E to Rewind Source\n");
	printf("Press F to toggle Bytes, Samples, Seconds\n");
	printf("Press Q to quit\n");

	while (1)
	{
		if (_kbhit())
		{
			ch = _getch();
			ch = toupper( ch );

			switch(ch)
			{
			case '1':
				alSourceStop(Source);
				if (iType == 0)
					alSourcei(Source, AL_BYTE_OFFSET, 216000);
				else if (iType == 1)
					alSourcei(Source, AL_SAMPLE_OFFSET, 108000);
				else if (iType == 2)
					alSourcef(Source, AL_SEC_OFFSET, 2.440f);
				alSourcePlay(Source);
				break;

			case '2':
				alSourceStop(Source);
				if (iType == 0)
					alSourcei(Source, AL_BYTE_OFFSET, 144000);
				else if (iType == 1)
					alSourcei(Source, AL_SAMPLE_OFFSET, 72000);
				else if (iType == 2)
					alSourcef(Source, AL_SEC_OFFSET, 1.640f);
				alSourcePlay(Source);
				break;

			case '3':
				alSourceStop(Source);
				if (iType == 0)
					alSourcei(Source, AL_BYTE_OFFSET, 75000);
				else if (iType == 1)
					alSourcei(Source, AL_SAMPLE_OFFSET, 37500);
				else if (iType == 2)
					alSourcef(Source, AL_SEC_OFFSET, 0.840f);
				alSourcePlay(Source);
				break;

			case '4':
				alSourceStop(Source);
				if (iType == 0)
					alSourcei(Source, AL_BYTE_OFFSET, 1000);
				else if (iType == 1)
					alSourcei(Source, AL_SAMPLE_OFFSET, 500);
				else if (iType == 2)
					alSourcef(Source, AL_SEC_OFFSET, 0.015f);
				alSourcePlay(Source);
				break;

			case '5':
				alSourceStop(Source);
				if (iType == 0)
					alSourcei(Source, AL_BYTE_OFFSET, 400000);
				else if (iType == 1)
					alSourcei(Source, AL_SAMPLE_OFFSET, 200000);
				else if (iType == 2)
					alSourcef(Source, AL_SEC_OFFSET, 5.000f);
				alSourcePlay(Source);
				break;

			case '6':
				if (iType == 0)
					alSourcei(Source, AL_BYTE_OFFSET, 216000);
				else if (iType == 1)
					alSourcei(Source, AL_SAMPLE_OFFSET, 108000);
				else if (iType == 2)
					alSourcef(Source, AL_SEC_OFFSET, 2.440f);
				break;

			case '7':
				if (iType == 0)
					alSourcei(Source, AL_BYTE_OFFSET, 144000);
				else if (iType == 1)
					alSourcei(Source, AL_SAMPLE_OFFSET, 72000);
				else if (iType == 2)
					alSourcef(Source, AL_SEC_OFFSET, 1.640f);
				break;

			case '8':
				if (iType == 0)
					alSourcei(Source, AL_BYTE_OFFSET, 75000);
				else if (iType == 1)
					alSourcei(Source, AL_SAMPLE_OFFSET, 37500);
				else if (iType == 2)
					alSourcef(Source, AL_SEC_OFFSET, 0.840f);
				break;

			case '9':
				if (iType == 0)
					alSourcei(Source, AL_BYTE_OFFSET, 1000);
				else if (iType == 1)
					alSourcei(Source, AL_SAMPLE_OFFSET, 500);
				else if (iType == 2)
					alSourcef(Source, AL_SEC_OFFSET, 0.015f);
				break;

			case '0':
				if (iType == 0)
					alSourcei(Source, AL_BYTE_OFFSET, 400000);
				else if (iType == 1)
					alSourcei(Source, AL_SAMPLE_OFFSET, 200000);
				else if (iType == 2)
					alSourcef(Source, AL_SEC_OFFSET, 5.000f);
				break;

			case 'A':
				alSourcei(Source, AL_LOOPING, AL_FALSE);
				alSourcePlay(Source);
				break;

			case 'B':
				alSourcei(Source, AL_LOOPING, AL_TRUE);
				alSourcePlay(Source);
				break;

			case 'C':
				alSourcePause(Source);
				break;

			case 'D':
				alSourceStop(Source);
				break;

			case 'E':
				alSourceRewind(Source);
				break;

			case 'F':
				iType = (++iType % 3);
				break;
			}

			if (ch == 'Q')
				break;
		}
		else
		{
			switch (iType)
			{
			case 0:
				alGetSourcei(Source, AL_BYTE_OFFSET, &lOffset);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alGetSourcei AL_BYTE_OFFSET : ", error);
				else
					printf("ByteOffset is %d                  \r", lOffset);
				break;

			case 1:
				alGetSourcei(Source, AL_SAMPLE_OFFSET, &lOffset);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alGetSourcei AL_SAMPLE_OFFSET : ", error);
				else
					printf("SampleOffset is %d                \r", lOffset);
				break;

			case 2:
				alGetSourcef(Source, AL_SEC_OFFSET, &flOffset);
				if ((error = alGetError()) != AL_NO_ERROR)
					DisplayALError("alGetSourcef AL_SEC_OFFSET : ", error);
				else
					printf("Seconds Offset is %.3f            \r", flOffset);
				break;
			}
		}
	}

	alSourceStop(Source);
	alDeleteSources(1, &Source);
}


/*
	Velocity Test
*/
ALvoid VelocityTest(ALvoid)
{	
	ALint	error;
	
	ALuint	source;
	ALbyte	ch;

	ALfloat sourcePos[] = { 1.0, 0.0, 0.0};
	ALfloat sourceVel[] = { 0.0, 0.0, 0.0};

	ALfloat listenerPos[]={ 0.0, 0.0, 0.0};
	ALfloat listenerVel[]={ 0.0, 0.0, 0.0};

	alGenSources(1,&source);

	alSourcefv(source,AL_POSITION,sourcePos);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_POSITION : \n", error);
	
	alSourcefv(source,AL_VELOCITY,sourceVel);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcefv 0 AL_VELOCITY : \n", error);

	alSourcei(source,AL_BUFFER, g_Buffers[1]);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_BUFFER buffer 0 : \n", error);

	alSourcei(source,AL_LOOPING,AL_TRUE);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcei 0 AL_LOOPING true: \n", error);

	alListenerfv(AL_POSITION, listenerPos);
	alListenerfv(AL_VELOCITY, listenerVel);

	alSourcePlay(source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourcePlay source 0 : ", error);

	printf("Velocity Test\n");
	printf("Press '1' to increase source X velocity by 50 units / second\n");
	printf("Press '2' to decrease source X velocity by 50 units / second\n");
	printf("Press '3' to increase listener X velocity by 50 units / second\n");
	printf("Press '4' to decrease listener X velocity by 50 units / second\n");
	printf("Press '5' to set Speed Of Sound to 343.3\n");
	printf("Press '6' to set Speed Of Sound to 686.6\n");
	printf("Press '7' to set Speed Of Sound to 171.6\n");
	printf("Press '8' to set Doppler Factor to 1\n");
	printf("Press '9' to set Doppler Factor to 2\n");
	printf("Press '0' to set Doppler Factor to 4\n");
	printf("Press 'q' to quit\n");

	do
	{
		printf("Source %.1f Listener %.1f\r", sourceVel[0], listenerVel[0]);

		ch = _getch();
		ch = toupper( ch );
		switch (ch)
		{
			case '1':
				sourceVel[0] += 50.0f;
				break;
			case '2':
				sourceVel[0] -= 50.0f;
				break;
			case '3':
				listenerVel[0] += 50.0f;
				break;
			case '4':
				listenerVel[0] -= 50.0f;
				break;
			case '5':
				if ((lMajor > 1) || ((lMajor == 1) && (lMinor >= 1)))
					alSpeedOfSound(343.3f);
				break;
			case '6':
				if ((lMajor > 1) || ((lMajor == 1) && (lMinor >= 1)))
					alSpeedOfSound(686.6f);
				break;
			case '7':
				if ((lMajor > 1) || ((lMajor == 1) && (lMinor >= 1)))
					alSpeedOfSound(171.6f);
				break;
			case '8':
				alDopplerFactor(1.0);
				break;
			case '9':
				alDopplerFactor(2.0);
				break;
			case '0':
				alDopplerFactor(4.0);
				break;
			default:
				break;
		}

		alSourcefv(source,AL_VELOCITY,sourceVel);
		alListenerfv(AL_VELOCITY, listenerVel);
	} while (ch != 'Q');

	// Release resources
	alSourceStopv(1, &source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourceStopv 2 : ", error);

	alDeleteSources(1, &source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alDeleteSources 2 : ", error);

	return;
}

ALvoid DistanceModelTest(ALvoid)
{
	ALuint	source[1];
	ALint	error;
	ALint	lLoop;
	
	alGenSources(1,source);
	if ((error = alGetError()) != AL_NO_ERROR)
	{
		DisplayALError("alGenSources 2 : ", error);
		return;
	}

	alSourcei(source[0], AL_BUFFER, g_Buffers[0]);
	alSourcei(source[0], AL_LOOPING, AL_TRUE);
	alSourcePlay(source[0]);
	
	printf("Distance Model Test\n");

	for (lLoop = 0; lLoop < 7; lLoop++)
	{
		switch (lLoop)
		{
		case 0:
			printf("Distance Model is AL_NONE\n");
			alDistanceModel(AL_NONE);
			break;
		case 1:
			printf("Distance Model is AL_INVERSE_DISTANCE\n");
			alDistanceModel(AL_INVERSE_DISTANCE);
			break;
		case 2:
			printf("Distance Model is AL_INVERSE_DISTANCE_CLAMPED\n");
			alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
			break;
		case 3:
			printf("Distance Model is AL_LINEAR_DISTANCE\n");
			alDistanceModel(AL_LINEAR_DISTANCE);
			break;
		case 4:
			printf("Distance Model is AL_LINEAR_DISTANCE_CLAMPED\n");
			alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
			break;
		case 5:
			printf("Distance Model is AL_EXPONENT_DISTANCE\n");
			alDistanceModel(AL_EXPONENT_DISTANCE);
			break;
		case 6:
			printf("Distance Model is AL_EXPONENT_DISTANCE_CLAMPED\n");
			alDistanceModel(AL_EXPONENT_DISTANCE_CLAMPED);
			break;
		}

		printf("Distance is 16\n");
		alSource3f(source[0], AL_POSITION, 0.0f, 0.0f, -16.0f);
		printf("Ref Distance is 8\n");
		alSourcef(source[0], AL_REFERENCE_DISTANCE, 8.0f);
		printf("Max Distance is 32\n");
		alSourcef(source[0], AL_MAX_DISTANCE, 32.0f);

		Sleep(2000);

		printf("Distance is 64\n");
		alSource3f(source[0], AL_POSITION, 0.0f, 0.0f, -64.0f);
		printf("Ref Distance is 8\n");
		alSourcef(source[0], AL_REFERENCE_DISTANCE, 8.0f);
		printf("Max Distance is 32\n");
		alSourcef(source[0], AL_MAX_DISTANCE, 32.0f);

		Sleep(2000);
	}

	printf("Restoring INVERSE_DISTANCE_CLAMPED model\n");
	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
	Sleep(50);

	alSourceStopv(1, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alSourceStopv : ", error);

	alDeleteSources(1, source);
	if ((error = alGetError()) != AL_NO_ERROR)
		DisplayALError("alDeleteSources 1 : ", error);

	return;
}

#pragma pack (push,1)
typedef struct
{
	char			szRIFF[4];
	long			lRIFFSize;
	char			szWave[4];
	char			szFmt[4];
	long			lFmtSize;
	WAVEFORMATEX	wfex;
	char			szData[4];
	long			lDataSize;
} WAVEHEADER;
#pragma pack (pop)

/*
	CaptureTest
	Records audio from the soundcard and writes a wavefile to disc with the captured data
*/
ALvoid CaptureTest(ALvoid)
{
	ALCdevice *pCaptureDevice;
	const ALCchar *szDefaultCaptureDevice;
	ALint lSamplesAvailable;
	ALchar ch;
	FILE *pFile;
	ALchar Buffer[4000];
	WAVEHEADER waveHeader;
	ALint lDataSize = 0;
	ALint lSize;

	// NOTE : This code does NOT setup the Wave Device's Audio Mixer to select a recording input
	// or recording level.

	// Get list of available Capture Devices
	const ALchar *pDeviceList = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
	if (pDeviceList)
	{
		printf("Available Capture Devices are:-\n");

		while (*pDeviceList)
		{
			printf("%s\n", pDeviceList);
			pDeviceList += strlen(pDeviceList) + 1;
		}
	}

	szDefaultCaptureDevice = alcGetString(NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
	printf("\nDefault Capture Device is '%s'\n\n", szDefaultCaptureDevice);

	pCaptureDevice = alcCaptureOpenDevice(szDefaultCaptureDevice, 22050, AL_FORMAT_STEREO16, 22050);
	if (pCaptureDevice)
	{
		printf("Opened '%s' Capture Device\n\n", alcGetString(pCaptureDevice, ALC_CAPTURE_DEVICE_SPECIFIER));

		printf("Press 1 to Start Recording\n");
		printf("Press 2 to Stop Recording\n");
		printf("Press Q to quit\n");

		pFile = fopen("recording.wav", "wb");

		sprintf(waveHeader.szRIFF, "RIFF");
		waveHeader.lRIFFSize = 0;
		sprintf(waveHeader.szWave, "WAVE");
		sprintf(waveHeader.szFmt, "fmt ");
		waveHeader.lFmtSize = sizeof(WAVEFORMATEX);		
		waveHeader.wfex.nChannels = 2;
		waveHeader.wfex.wBitsPerSample = 16;
		waveHeader.wfex.wFormatTag = WAVE_FORMAT_PCM;
		waveHeader.wfex.nSamplesPerSec = 22050;
		waveHeader.wfex.nBlockAlign = waveHeader.wfex.nChannels * waveHeader.wfex.wBitsPerSample / 8;
		waveHeader.wfex.nAvgBytesPerSec = waveHeader.wfex.nSamplesPerSec * waveHeader.wfex.nBlockAlign;
		waveHeader.wfex.cbSize = 0;
		sprintf(waveHeader.szData, "data");
		waveHeader.lDataSize = 0;

		fwrite(&waveHeader, sizeof(WAVEHEADER), 1, pFile);

		while (1)
		{
			if (_kbhit())
			{
				ch = _getch();
				ch = toupper( ch );

				switch(ch)
				{
				case '1':
					alcCaptureStart(pCaptureDevice);
					break;

				case '2':
					alcCaptureStop(pCaptureDevice);
					break;
				}

				if (ch == 'Q')
				{
					alcCaptureStop(pCaptureDevice);
					break;
				}
			}
			else
			{
				alcGetIntegerv(pCaptureDevice, ALC_CAPTURE_SAMPLES, 1, &lSamplesAvailable);
				printf("Samples available is %d\r", lSamplesAvailable);

				// When we have enough data to fill our 4000 byte buffer, grab the samples
				if (lSamplesAvailable > (4000 / waveHeader.wfex.nBlockAlign))
				{
					// Consume Samples
					alcCaptureSamples(pCaptureDevice, Buffer, 4000 / waveHeader.wfex.nBlockAlign);

					// Write the audio data to a file
					fwrite(Buffer, 4000, 1, pFile);

					// Record total amount of data recorded
					lDataSize += 4000;
				}
			}
		}

		// Check if any Samples haven't been captured yet
		alcGetIntegerv(pCaptureDevice, ALC_CAPTURE_SAMPLES, 1, &lSamplesAvailable);
		while (lSamplesAvailable)
		{
			if (lSamplesAvailable > (4000 / waveHeader.wfex.nBlockAlign))
			{
				alcCaptureSamples(pCaptureDevice, Buffer, 4000 / waveHeader.wfex.nBlockAlign);
				fwrite(Buffer, 4000, 1, pFile);
				lSamplesAvailable -= (4000 / waveHeader.wfex.nBlockAlign);
				lDataSize += 4000;
			}
			else
			{
				alcCaptureSamples(pCaptureDevice, Buffer, lSamplesAvailable);
				fwrite(Buffer, lSamplesAvailable * waveHeader.wfex.nBlockAlign, 1, pFile);
				lSamplesAvailable = 0;
				lDataSize += lSamplesAvailable * waveHeader.wfex.nBlockAlign;
			}
		}

		// Fill in Size information in Wave Header
		fseek(pFile, 4, SEEK_SET);
		lSize = lDataSize + sizeof(WAVEHEADER) - 8;
		fwrite(&lSize, 4, 1, pFile);
		fseek(pFile, 42, SEEK_SET);
		fwrite(&lDataSize, 4, 1, pFile);

		fclose(pFile);

		alcCaptureCloseDevice(pCaptureDevice);
	}
}

#define QUEUEBUFFERSIZE		2048
#define QUEUEBUFFERCOUNT	4

/*
	CapturePlayTest
	Records and plays back audio from the soundcard with EAX effects (great for recording the microphone input !)
*/
ALvoid CapturePlayTest(ALvoid)
{
	ALCdevice		*pCaptureDevice;
	const			ALCchar *szDefaultCaptureDevice;
	ALint			lSamplesAvailable;
	ALchar			ch;
	ALchar			Buffer[QUEUEBUFFERSIZE];
	ALuint			SourceID, TempBufferID;
	ALuint			BufferID[QUEUEBUFFERCOUNT];
	ALuint			ulBuffersAvailable = QUEUEBUFFERCOUNT;
	ALuint			ulUnqueueCount, ulQueueCount;
	ALint			lLoop, lFormat, lFrequency, lBlockAlignment, lProcessed, lPlaying;
	ALboolean		bPlaying = AL_FALSE;
	ALboolean		bPlay = AL_FALSE;
#ifdef _USEEAX
	ALint			lEnv, lDirect, lRoom;
#endif // _USEEAX

	// NOTE : This code does NOT setup the Wave Device's Audio Mixer to select a recording input
	// or recording level.

	// Generate a Source and QUEUEBUFFERCOUNT Buffers for Queuing
	alGetError();
	alGenSources(1, &SourceID);

	for (lLoop = 0; lLoop < QUEUEBUFFERCOUNT; lLoop++)
		alGenBuffers(1, &BufferID[lLoop]);

	if (alGetError() != AL_NO_ERROR)
	{
		printf("Failed to generate Source and / or Buffers\n");
		return;
	}

	ulUnqueueCount = 0;
	ulQueueCount = 0;

	// Get list of available Capture Devices
	const ALchar *pDeviceList = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
	if (pDeviceList)
	{
		printf("Available Capture Devices are:-\n");

		while (*pDeviceList)
		{
			printf("%s\n", pDeviceList);
			pDeviceList += strlen(pDeviceList) + 1;
		}
	}

	szDefaultCaptureDevice = alcGetString(NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
	printf("\nDefault Capture Device is '%s'\n\n", szDefaultCaptureDevice);

	// The next call can fail if the WaveDevice does not support the requested format, so the application
	// should be prepared to try different formats in case of failure

	lFormat = AL_FORMAT_MONO16;
	lFrequency = 22050;
	lBlockAlignment = 2;

	pCaptureDevice = alcCaptureOpenDevice(szDefaultCaptureDevice, lFrequency, lFormat, lFrequency);
	if (pCaptureDevice)
	{
		printf("Opened '%s' Capture Device\n\n", alcGetString(pCaptureDevice, ALC_CAPTURE_DEVICE_SPECIFIER));

		printf("Press 1 to Start Recording and Playing\n");
		printf("Press 2 to Stop Recording\n");
#ifdef _USEEAX
		printf("Press 3 to Apply EAX Reverb\n");
		printf("Press 4 to Remove EAX Reverb\n");
#endif // _USEEAX
		printf("Press Q to quit\n");

		while (1)
		{
			if (_kbhit())
			{
				ch = _getch();
				ch = toupper( ch );

				switch(ch)
				{
				case '1':
					alcCaptureStart(pCaptureDevice);
					bPlay = AL_TRUE;
					break;

				case '2':
					alcCaptureStop(pCaptureDevice);
					break;
#ifdef _USEEAX
				case '3':
					// Mute Direct path
					lDirect = -10000;
					eaxSet(&DSPROPSETID_EAX20_BufferProperties, DSPROPERTY_EAXBUFFER_DIRECT,
						NULL, &lDirect, sizeof(ALint));

					// Apply a Reverb Preset
					lEnv = EAX_ENVIRONMENT_HANGAR;
					eaxSet(&DSPROPSETID_EAX20_ListenerProperties, DSPROPERTY_EAXLISTENER_ENVIRONMENT,
						NULL, &lEnv, sizeof(ALint));

					lRoom = 0;
					eaxSet(&DSPROPSETID_EAX20_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOM,
						NULL, &lRoom, sizeof(ALint));
					break;

				case '4':
					// Un-mute Direct path
					lDirect = 0;
					eaxSet(&DSPROPSETID_EAX20_BufferProperties, DSPROPERTY_EAXBUFFER_DIRECT,
						NULL, &lDirect, sizeof(ALint));

					// Mute Reverb Preset
					lRoom = -10000;
					eaxSet(&DSPROPSETID_EAX20_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOM,
						NULL, &lRoom, sizeof(ALint));
					break;
#endif // _USEEAX
				}

				if (ch == 'Q')
				{
					alcCaptureStop(pCaptureDevice);
					break;
				}
			}
			else
			{
				alcGetIntegerv(pCaptureDevice, ALC_CAPTURE_SAMPLES, 1, &lSamplesAvailable);
				printf("Samples available is %d\r", lSamplesAvailable);

				// If the Source is (or should be) playing, get number of buffers processed
				// and check play status
				if (bPlaying)
				{
					alGetSourcei(SourceID, AL_BUFFERS_PROCESSED, &lProcessed);
					while (lProcessed)
					{
						// Unqueue the buffer
						alSourceUnqueueBuffers(SourceID, 1, &TempBufferID);

						// Update unqueue count
						if (++ulUnqueueCount == QUEUEBUFFERCOUNT)
							ulUnqueueCount = 0;

						// Increment buffers available
						ulBuffersAvailable++;

						lProcessed--;
					}

					// If the Source has stopped (been starved of data) it will need to be
					// restarted
					alGetSourcei(SourceID, AL_SOURCE_STATE, &lPlaying);
					if (lPlaying == AL_STOPPED)
						bPlay = AL_TRUE;
				}

				// When we have enough data to fill our QUEUEBUFFERSIZE byte buffer, grab the samples
				if ((lSamplesAvailable > (QUEUEBUFFERSIZE / lBlockAlignment)) && (ulBuffersAvailable))
				{
					// Consume Samples
					alcCaptureSamples(pCaptureDevice, Buffer, QUEUEBUFFERSIZE / lBlockAlignment);

					alBufferData(BufferID[ulQueueCount], lFormat, Buffer, QUEUEBUFFERSIZE, lFrequency);

					// Queue the buffer, and mark buffer as queued
					alSourceQueueBuffers(SourceID, 1, &BufferID[ulQueueCount]);

					if (++ulQueueCount == QUEUEBUFFERCOUNT)
						ulQueueCount = 0;

					// Decrement buffers available
					ulBuffersAvailable--;

					// If we need to start the Source do it now IF AND ONLY IF we have queued at least 2 buffers
					if ((bPlay) && (ulBuffersAvailable <= (QUEUEBUFFERCOUNT - 2)))
					{
						alSourcePlay(SourceID);
						bPlaying = AL_TRUE;
						bPlay = AL_FALSE;
					}
				}
			}
		}

		alcCaptureCloseDevice(pCaptureDevice);
	}
	else
	{
		printf("WaveDevice is unavailable, or does not supported the request format\n");
	}

	alSourceStop(SourceID);
	alDeleteSources(1, &SourceID);
	for (lLoop = 0; lLoop < QUEUEBUFFERCOUNT; lLoop++)
		alDeleteBuffers(1, &BufferID[lLoop]);
}