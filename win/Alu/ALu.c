/**
 * OpenAL cross platform audio library
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
 *  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA  02111-1307, USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */

#include <math.h>
#include "OpenAL32/Include/alMain.h"
#include "AL/al.h"
#include "AL/alc.h"
#define SPEEDOFSOUNDMETRESPERSEC	(343.3f)

ALUAPI ALint ALUAPIENTRY aluF2L(ALfloat Value)
{
	double temp;

	temp=Value+(((65536.0*65536.0*16.0)+(65536.0*65536.0*8.0))*65536.0);
	return *((long *)&temp);
}

ALUAPI ALshort ALUAPIENTRY aluF2S(ALfloat Value)
{
	double temp;
	long i;

	temp=Value+(((65536.0*65536.0*16.0)+(65536.0*65536.0*8.0))*65536.0);
	i=(*((long *)&temp));
	if (i>32767)
		i=32767;
	else if (i<-32768)
		i=-32768;
	return ((short)i);
}

ALUAPI ALvoid ALUAPIENTRY aluCrossproduct(ALfloat *inVector1,ALfloat *inVector2,ALfloat *outVector)
{
	outVector[0]=(inVector1[1]*inVector2[2]-inVector1[2]*inVector2[1]);
	outVector[1]=(inVector1[2]*inVector2[0]-inVector1[0]*inVector2[2]);
	outVector[2]=(inVector1[0]*inVector2[1]-inVector1[1]*inVector2[0]);
}

ALUAPI ALfloat ALUAPIENTRY aluDotproduct(ALfloat *inVector1,ALfloat *inVector2)
{
	return (inVector1[0]*inVector2[0]+inVector1[1]*inVector2[1]+inVector1[2]*inVector2[2]);
}

ALUAPI ALvoid ALUAPIENTRY aluNormalize(ALfloat *inVector)
{
	ALfloat length,inverse_length;

	length=(ALfloat)sqrt(aluDotproduct(inVector,inVector));
	if (length != 0)
	{
		inverse_length=(1.0f/length);
		inVector[0]*=inverse_length;
		inVector[1]*=inverse_length;
		inVector[2]*=inverse_length;
	}
}

ALUAPI __inline ALvoid ALUAPIENTRY aluMatrixVector(ALfloat *vector,ALfloat matrix[3][3])
{
	ALfloat result[3];

	result[0]=vector[0]*matrix[0][0]+vector[1]*matrix[1][0]+vector[2]*matrix[2][0];
	result[1]=vector[0]*matrix[0][1]+vector[1]*matrix[1][1]+vector[2]*matrix[2][1];
	result[2]=vector[0]*matrix[0][2]+vector[1]*matrix[1][2]+vector[2]*matrix[2][2];
	memcpy(vector,result,sizeof(result));
}

