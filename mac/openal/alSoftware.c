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
#include "globals.h" 
#include "alSource.h"
#include "alSoftware.h"
#include "alState.h"
#include "alListener.h"
#include "alBuffer.h"
#include "alError.h"

extern ALenum gDistanceModel;

ALAPI ALint ALAPIENTRY aluF2L(ALfloat Value)
{
	double temp;

	temp=Value+(((65536.0*65536.0*16.0)+(65536.0*65536.0*8.0))*65536.0);
	return *((long *)&temp);
}

ALAPI ALshort ALAPIENTRY aluF2S(ALfloat Value)
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

ALAPI ALvoid ALAPIENTRY aluCrossproduct(ALfloat *inVector1,ALfloat *inVector2,ALfloat *outVector)
{
	outVector[0]=(inVector1[1]*inVector2[2]-inVector1[2]*inVector2[1]);
	outVector[1]=(inVector1[2]*inVector2[0]-inVector1[0]*inVector2[2]);
	outVector[2]=(inVector1[0]*inVector2[1]-inVector1[1]*inVector2[0]);
}

ALAPI ALfloat ALAPIENTRY aluDotproduct(ALfloat *inVector1,ALfloat *inVector2)
{
	return (inVector1[0]*inVector2[0]+inVector1[1]*inVector2[1]+inVector1[2]*inVector2[2]);
}

ALAPI ALvoid ALAPIENTRY aluNormalize(ALfloat *inVector)
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

ALAPI ALvoid ALAPIENTRY aluMatrixVector(ALfloat *vector,ALfloat matrix[3][3])
{
	ALfloat result[3];

	result[0]=vector[0]*matrix[0][0]+vector[1]*matrix[1][0]+vector[2]*matrix[2][0];
	result[1]=vector[0]*matrix[0][1]+vector[1]*matrix[1][1]+vector[2]*matrix[2][1];
	result[2]=vector[0]*matrix[0][2]+vector[1]*matrix[1][2]+vector[2]*matrix[2][2];
	memcpy(vector,result,sizeof(result));
}

ALfloat max(ALfloat f1, ALfloat f2) 
{
    if (f1 > f2) {
        return f1;
    } else {
        return f2;
    }
}

