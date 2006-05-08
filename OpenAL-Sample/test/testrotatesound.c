#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <AL/al.h>
#include <AL/alut.h>


ALuint Buffer;
ALuint Source;

ALfloat SourcePos[] = { 0.0, 0.0, -1.0 };
ALfloat SourceVel[] = { 0.00, 0.00, 0.00 };

ALfloat ListenerPos[] = { 0.0, 0.0, 0.0 };
ALfloat ListenerVel[] = { 0.0, 0.0, 0.0 };
ALfloat ListenerOri[] = { 0.0f, 0.0f, -1.0f,  0.0f, 1.0f, 0.0f };


ALboolean LoadALData()
{
	Buffer = alutCreateBufferFromFile ("boom.wav");
	alGenSources(1, &Source);

	if (alGetError() != AL_NO_ERROR)
		return AL_FALSE;

	alSourcei (Source, AL_BUFFER,   Buffer   );
	alSourcef (Source, AL_PITCH,    1.0f     );
	alSourcef (Source, AL_GAIN,     1.0f     );
	alSourcefv(Source, AL_POSITION, SourcePos);
	alSourcefv(Source, AL_VELOCITY, SourceVel);
	alSourcei (Source, AL_LOOPING,  AL_TRUE  );


	if (alGetError() != AL_NO_ERROR)
		return AL_FALSE;

	return AL_TRUE;
}


void SetListenerValues()
{
	alListenerfv(AL_POSITION,    ListenerPos);
	alListenerfv(AL_VELOCITY,    ListenerVel);
	alListenerfv(AL_ORIENTATION, ListenerOri);
}

void KillALData()
{
	alDeleteBuffers(1, &Buffer);
	alDeleteSources(1, &Source);
	alutExit();
}

#define PI 3.141592653589793f

int main()
{
	int deg = 0;
	float r = 1.00f;
	float ang;
	
	alutInit(NULL,0);
	atexit(KillALData);


	if (LoadALData() == AL_FALSE)
		return 0;


	SetListenerValues();

	printf("You should hear a clock-wise rotating sound, starting from center.\n");
	alSourcePlay(Source);

	while (1) {
		alutSleep (0.1f);

		ang = (float)deg / 180.0f * PI;
		SourcePos[0] =  r * sin(ang);
		SourcePos[2] = -r * cos(ang);
		printf("deg: %03u°  pos (% f, % f, % f)\n",deg, SourcePos[0], SourcePos[1], SourcePos[2]);

		alSourcefv(Source, AL_POSITION, SourcePos);
		deg = (deg + 4) % 360;
	}

	return 0;
}