ALUAPI ALvoid ALUAPIENTRY aluCalculateSourceParameters(ALuint source,ALuint numOutputChannels,ALfloat *drysend,ALfloat *wetsend,ALfloat *pitch)
{
	ALfloat ListenerOrientation[6],ListenerPosition[3],ListenerVelocity[3];
	ALfloat InnerAngle,OuterAngle,OuterGain,Angle,Distance,DryMix,WetMix;
	ALfloat Direction[3],Position[3],Velocity[3],SourceToListener[3];
	ALfloat MinVolume,MaxVolume,MinDist,MaxDist,Rolloff,RoomRolloff;
	ALfloat Pitch,Volume,PanningFB,PanningLR,ListenerGain;
	ALuint NumBufferChannels;
	ALfloat U[3],V[3],N[3];
	ALfloat DopplerFactor, DopplerVelocity;
	ALfloat flProjectedVelocity;
	ALfloat flProjectedListenerVelocity;
	ALuint DistanceModel;
	ALfloat Matrix[3][3];
	ALint HeadRelative;
	ALuint Buffer;
	ALenum Error;

	if (alIsSource(source))
	{
		//Get global properties
		alGetFloatv(AL_DOPPLER_FACTOR,&DopplerFactor);
		alGetIntegerv(AL_DISTANCE_MODEL,&DistanceModel);
		alGetFloatv(AL_DOPPLER_VELOCITY,&DopplerVelocity);
		DopplerVelocity *= SPEEDOFSOUNDMETRESPERSEC;
		
		//Get listener properties
		alGetListenerf(AL_GAIN,&ListenerGain);
		alGetListenerfv(AL_POSITION,ListenerPosition);
		alGetListenerfv(AL_VELOCITY,ListenerVelocity);
		alGetListenerfv(AL_ORIENTATION,ListenerOrientation);

		//Get source properties
		alGetSourcef(source,AL_PITCH,&Pitch);
		alGetSourcef(source,AL_GAIN,&Volume);
		alGetSourcei(source,AL_BUFFER,&Buffer);
		alGetSourcefv(source,AL_POSITION,Position);
		alGetSourcefv(source,AL_VELOCITY,Velocity);
		alGetSourcefv(source,AL_DIRECTION,Direction);
		alGetSourcef(source,AL_MIN_GAIN,&MinVolume);
		alGetSourcef(source,AL_MAX_GAIN,&MaxVolume);
		alGetSourcef(source,AL_REFERENCE_DISTANCE,&MinDist);
		alGetSourcef(source,AL_MAX_DISTANCE,&MaxDist);
		alGetSourcef(source,AL_ROLLOFF_FACTOR,&Rolloff);
		alGetSourcef(source,AL_CONE_OUTER_GAIN,&OuterGain);
		alGetSourcef(source,AL_CONE_INNER_ANGLE,&InnerAngle);
		alGetSourcef(source,AL_CONE_OUTER_ANGLE,&OuterAngle);
		alGetSourcei(source,AL_SOURCE_RELATIVE,&HeadRelative);
		
		//Set working variables
		DryMix=(ALfloat)(1.0f);
		WetMix=(ALfloat)(0.0f);
		RoomRolloff=(ALfloat)(0.0f);
		
		//Get buffer properties
		alGetBufferi(Buffer,AL_CHANNELS,&NumBufferChannels);
		//Only apply 3D calculations for mono buffers
		if (NumBufferChannels==1)
		{
			//1. Translate Listener to origin (convert to head relative)
			if (HeadRelative==AL_FALSE)
			{
				Position[0]-=ListenerPosition[0];
				Position[1]-=ListenerPosition[1];
				Position[2]-=ListenerPosition[2];
			}
			//2. Align coordinate system axes
			aluCrossproduct(&ListenerOrientation[0],&ListenerOrientation[3],U); // Right-vector
			aluNormalize(U);								// Normalized Right-vector
			memcpy(V,&ListenerOrientation[3],sizeof(V));	// Up-vector
			aluNormalize(V);								// Normalized Up-vector
			memcpy(N,&ListenerOrientation[0],sizeof(N));	// At-vector
			aluNormalize(N);								// Normalized At-vector
			Matrix[0][0]=U[0]; Matrix[0][1]=V[0]; Matrix[0][2]=-N[0];
			Matrix[1][0]=U[1]; Matrix[1][1]=V[1]; Matrix[1][2]=-N[1];
			Matrix[2][0]=U[2]; Matrix[2][1]=V[2]; Matrix[2][2]=-N[2];
			aluMatrixVector(Position,Matrix);
			//3. Calculate distance attenuation
			Distance=(ALfloat)sqrt(aluDotproduct(Position,Position));
			if (DistanceModel!=AL_NONE)
			{
				if (DistanceModel==AL_INVERSE_DISTANCE_CLAMPED)
				{
					Distance=(Distance<MinDist?MinDist:Distance);
					Distance=(Distance>MaxDist?MaxDist:Distance);
				}
				if (Distance!=0)
				{
					DryMix=((DryMix)/(1.0f+Rolloff*((Distance-MinDist)/__max(MinDist,FLT_MIN))));
					WetMix=((WetMix)/(1.0f+RoomRolloff*((Distance-MinDist)/__max(MinDist,FLT_MIN))));
				}
			}
			DryMix=__min(DryMix,MaxVolume);
			DryMix=__max(DryMix,MinVolume);
			WetMix=__min(WetMix,MaxVolume);
			WetMix=__max(WetMix,MinVolume);
			//4. Apply directional soundcones
			SourceToListener[0]=-Position[0];
			SourceToListener[1]=-Position[1];
			SourceToListener[2]=-Position[2];
			aluNormalize(Direction);
			aluNormalize(SourceToListener);
			Angle=(ALfloat)(180.0*acos(aluDotproduct(Direction,SourceToListener))/3.141592654f);
			if ((Angle>=InnerAngle)&&(Angle<=OuterAngle))
				Volume=(Volume*(1.0f+(OuterGain-1.0f)*(Angle-InnerAngle)/(OuterAngle-InnerAngle)));
			else if (Angle>OuterAngle)
				Volume=(Volume*(1.0f+(OuterGain-1.0f)                                           ));

			//5. Calculate Velocity
			flProjectedVelocity = aluDotproduct(Velocity,SourceToListener);
			flProjectedListenerVelocity = aluDotproduct(ListenerVelocity,SourceToListener);

			if (flProjectedVelocity >= DopplerVelocity)
				flProjectedVelocity = (DopplerVelocity - FLT_MIN);
			else if (flProjectedVelocity <= -DopplerVelocity)
				flProjectedVelocity = -(DopplerVelocity - FLT_MIN);

			if (flProjectedListenerVelocity >= DopplerVelocity)
				flProjectedListenerVelocity = (DopplerVelocity - FLT_MIN);
			else if (flProjectedListenerVelocity <= -DopplerVelocity)
				flProjectedListenerVelocity = -(DopplerVelocity - FLT_MIN);

			if (DopplerFactor != 0.0f)
				pitch[0] = Pitch * DopplerFactor * (DopplerVelocity - flProjectedListenerVelocity)/(DopplerVelocity - flProjectedVelocity);
			else
				pitch[0] = Pitch;

            //6. Convert normalized position into font/back panning
			if (Distance != 0.0f)
			{
				aluNormalize(Position);
				PanningLR=(0.5f+0.5f*Position[0]);
				PanningFB=(0.5f+0.5f*Position[2]);
			}
			else
			{
				PanningLR=0.5f;
				PanningFB=0.5f;
			}

			//7. Convert front/back panning into channel volumes
			switch (numOutputChannels)
			{
				case 1:
					drysend[0]=(Volume*ListenerGain*DryMix*(ALfloat)1.0f				  );	//Direct
					wetsend[0]=(Volume*ListenerGain*DryMix*(ALfloat)1.0f				  );	//Room
					break;
				case 2:
					drysend[0]=(Volume*ListenerGain*DryMix*(ALfloat)sqrt((1.0f-PanningLR)));	//FL Direct
					drysend[1]=(Volume*ListenerGain*DryMix*(ALfloat)sqrt((     PanningLR)));	//FR Direct
					wetsend[0]=(Volume*ListenerGain*WetMix*(ALfloat)sqrt((1.0f-PanningLR)));	//FL Room
					wetsend[1]=(Volume*ListenerGain*WetMix*(ALfloat)sqrt((     PanningLR)));	//FR Room
			 		break;
				default:
					break;
			}
		}
		else
		{
			//1. Stereo buffers always play from front left/front right
			switch (numOutputChannels)
			{
				case 1:
					drysend[0]=(Volume*1.0f*ListenerGain);
					wetsend[0]=(Volume*0.0f*ListenerGain);
					break;
				case 2:
					drysend[0]=(Volume*1.0f*ListenerGain);
					drysend[1]=(Volume*1.0f*ListenerGain);
					wetsend[0]=(Volume*0.0f*ListenerGain);
					wetsend[1]=(Volume*0.0f*ListenerGain);
			 		break;
				default:
					break;
			}
			pitch[0]=(ALfloat)(Pitch);
		}
		Error=alGetError();
	}
}