ALAPI ALvoid ALAPIENTRY alCalculateSourceParameters(ALuint source,ALuint numOutputChannels,ALfloat *drysend,ALfloat *wetsend,ALfloat *pitch)
{
	ALfloat ListenerOrientation[6],ListenerPosition[3],ListenerVelocity[3];
	ALfloat InnerAngle,OuterAngle,OuterGain,Angle,Distance,DryMix,WetMix;
	ALfloat Direction[3],Position[3],Velocity[3],SourceToListener[3];
	ALfloat MinVolume,MaxVolume,MinDist,MaxDist,Rolloff,RoomRolloff;
	ALfloat Pitch,Volume,PanningFB,PanningLR,ListenerGain;
	ALuint NumBufferChannels;
	ALfloat U[3],V[3],N[3];
	ALfloat DopplerFactor;
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
		OuterGain = 1.0; //alGetSourcef(source,AL_CONE_OUTER_GAIN,&OuterGain);
		InnerAngle = 0.0; //alGetSourcef(source,AL_CONE_INNER_ANGLE,&InnerAngle);
		OuterAngle = 360.0; //alGetSourcef(source,AL_CONE_OUTER_ANGLE,&OuterAngle);
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
				DryMix=((DryMix)/(1.0f+Rolloff*((Distance-MinDist)/max(MinDist,FLT_MIN))));
				WetMix=((WetMix)/(1.0f+RoomRolloff*((Distance-MinDist)/max(MinDist,FLT_MIN))));
                                DryMix=((DryMix<=MaxVolume)?DryMix:MaxVolume);
                                DryMix=((DryMix>=MinVolume)?DryMix:MinVolume);
                                WetMix=((WetMix<=MaxVolume)?WetMix:MaxVolume);
                                WetMix=((WetMix>=MinVolume)?WetMix:MinVolume);
			}
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
			//5. Calculate differential velocity
			Velocity[0]-=ListenerVelocity[0];
			Velocity[1]-=ListenerVelocity[1];
			Velocity[2]-=ListenerVelocity[2];
			aluMatrixVector(Velocity,Matrix);		
			//6. Calculate doppler
			if ((DopplerFactor!=0.0f)&&(Distance!=0.0f))
				pitch[0]=(ALfloat)((Pitch*DopplerFactor)/(1.0+(aluDotproduct(Velocity,Position)/(343.0f*Distance))));
			else
				pitch[0]=(ALfloat)((Pitch			   )/(1.0+(aluDotproduct(Velocity,Position)/(343.0f))));
			//7. Convert normalized position into font/back panning
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

			//8. Convert front/back panning into channel volumes
			switch (numOutputChannels)
			{
				case 1:
					drysend[0]=(Volume*ListenerGain*DryMix*(ALfloat)1.0f								  );	//Direct
					wetsend[0]=(Volume*ListenerGain*DryMix*(ALfloat)1.0f								  );	//Room
					break;																					
				case 2:
					drysend[0]=(Volume*ListenerGain*DryMix*(ALfloat)sqrt((1.0f-PanningLR)));	//FL Direct
					drysend[1]=(Volume*ListenerGain*DryMix*(ALfloat)sqrt((     PanningLR)));	//FR Direct
					wetsend[0]=(Volume*ListenerGain*WetMix*(ALfloat)sqrt((1.0f-PanningLR)));	//FL Room
					wetsend[1]=(Volume*ListenerGain*WetMix*(ALfloat)sqrt((     PanningLR)));	//FR Room
			 		break;
				case 3:
					drysend[0]=(Volume*ListenerGain*DryMix*(ALfloat)sqrt((1.0f-PanningLR)));	//FL Direct
					drysend[1]=(Volume*ListenerGain*DryMix*(ALfloat)sqrt((     PanningLR)));	//FR Direct
					drysend[2]=(Volume*ListenerGain*DryMix*(ALfloat)sqrt((     PanningFB)));	//SUR Direct
					wetsend[0]=(Volume*ListenerGain*WetMix*(ALfloat)sqrt((1.0f-PanningLR)));	//FL Room
					wetsend[1]=(Volume*ListenerGain*WetMix*(ALfloat)sqrt((     PanningLR)));	//FR Room
					break;
				case 4:
					drysend[0]=(Volume*ListenerGain*DryMix*(ALfloat)sqrt((1.0f-PanningLR)*(1.0f-PanningFB))); //FL Direct
					drysend[1]=(Volume*ListenerGain*DryMix*(ALfloat)sqrt((     PanningLR)*(1.0f-PanningFB))); //FR Direct
					drysend[2]=(Volume*ListenerGain*DryMix*(ALfloat)sqrt((1.0f-PanningLR)*(     PanningFB))); //RL Direct
					drysend[3]=(Volume*ListenerGain*DryMix*(ALfloat)sqrt((     PanningLR)*(     PanningFB))); //RR Direct
					wetsend[0]=(Volume*ListenerGain*WetMix*(ALfloat)sqrt((1.0f-PanningLR)));	//FL Room
					wetsend[1]=(Volume*ListenerGain*WetMix*(ALfloat)sqrt((     PanningLR)));	//FR Room
					break;
				case 5:
					if (PanningLR<0.5f)
					{
						drysend[0]=(Volume*ListenerGain*DryMix*(ALfloat)sqrt((1.0f-2.0f*PanningLR)*(1.0f-PanningFB)));//FL Direct
						drysend[1]=(Volume*ListenerGain*DryMix*(ALfloat)0.0f                                        );//FR Direct
						drysend[2]=(Volume*ListenerGain*DryMix*(ALfloat)sqrt((     2.0f*PanningLR)*(1.0f-PanningFB)));//FC Direct
						drysend[3]=(Volume*ListenerGain*DryMix*(ALfloat)sqrt((1.0f-     PanningLR)*(     PanningFB)));//RL Direct
						drysend[4]=(Volume*ListenerGain*DryMix*(ALfloat)sqrt((          PanningLR)*(     PanningFB)));//RR Direct
					}
					else
					{
						drysend[0]=(Volume*ListenerGain*DryMix*(ALfloat)0.0f); //FL Direct
						drysend[1]=(Volume*ListenerGain*DryMix*(ALfloat)sqrt((2.0f*PanningLR-1.0f)*(1.0f-PanningFB)));//FR Direct
						drysend[2]=(Volume*ListenerGain*DryMix*(ALfloat)sqrt((2.0f-2.0f*PanningLR)*(1.0f-PanningFB)));//FC Direct
						drysend[3]=(Volume*ListenerGain*DryMix*(ALfloat)sqrt((1.0f-PanningLR)*(     PanningFB)));//RL Direct
						drysend[4]=(Volume*ListenerGain*DryMix*(ALfloat)sqrt((PanningLR)*(     PanningFB)));//RR Direct
					}
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
				case 3:
					drysend[0]=(Volume*1.0f*ListenerGain);
					drysend[1]=(Volume*1.0f*ListenerGain);
					drysend[2]=(Volume*0.0f*ListenerGain);
					wetsend[0]=(Volume*0.0f*ListenerGain);
					wetsend[1]=(Volume*0.0f*ListenerGain);
					break;
				case 4:
					drysend[0]=(Volume*1.0f*ListenerGain);
					drysend[1]=(Volume*1.0f*ListenerGain);
					drysend[2]=(Volume*0.0f*ListenerGain);
					drysend[3]=(Volume*0.0f*ListenerGain);
					wetsend[0]=(Volume*0.0f*ListenerGain);
					wetsend[1]=(Volume*0.0f*ListenerGain);
					break;
				case 5:
					drysend[0]=(Volume*1.0f*ListenerGain);
					drysend[1]=(Volume*1.0f*ListenerGain);
					drysend[2]=(Volume*0.0f*ListenerGain);
					drysend[3]=(Volume*0.0f*ListenerGain);
					drysend[4]=(Volume*0.0f*ListenerGain);
					wetsend[0]=(Volume*1.0f*ListenerGain);
					wetsend[1]=(Volume*1.0f*ListenerGain);
					break;
				default:
					break;
			}
			pitch[0]=(ALfloat)(Pitch);
		}
		Error=alGetError();
	}
}