ALUAPI ALvoid ALUAPIENTRY aluMixData(ALvoid *context,ALvoid *buffer,ALsizei size,ALenum format)
{
	ALfloat Pitch,DrySend[OUTPUTCHANNELS],WetSend[OUTPUTCHANNELS];
	static float DryBuffer[BUFFERSIZE][OUTPUTCHANNELS];
	static float WetBuffer[BUFFERSIZE][OUTPUTCHANNELS];
	ALuint BlockAlign,BytesPerSample,BufferSize;
	ALuint DataSize,DataPosInt,DataPosFrac;
	ALint Looping,increment,State;
	ALuint Channels,Bits,Frequency;
	ALuint Buffer,fraction;
	ALCcontext *ALContext;
	ALsizei SamplesToDo;
	ALshort value,*Data;
	ALsource *ALSource;
	ALbuffer *ALBuffer;
	ALsizei i,j,k;
	ALenum Error;
	ALbufferlistitem *BufferListItem;
	ALuint loop;

	if (context)
	{
		ALContext=((ALCcontext *)context);
		SuspendContext(ALContext);
		if ((buffer)&&(size))
		{
			//Figure output format variables
			switch (format)
			{
				case AL_FORMAT_MONO8:
					BlockAlign=1;
					BytesPerSample=1;
					break;
				case AL_FORMAT_STEREO8:
					BlockAlign=2;
					BytesPerSample=1;
					break;
				case AL_FORMAT_MONO16:
					BlockAlign=2;
					BytesPerSample=2;
					break;
				case AL_FORMAT_STEREO16:
				default:
					BlockAlign=4;
					BytesPerSample=2;
					break;
			}
			//Setup variables
			ALSource=ALContext->Source;
			SamplesToDo=((size/BlockAlign)<BUFFERSIZE?(size/BlockAlign):BUFFERSIZE);
			//Clear mixing buffer
			memset(DryBuffer,0,SamplesToDo*OUTPUTCHANNELS*sizeof(ALfloat));
			memset(WetBuffer,0,SamplesToDo*OUTPUTCHANNELS*sizeof(ALfloat));
			//Actual mixing loop
			for (i=0;i<ALContext->SourceCount;i++)
			{
				j=0;
				State = ALSource->state;
				while ((State==AL_PLAYING)&&(j<SamplesToDo))
				{
                    aluCalculateSourceParameters((ALuint)ALSource->source,OUTPUTCHANNELS,DrySend,WetSend,&Pitch);
					//Get buffer info
					Buffer = ALSource->param[AL_BUFFER-AL_CONE_INNER_ANGLE].data.i;
					if (Buffer)
					{
                        ALBuffer = (ALbuffer*)ALTHUNK_LOOKUPENTRY(Buffer);

						Data = ALBuffer->data;
						Bits = (((ALBuffer->format==AL_FORMAT_MONO8)||(ALBuffer->format==AL_FORMAT_STEREO8))?8:16);
						DataSize = ALBuffer->size;
						Channels = (((ALBuffer->format==AL_FORMAT_MONO8)||(ALBuffer->format==AL_FORMAT_MONO16))?1:2);
						Frequency = ALBuffer->frequency;

						Pitch=((Pitch*Frequency)/ALContext->Frequency);
						DataSize=(DataSize/(Bits*Channels/8));
						//Get source info
						DataPosInt=ALSource->position;
						DataPosFrac=ALSource->position_fraction;
						//Figure out how many samples we can mix.
						BufferSize=(ALuint)((((ALfloat)DataSize)-(((ALfloat)DataPosInt)+(((ALfloat)DataPosFrac)/(1L<<FRACTIONBITS))))/Pitch);
						BufferSize=(BufferSize<(SamplesToDo-j)?BufferSize:(SamplesToDo-j));
						//Actual sample mixing loop
						Data+=DataPosInt*Channels;
						increment=aluF2L(Pitch*(1L<<FRACTIONBITS));
						while (BufferSize--)
						{
							k=DataPosFrac>>FRACTIONBITS; fraction=DataPosFrac&FRACTIONMASK;
							switch (Channels)
							{
								case 0x01:
                                    value=(ALshort)(((Data[k]*((1L<<FRACTIONBITS)-fraction))+(Data[k+1]*(fraction)))>>FRACTIONBITS);
									DryBuffer[j][0]+=((ALfloat)value)*DrySend[0];
									DryBuffer[j][1]+=((ALfloat)value)*DrySend[1];
									WetBuffer[j][0]+=((ALfloat)value)*WetSend[0];
									WetBuffer[j][1]+=((ALfloat)value)*WetSend[1];
									break;
								case 0x02:
                                    value=(ALshort)(((Data[k*2  ]*((1L<<FRACTIONBITS)-fraction))+(Data[k*2+2]*(fraction)))>>FRACTIONBITS);
									DryBuffer[j][0]+=((ALfloat)value)*DrySend[0];
									WetBuffer[j][0]+=((ALfloat)value)*WetSend[0];
                                    value=(ALshort)(((Data[k*2+1]*((1L<<FRACTIONBITS)-fraction))+(Data[k*2+3]*(fraction)))>>FRACTIONBITS);
									DryBuffer[j][1]+=((ALfloat)value)*DrySend[1];
									WetBuffer[j][1]+=((ALfloat)value)*WetSend[1];
									break;
								default:
									break;
							}
							DataPosFrac+=increment;
							j++;
						}
						DataPosInt+=(DataPosFrac>>FRACTIONBITS);
						DataPosFrac=(DataPosFrac&FRACTIONMASK);
						//Update source info
						ALSource->position=DataPosInt;
						ALSource->position_fraction=DataPosFrac;
					}
					//Handle looping sources
					DataPosFrac+=increment; //one more step
					DataPosInt+=(DataPosFrac>>FRACTIONBITS);
					DataPosFrac=(DataPosFrac&FRACTIONMASK);
					if ((!Buffer)||(DataPosInt>=DataSize))
					{
						//queueing
						if (ALSource->queue)
						{
							Looping = ALSource->param[AL_LOOPING-AL_CONE_INNER_ANGLE].data.i;
							if (ALSource->BuffersAddedToDSBuffer < (ALSource->BuffersInQueue-1))
							{
								BufferListItem = ALSource->queue;
								for (loop = 0; loop <= ALSource->BuffersAddedToDSBuffer; loop++)
								{
									if (BufferListItem)
									{
										if (!Looping)
											BufferListItem->bufferstate=PROCESSED;
										BufferListItem = BufferListItem->next;
									}
								}
								if (!Looping)
									ALSource->BuffersProcessed++;
								if (BufferListItem)
									ALSource->param[AL_BUFFER-AL_CONE_INNER_ANGLE].data.i=BufferListItem->buffer;
								ALSource->position=0;
								ALSource->position_fraction=0;
								ALSource->BuffersAddedToDSBuffer++;
							}
							else
							{
                                alSourceStop((ALuint)ALSource->source);
								Looping = ALSource->param[AL_LOOPING-AL_CONE_INNER_ANGLE].data.i;
								if (Looping)
								{
                                    alSourceRewind((ALuint)ALSource->source);
                                    alSourcePlay((ALuint)ALSource->source);
								}
							}
						}
						else
						{
                            alSourceStop((ALuint)ALSource->source);
							Looping = ALSource->param[AL_LOOPING-AL_CONE_INNER_ANGLE].data.i;
							if (Looping)
							{
                                alSourceRewind((ALuint)ALSource->source);
                                alSourcePlay((ALuint)ALSource->source);
							}
						}
					}
					//Get source state
					State = ALSource->state;
				}
				ALSource=ALSource->next;
			}
			//Post processing loop
			switch (format)
			{
				case AL_FORMAT_MONO8:
					for (i=0;i<(size/BytesPerSample);i++)
						((ALubyte *)buffer)[i]=aluF2S(DryBuffer[i][0]+DryBuffer[i][1]+WetBuffer[i][0]+WetBuffer[i][1])+128;
					break;
				case AL_FORMAT_STEREO8:
					for (i=0;i<(size/BytesPerSample);i++)
						((ALubyte *)buffer)[i]=aluF2S(DryBuffer[i>>1][i&1]+WetBuffer[i>>1][i&1])+128;
					break;
				case AL_FORMAT_MONO16:
					for (i=0;i<(size/BytesPerSample);i++)
						((ALshort *)buffer)[i]=aluF2S(DryBuffer[i][0]+DryBuffer[i][1]+WetBuffer[i][0]+WetBuffer[i][1]);
					break;
				case AL_FORMAT_STEREO16:
				default:
					for (i=0;i<(size/BytesPerSample);i++)
						((ALshort *)buffer)[i]=aluF2S(DryBuffer[i>>1][i&1]+WetBuffer[i>>1][i&1]);
					break;
			}
		}
		Error=alGetError();
		ProcessContext(ALContext);
	}
}

