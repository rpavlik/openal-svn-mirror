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
 
#include <CoreAudio/CoreAudio.h>
#include <vecLib/vecLib.h>
#include <stdlib.h>
#include "mutex.h"
#include "globals.h"
#include "alSource.h"
#include "alSoftware.h"
#include "sm_ca.h"

#ifdef VORBIS_EXTENSION
#include "vorbisrtn.h"
#endif

#define volume_threshold 0.005

typedef struct {
    AudioDeviceID	deviceW;		/* the device ID */
    void *		deviceWBuffer;
    UInt32		deviceWBufferSize;	/* Buffer size of the audio device */
    AudioBufferList*	deviceWBufferList;
	AudioStreamBasicDescription	*deviceFormats;	/* format of the default device */
    AudioStreamBasicDescription	streamFormat;	/* format of the default device */
} globalVars, *globalPtr; 
static globalVars	libGlobals; /* my globals */
static unsigned int	alWriteFormat;	/* format of data from AL*/
static unsigned int	nativePreferedBuffSize,nativePreferedBuffSize_X_2,originalPreferedBuffSize; 
static int ratio;
int iBufferNum=0;
Boolean INITIALIZED=FALSE;
Float32 *mixbuf;
Float32 *mixbuf2,*mixbuf3,*mixbuf4,*mixbuf5,*mixbuf6; //to do less SRC's per IOProc
Float32 *volbuf;
Float32 prev_left,prev_right;

void smSourceInit(unsigned int source)
{
	gSource[source].readOffset = 0;
	gSource[source].uncompressedReadOffset = 0;
	gSource[source].uncompressedBufferOffset=0;
	if (gSource[source].pCompHdr != NULL) {
		free(gSource[source].pCompHdr);
		gSource[source].pCompHdr = NULL;
	}
}

void smSourceKill(unsigned int source)
{
	gSource[source].srcBufferNum = AL_MAXBUFFERS + 1;
	gSource[source].samplePtr=NULL;
}

//No SoundManager Functions Below Here
void mix_with_same_sample_rates(Float32 *outDataPtr,SInt16 *inDataPtr16,UInt8 *inDataPtr8,Float32 vol_gainL,Float32 vol_gainR,unsigned int numFrames,int numChannels,ALuint *readOffset,ALsizei size,Boolean isOutput)
{
	register unsigned int count,idx2,amount_to_write,amount_to_offset;
	
	amount_to_write = ((numFrames)<<1);
	if((int)amount_to_write<0){
		numFrames=0;
		amount_to_write=0;
	}
	if(amount_to_write>nativePreferedBuffSize_X_2) amount_to_write=nativePreferedBuffSize_X_2;
	amount_to_offset = (amount_to_write>>1);
	
	if(isOutput==FALSE){ //so we can get an extra sample on the end for linear interp
		numFrames+=1;
		amount_to_write+=2;
	}
	
	if(numChannels==1) amount_to_write = (amount_to_write>>1);
	
	switch(alWriteFormat){
		case AL_FORMAT_STEREO16:
			vol_gainL*=0.000030519f; //same as divby 32767
			vol_gainR*=0.000030519f;
			for(count=0;count<numFrames;count++){ //create buffers for vector ops
					idx2 = (count<<1);
					if(*readOffset+(idx2<<1)<size)
						*(mixbuf+idx2) = ((Float32)(*(inDataPtr16+(count<<1))));
					else
						*(mixbuf+idx2) = 0.0;
					*(volbuf+idx2) = vol_gainL;
					if(*readOffset+((idx2+1)<<1)<size)
						*(mixbuf+idx2+1) = ((Float32)(*(inDataPtr16+(count<<1)+1)));
					else
						*(mixbuf+idx2+1) = 0.0;
					*(volbuf+idx2+1) = vol_gainR;
				if(*readOffset+(idx2<<1)>=size){
					*readOffset=size;
					break;
				}
			}
			vmul(mixbuf,1,volbuf,1,mixbuf,1,amount_to_write);
			
			// !!! FIXME: Fatal problems if output stream wants to be mono
			if (numChannels == 2)  // common scenario, faster.
			{
				vadd(mixbuf,1,outDataPtr,1,outDataPtr,1,amount_to_write);
			}
			else
			{
				vadd(mixbuf,2,outDataPtr,numChannels,outDataPtr,numChannels,amount_to_write);
				vadd(mixbuf+1,2,outDataPtr+1,numChannels,outDataPtr+1,numChannels,amount_to_write);
			}
			*readOffset += (amount_to_offset<<2); //update source ptr
			break;
		case AL_FORMAT_MONO16:
			vol_gainL*=0.000030519f; //same as divby 32767
			vol_gainR*=0.000030519f;
			
			for(count=0;count<numFrames;count++){ //create buffers for vector ops
				idx2 = (count<<1);
				if(*readOffset+idx2<size)
					*(mixbuf+idx2) = ((Float32)(*(inDataPtr16+count)));
				else
					*(mixbuf+idx2) = 0.0;
				*(volbuf+idx2) = vol_gainL;
				*(mixbuf+idx2+1) = *(mixbuf+idx2);
				*(volbuf+idx2+1) = vol_gainR;
				if(*readOffset+idx2>=size){
					*readOffset=size;
					break;
				}
			}
			vmul(mixbuf,1,volbuf,1,mixbuf,1,amount_to_write);
			
			// !!! FIXME: Fatal problems if output stream wants to be mono
			if (numChannels == 2)  // common scenario, faster.
			{
				vadd(mixbuf,1,outDataPtr,1,outDataPtr,1,amount_to_write);
			}
			else
			{
				vadd(mixbuf,2,outDataPtr,numChannels,outDataPtr,numChannels,amount_to_write);
				vadd(mixbuf+1,2,outDataPtr+1,numChannels,outDataPtr+1,numChannels,amount_to_write);
			}
			*readOffset += (amount_to_offset<<1); //update source ptr
			break;
		case AL_FORMAT_STEREO8:
			vol_gainL*=0.007874016f; //same as divby 127
			vol_gainR*=0.007874016f;

			for(count=0;count<numFrames;count++){ //create buffers for vector ops
					idx2 = (count<<1);
					if(*readOffset+idx2<size)
						*(mixbuf+idx2) = ((Float32)(*(inDataPtr8+idx2))-128.0);
					else
						*(mixbuf+idx2) = 0.0;
					*(volbuf+idx2) = vol_gainL;
					if(*readOffset+idx2+1<size)
						*(mixbuf+idx2+1) = ((Float32)(*(inDataPtr8+idx2+1))-128.0);
					else
						*(mixbuf+idx2+1) = 0.0;
					*(volbuf+idx2+1) = vol_gainR;
				if(*readOffset+idx2>=size){
					*readOffset=size;
					break;
				}
			}
			vmul(mixbuf,1,volbuf,1,mixbuf,1,amount_to_write);
			
			// !!! FIXME: Fatal problems if output stream wants to be mono
			if (numChannels == 2)  // common scenario, faster.
			{
				vadd(mixbuf,1,outDataPtr,1,outDataPtr,1,amount_to_write);
			}
			else
			{
				vadd(mixbuf,2,outDataPtr,numChannels,outDataPtr,numChannels,amount_to_write);
				vadd(mixbuf+1,2,outDataPtr+1,numChannels,outDataPtr+1,numChannels,amount_to_write);
			}
			
			*readOffset += ((amount_to_offset)<<1); //update source ptr
			break;
		case AL_FORMAT_MONO8:
			vol_gainL*=0.007874016f; //same as divby 127
			vol_gainR*=0.007874016f;
			
			for(count=0;count<numFrames;count++){ //create buffers for vector ops
				idx2 = (count<<1);
				if(*readOffset+count<size)
					*(mixbuf+idx2) = ((Float32)(*(inDataPtr8+count))-128.0);
				else
					*(mixbuf+idx2) = 0.0;
				*(volbuf+idx2) = vol_gainL;
				*(mixbuf+idx2+1) = *(mixbuf+idx2);
				*(volbuf+idx2+1) = vol_gainR;
				if(*readOffset+count>=size){
					*readOffset=size;
					break;
				}
			}
			vmul(mixbuf,1,volbuf,1,mixbuf,1,amount_to_write);
			
			// !!! FIXME: Fatal problems if output stream wants to be mono
			if (numChannels == 2)  // common scenario, faster.
			{
				vadd(mixbuf,1,outDataPtr,1,outDataPtr,1,amount_to_write);
			}
			else
			{
				vadd(mixbuf,2,outDataPtr,numChannels,outDataPtr,numChannels,amount_to_write);
				vadd(mixbuf+1,2,outDataPtr+1,numChannels,outDataPtr+1,numChannels,amount_to_write);
			}
			
			*readOffset += amount_to_offset; //update source ptr
			break;
		default:
			exit(1);
			break;
	}
}

void mix_with_same_sample_rates_w_pitch(Float32 *outDataPtr,SInt16 *inDataPtr16,UInt8 *inDataPtr8,Float32 vol_gainL,Float32 vol_gainR, ALfloat Pitch,unsigned int numFrames,int numChannels,ALuint *readOffset,ALsizei size,Boolean isOutput)
{
	register unsigned int count,pitch_count,idx2,idx,amount_to_write,amount_to_offset;
	register Float32 val1L,val2L,pitch_count_float;
	
	amount_to_write = ((numFrames)<<1);
	if((int)amount_to_write<0){
		numFrames=0;
		amount_to_write=0;
	}
	if(amount_to_write>nativePreferedBuffSize_X_2) amount_to_write=nativePreferedBuffSize_X_2;
	amount_to_offset = (amount_to_write>>1);
	
	if(isOutput==FALSE){ //so we can get an extra sample on the end for linear interp
		numFrames+=1;
		amount_to_write+=2;
	}
	
	if(numChannels==1) amount_to_write = (amount_to_write>>1);
	
	switch(alWriteFormat){
		case AL_FORMAT_STEREO16:
			vol_gainL*=0.000030519f; //same as divby 32767
			vol_gainR*=0.000030519f;
			for(count=0,pitch_count_float=0.0;count<numFrames;count++,pitch_count_float+=Pitch){ //create buffers for vector ops
					pitch_count = (int)(pitch_count_float);
					idx2 = (count<<1);
					idx = (pitch_count<<1);
					if(*readOffset+(idx<<1)<size){
						val1L = ((Float32)(*(inDataPtr16+idx)));
						if(*readOffset+((idx+1)<<1)<size) val2L = ((Float32)(*(inDataPtr16+idx+2)));
						else val2L = 0.0;
					}
					else{
						val1L = 0.0;
						val2L = 0.0;
					}
					*(mixbuf+idx2) = val1L + (pitch_count_float - (float)pitch_count)*(val2L - val1L);
					*(volbuf+idx2) = vol_gainL;
					if(*readOffset+((idx+1)<<1)<size){
						val1L = ((Float32)(*(inDataPtr16+idx+1)));
						if(*readOffset+((idx+3)<<1)<size) val2L = ((Float32)(*(inDataPtr16+idx+3)));
						else val2L = 0.0;
					}
					else{
						val1L = 0.0;
						val2L = 0.0;
					}
					*(mixbuf+idx2+1) = val1L + (pitch_count_float - (float)pitch_count)*(val2L - val1L);
					*(volbuf+idx2+1) = vol_gainR;
				if(*readOffset+(idx<<1)>=size){
					*readOffset=size;
					break;
				}
			}
			vmul(mixbuf,1,volbuf,1,mixbuf,1,amount_to_write);
			
			// !!! FIXME: Fatal problems if output stream wants to be mono
			if (numChannels == 2)  // common scenario, faster.
			{
				vadd(mixbuf,1,outDataPtr,1,outDataPtr,1,amount_to_write);
			}
			else
			{
				vadd(mixbuf,2,outDataPtr,numChannels,outDataPtr,numChannels,amount_to_write);
				vadd(mixbuf+1,2,outDataPtr+1,numChannels,outDataPtr+1,numChannels,amount_to_write);
			}
			*readOffset += ((int)(0.5+Pitch*(ALfloat)(amount_to_offset))<<2); //update source ptr
			break;
		case AL_FORMAT_MONO16:
			vol_gainL*=0.000030519f; //same as divby 32767
			vol_gainR*=0.000030519f;
			
			for(count=0,pitch_count_float=0.0;count<numFrames;count++,pitch_count_float+=Pitch){ //create buffers for vector ops
				pitch_count = (int)(pitch_count_float);
				idx2 = (count<<1);
				if(*readOffset+(pitch_count<<1)<size){
					val1L = ((Float32)(*(inDataPtr16+pitch_count)));
					if(*readOffset+((pitch_count+1)<<1)<size) val2L = ((Float32)(*(inDataPtr16+pitch_count+1)));
					else val2L = 0.0;
				}
				else{
					val1L = 0.0;
					val2L = 0.0;
				}
				*(mixbuf+idx2) = val1L + (pitch_count_float - (float)pitch_count)*(val2L - val1L);
				*(volbuf+idx2) = vol_gainL;
				*(mixbuf+idx2+1) = *(mixbuf+idx2);
				*(volbuf+idx2+1) = vol_gainR;
				if(*readOffset+(pitch_count<<1)>=size){
					*readOffset=size;
					break;
				}
			}
			vmul(mixbuf,1,volbuf,1,mixbuf,1,amount_to_write);
			
			// !!! FIXME: Fatal problems if output stream wants to be mono
			if (numChannels == 2)  // common scenario, faster.
			{
				vadd(mixbuf,1,outDataPtr,1,outDataPtr,1,amount_to_write);
			}
			else
			{
				vadd(mixbuf,2,outDataPtr,numChannels,outDataPtr,numChannels,amount_to_write);
				vadd(mixbuf+1,2,outDataPtr+1,numChannels,outDataPtr+1,numChannels,amount_to_write);
			}
			*readOffset += ((int)(0.5+Pitch*(ALfloat)(amount_to_offset))<<1); //update source ptr
			
			break;
		case AL_FORMAT_STEREO8:
			vol_gainL*=0.007874016f; //same as divby 127
			vol_gainR*=0.007874016f;

			for(count=0,pitch_count_float=0.0;count<numFrames;count++,pitch_count_float+=Pitch){ //create buffers for vector ops
				pitch_count = (int)(pitch_count_float);
				idx2 = (count<<1);
				idx = (pitch_count<<1);
				if(*readOffset+idx<size){
					val1L = ((Float32)(*(inDataPtr8+idx))-128.0);
					if(*readOffset+(idx+2)<size) val2L = ((Float32)(*(inDataPtr8+idx+2))-128.0);
					else val2L = 0.0;
				}
				else{
					val1L = 0.0;
					val2L = 0.0;
				}
				*(mixbuf+idx2) = val1L + (pitch_count_float - (float)pitch_count)*(val2L - val1L);
				*(volbuf+idx2) = vol_gainL;
				if(*readOffset+idx+1<size){
					val1L = ((Float32)(*(inDataPtr8+idx+1))-128.0);
					if(*readOffset+idx+3<size) val2L = ((Float32)(*(inDataPtr8+idx+3))-128.0);
					else val2L = 0.0;
				}
				else{
					val1L = 0.0;
					val2L = 0.0;
				}
				*(mixbuf+idx2+1) = val1L + (pitch_count_float - (float)pitch_count)*(val2L - val1L);
				*(volbuf+idx2+1) = vol_gainR;
				if(*readOffset+idx>=size){
					*readOffset=size;
					break;
				}
			}
			vmul(mixbuf,1,volbuf,1,mixbuf,1,amount_to_write);
			
			// !!! FIXME: Fatal problems if output stream wants to be mono
			if (numChannels == 2)  // common scenario, faster.
			{
				vadd(mixbuf,1,outDataPtr,1,outDataPtr,1,amount_to_write);
			}
			else
			{
				vadd(mixbuf,2,outDataPtr,numChannels,outDataPtr,numChannels,amount_to_write);
				vadd(mixbuf+1,2,outDataPtr+1,numChannels,outDataPtr+1,numChannels,amount_to_write);
			}
			
			*readOffset += ((int)(0.5+Pitch*(ALfloat)(amount_to_offset))<<1); //update source ptr
			break;
		case AL_FORMAT_MONO8:
			vol_gainL*=0.007874016f; //same as divby 127
			vol_gainR*=0.007874016f;
			
			for(count=0,pitch_count_float=0.0;count<numFrames;count++,pitch_count_float+=Pitch){ //create buffers for vector ops
				pitch_count = (int)(pitch_count_float);
				idx2 = (count<<1);
				if(*readOffset+pitch_count<size){
					val1L = ((Float32)(*(inDataPtr8+pitch_count))-128.0);
					if(*readOffset+pitch_count+1<size) val2L = ((Float32)(*(inDataPtr8+pitch_count+1))-128.0);
					val2L = 0.0;
				}
				else{
					val1L = 0.0;
					val2L = 0.0;
				}
				*(mixbuf+idx2) = val1L + (pitch_count_float - (float)pitch_count)*(val2L - val1L);
				*(volbuf+idx2) = vol_gainL;
				*(mixbuf+idx2+1) = *(mixbuf+idx2);
				*(volbuf+idx2+1) = vol_gainR;
				if(*readOffset+pitch_count>=size){
					*readOffset=size;
					break;
				}
			}
			vmul(mixbuf,1,volbuf,1,mixbuf,1,amount_to_write);
			
			// !!! FIXME: Fatal problems if output stream wants to be mono
			if (numChannels == 2)  // common scenario, faster.
			{
				vadd(mixbuf,1,outDataPtr,1,outDataPtr,1,amount_to_write);
			}
			else
			{
				vadd(mixbuf,2,outDataPtr,numChannels,outDataPtr,numChannels,amount_to_write);
				vadd(mixbuf+1,2,outDataPtr+1,numChannels,outDataPtr+1,numChannels,amount_to_write);
			}
			
			*readOffset += (int)(0.5+Pitch*(ALfloat)(amount_to_offset)); //update source ptr
			break;
		default:
			exit(1);
			break;
	}
}

void parse_through_buffer_w_out_playing(ALfloat Pitch,unsigned int numFrames,ALuint *readOffset,ALsizei size)
{
	register unsigned int amount_to_write,amount_to_offset;
	
	amount_to_write = ((numFrames)<<1);
	if((int)amount_to_write<0){
		numFrames=0;
		amount_to_write=0;
	}
	if(amount_to_write>nativePreferedBuffSize_X_2) amount_to_write=nativePreferedBuffSize_X_2;
	amount_to_offset = (amount_to_write>>1);
	
	if(Pitch!=1.0){
		switch(alWriteFormat){
			case AL_FORMAT_STEREO16:
				*readOffset += ((int)(0.5+Pitch*(ALfloat)(amount_to_offset))<<2); //update source ptr
				break;
			case AL_FORMAT_MONO16:
				*readOffset += ((int)(0.5+Pitch*(ALfloat)(amount_to_offset))<<1); //update source ptr
				break;
			case AL_FORMAT_STEREO8:
				*readOffset += ((int)(0.5+Pitch*(ALfloat)(amount_to_offset))<<1); //update source ptr
				break;
			case AL_FORMAT_MONO8:
				*readOffset += (int)(0.5+Pitch*(ALfloat)(amount_to_offset)); //update source ptr
				break;
			default:
				exit(1);
				break;
		}
	}
	else{
		switch(alWriteFormat){
			case AL_FORMAT_STEREO16:
				*readOffset += (amount_to_offset<<2); //update source ptr
				break;
			case AL_FORMAT_MONO16:
				*readOffset += (amount_to_offset<<1); //update source ptr
				break;
			case AL_FORMAT_STEREO8:
				*readOffset += (amount_to_offset<<1); //update source ptr
				break;
			case AL_FORMAT_MONO8:
				*readOffset += amount_to_offset; //update source ptr
				break;
			default:
				exit(1);
				break;
		}
	}
}

void SRC_and_mix(Float32 *outDataPtr,Float32 *inbuf,int R,int numChannels)
{
	register unsigned int count,i,idx2,idx,idx3,amount_to_write,numFrames;
	register Float32 val1L=0.0,val1R=0.0,val2L=0.0,val2R=0.0,linear_idx,one_over_ratio = 1.0f/(Float32)R;
	
	memset( mixbuf, 0, (nativePreferedBuffSize_X_2)*sizeof(Float32)+2*sizeof(Float32)); //clear input buffer
	
	numFrames = nativePreferedBuffSize/R;
	
	amount_to_write = nativePreferedBuffSize_X_2;
	
	if (numChannels == 2){
		for(count=0;count<numFrames;count++){
			idx2 = (count<<1);
			idx3 = idx2*R;
			*(mixbuf+idx3) = *(inbuf+idx2);
			*(mixbuf+idx3+1) = *(inbuf+idx2+1);
			val1L = *(inbuf+idx2);
			val1R = *(inbuf+idx2+1);
			val2L = *(inbuf+idx2+2);
			val2R = *(inbuf+idx2+3);
			for (i = 1,linear_idx=one_over_ratio; i<R; i++){
				idx=(int)linear_idx;
				*(mixbuf+idx3+(i<<1)) = val1L + (linear_idx-(float)idx)*(val2L-val1L);//linearly interp
				*(mixbuf+idx3+(i<<1)+1) = val1R + (linear_idx-(float)idx)*(val2R-val1R);//linearly interp
				linear_idx+=one_over_ratio;
			}
		}
		vadd(mixbuf,1,outDataPtr,1,outDataPtr,1,amount_to_write);
	}
	else{
		for(count=0;count<numFrames;count++){
			idx3 = count*R;
			*(mixbuf+idx3) = *(inbuf+count);
			val1L = *(inbuf+count);
			val2L = *(inbuf+count+1);
			for (i = 1,linear_idx=one_over_ratio; i<R; i++){
				idx=(int)linear_idx;
				
				*(mixbuf+idx3+(i<<1)) = val1L + (linear_idx-(float)idx)*(val2L-val1L);//linearly interp
				*(mixbuf+idx3+(i<<1)+1) = val1R + (linear_idx-(float)idx)*(val2R-val1R);//linearly interp
				
				linear_idx+=one_over_ratio;
			}
		}
		vadd(mixbuf,1,outDataPtr,1,outDataPtr,1,amount_to_write>>1);
	}
}

void IOProc_Service(unsigned int source,unsigned int *iBufferNum,unsigned int *ratio,ALuint *readOffset,float *one_over_ratio,int *gbufsize)
{
	ALboolean bNewQE;
	QueueEntry *pQE;
	
	bNewQE = AL_FALSE;
#ifdef VORBIS_EXTENSION
	if(gBuffer[*iBufferNum].format == AL_FORMAT_VORBIS_EXT){
		if(gSource[source].uncompressedSize>0){
			if(*readOffset>=gSource[source].uncompressedSize){
				*readOffset=0;
				*iBufferNum = gSource[source].srcBufferNum;
				no_smPlaySegment(source);
				*iBufferNum = gSource[source].srcBufferNum;
				if(gSource[source].uncompressedSize==0){
					if (gSource[source].looping == AL_TRUE){
						gSource[source].readOffset = 0;
						gSource[source].uncompressedReadOffset = 0;
						gSource[source].uncompressedBufferOffset=0;
						if (gSource[source].pCompHdr != NULL) {
							free(gSource[source].pCompHdr);
							gSource[source].pCompHdr = NULL;
						} 
						no_smPlaySegment(source);
						*iBufferNum = gSource[source].srcBufferNum;
						*ratio = (unsigned int)(libGlobals.streamFormat.mSampleRate/(float)gBuffer[*iBufferNum].frequency+0.5);
						*one_over_ratio = 1.0f/(Float32)(*ratio);
						*gbufsize=1;
						if(gBuffer[*iBufferNum].bits==16)*gbufsize*=2;
						if(gBuffer[*iBufferNum].channels==2)*gbufsize*=2;
					}
					else
						gSource[source].state = AL_STOPPED;
				}
			}
		}
		else{
			if (gSource[source].looping == AL_TRUE){
				gSource[source].readOffset = 0;
				gSource[source].uncompressedReadOffset = 0;
				gSource[source].uncompressedBufferOffset=0;
				if (gSource[source].pCompHdr != NULL) {
					free(gSource[source].pCompHdr);
					gSource[source].pCompHdr = NULL;
				} 
				no_smPlaySegment(source);
			}
			else
				gSource[source].state = AL_STOPPED; 
		}
	}//vorbis handling
						
	if(gBuffer[*iBufferNum].format != AL_FORMAT_VORBIS_EXT){
#endif
		if ((gSource[source].readOffset >= gBuffer[*iBufferNum].size)){
			// check if there is a new queue to go to -- if not, then reset queue processed flags, decrement loop counter, and restart
			pQE = gSource[source].ptrQueue;
			if (pQE != NULL) {
				while (pQE->processed == AL_TRUE) {
					pQE = pQE->pNext;
					if (pQE == NULL) break;
				}
			}
				
			if (pQE != NULL){ // process next queued buffer
				pQE->processed = AL_TRUE;
				gSource[source].srcBufferNum = pQE->bufferNum;
				gSource[source].readOffset = 0;
				if (gSource[source].pCompHdr != NULL) {
					free(gSource[source].pCompHdr);
					gSource[source].pCompHdr = NULL;
				}
				bNewQE = AL_TRUE;
			} 
			else{ // completed all buffers, so reset buffer processed flags and decrement loop counter
					// reset all processed flags if looping
				if (gSource[source].looping == AL_TRUE){
					pQE = gSource[source].ptrQueue;
					while (pQE != NULL){
						pQE->processed = AL_FALSE;
						pQE = pQE->pNext;
					}
				}
								
				pQE = gSource[source].ptrQueue; // if there is a queue, stage first buffer
				if (pQE != NULL){
					gSource[source].srcBufferNum = pQE->bufferNum;
					gSource[source].readOffset = 0;
					if (gSource[source].pCompHdr != NULL) {
						free(gSource[source].pCompHdr);
						gSource[source].pCompHdr = NULL;
					}							
					pQE->processed = AL_TRUE;
				}
						
				// if looping is on and have uncompressed all compressed data, then reset source but keep it in a playing state
				if ((gSource[source].looping == AL_TRUE) && (gSource[source].uncompressedSize <= 0)) {
					gSource[source].readOffset = 0;
					gSource[source].uncompressedReadOffset = 0;
					gSource[source].uncompressedBufferOffset=0;
					if (gSource[source].pCompHdr != NULL) {
						free(gSource[source].pCompHdr);
						gSource[source].pCompHdr = NULL;
					}
				}
					
				// if looping is off and have uncompressed all compressed data, then set the source's state to STOPPED
				if ((gSource[source].looping != AL_TRUE) && (gSource[source].uncompressedSize <= 0))
					gSource[source].state = AL_STOPPED; 
			}
		}
#ifdef VORBIS_EXTENSION
	}
#endif
	// if now stopped, reset read pointer
	if (gSource[source].state == AL_STOPPED){
		gSource[source].readOffset = 0;
		if (gSource[source].pCompHdr != NULL) {
			free(gSource[source].pCompHdr);
			gSource[source].pCompHdr = NULL;
		}
	}

#ifdef VORBIS_EXTENSION				
	if(gBuffer[*iBufferNum].format != AL_FORMAT_VORBIS_EXT){
#endif
	// evaluate if more data needs to be played -- if so then call smPlaySegment
		if ((gSource[source].state == AL_PLAYING) && (((gSource[source].readOffset == 0) && (gSource[source].looping == AL_TRUE)) || (gSource[source].readOffset != 0) || (bNewQE == AL_TRUE)))
				no_smPlaySegment(source);
#ifdef VORBIS_EXTENSION
	}
#endif
}

OSStatus deviceFillingProc(AudioDeviceID  inDevice, const AudioTimeStamp*  inNow, const AudioBufferList*  inInputData, const AudioTimeStamp*  inInputTime, AudioBufferList*  outOutputData, const AudioTimeStamp* inOutputTime, void* inClientData)
{    
	int source,gbufsize=0;
	register int numChannels = libGlobals.streamFormat.mChannelsPerFrame;
	register unsigned int numFrames=0;
    register SInt16	*inDataPtr16=NULL;
    register UInt8	*inDataPtr8=NULL;
	Float32 vol_gainL=0.0,vol_gainR=0.0;
	Float32 one_over_ratio;
	ALfloat Pitch;
	register Float32	*outDataPtr = (outOutputData->mBuffers[0]).mData;
	int output_offset=0;
	ALuint *readOffset=NULL;
	ALfloat drysend[2], wetsend[2];
	ALsizei size;
	Boolean mix_2,mix_3,mix_4,mix_5,mix_6;
	
	memset( outDataPtr, 0, numChannels*(nativePreferedBuffSize*(sizeof(Float32) / 2)) );
	memset( mixbuf2, 0, nativePreferedBuffSize_X_2*sizeof(Float32)/2+2*sizeof(Float32));
	memset( mixbuf3, 0, nativePreferedBuffSize_X_2*sizeof(Float32)/3+2*sizeof(Float32));
	memset( mixbuf4, 0, nativePreferedBuffSize_X_2*sizeof(Float32)/4+2*sizeof(Float32));
	memset( mixbuf5, 0, nativePreferedBuffSize_X_2*sizeof(Float32)/5+2*sizeof(Float32));
	memset( mixbuf6, 0, nativePreferedBuffSize_X_2*sizeof(Float32)/6+2*sizeof(Float32));
	mix_2=FALSE;mix_3=FALSE;mix_4=FALSE;mix_5=FALSE;mix_6=FALSE;
	
	LockBufs();
	for(source=1;source<=AL_MAXSOURCES;source++){//loop over each source
		if((gSource[source].state==AL_PLAYING)){//only deal with it if it is playing
			iBufferNum = gSource[source].srcBufferNum;//get source's buffer num
			if(gBuffer[iBufferNum].size>0){
				alCalculateSourceParameters(source,2,drysend,wetsend,&Pitch);
				vol_gainL = drysend[0];
				if(vol_gainL>1.0f) vol_gainL=1.0f;
				vol_gainR = drysend[1];
				if(vol_gainR>1.0f) vol_gainR=1.0f;
				
				//if((vol_gainL>volume_threshold)||(vol_gainR>volume_threshold)){ //can actually hear it, so deal with it
#ifdef VORBIS_EXTENSION
					if((gBuffer[iBufferNum].frequency==0)&&(gBuffer[iBufferNum].format == AL_FORMAT_VORBIS_EXT)){ //if in queue, get the next valid buffer
						no_smPlaySegment(source);
						gSource[source].uncompressedBufferOffset=0;
						gSource[source].uncompressedReadOffset = 0;
						iBufferNum = gSource[source].srcBufferNum;//get source's buffer num
					}
#endif
					ratio = (unsigned int)(libGlobals.streamFormat.mSampleRate/(float)gBuffer[iBufferNum].frequency+0.5);
					if(ratio==0) ratio=1;
					one_over_ratio = 1.0f/(Float32)ratio;
					outDataPtr = (outOutputData->mBuffers[0]).mData; //coreaudio output buffer
					alWriteFormat = gBuffer[iBufferNum].format;
					
					gbufsize=1;
					if(gBuffer[iBufferNum].bits==16)gbufsize*=2;
					if(gBuffer[iBufferNum].channels==2)gbufsize*=2;
					
#ifdef VORBIS_EXTENSION
					if (gBuffer[iBufferNum].format != AL_FORMAT_VORBIS_EXT){
#endif
						if ((gSource[source].readOffset + (int)(Pitch*(ALfloat)(gbufsize*nativePreferedBuffSize/ratio))+0.5) <= gBuffer[iBufferNum].size) //determine samples to process
							numFrames=nativePreferedBuffSize/ratio;
						else
							numFrames=(int)((ALfloat)((gBuffer[iBufferNum].size-gSource[source].readOffset)/gbufsize)/Pitch+0.5);
						gSource[source].samplePtr = (char *) gBuffer[iBufferNum].data + gSource[source].readOffset; //get starting sample
						readOffset = &gSource[source].readOffset;
						size=gBuffer[iBufferNum].size;
#ifdef VORBIS_EXTENSION
					}
					else{
						if((gSource[source].uncompressedReadOffset+(int)(Pitch*(ALfloat)(gbufsize*nativePreferedBuffSize/ratio))+0.5) <= gSource[source].uncompressedSize)
							numFrames=nativePreferedBuffSize/ratio;
						else
							numFrames = (int)((ALfloat)((gSource[source].uncompressedSize-gSource[source].uncompressedReadOffset)/gbufsize)/Pitch+0.5);
						gSource[source].uncompressedBufferOffset+=(int)(Pitch*(ALfloat)((numFrames)/gbufsize)+0.5);
						gSource[source].samplePtr = (char *) gSource[source].uncompressedData + gSource[source].uncompressedReadOffset; //get starting sample
						if (gBuffer[iBufferNum].bits == 16){
							if(gBuffer[iBufferNum].channels==2)
								alWriteFormat = AL_FORMAT_STEREO16;
							else alWriteFormat = AL_FORMAT_MONO16;
						}
						else{
							if(gBuffer[iBufferNum].channels==2)
								alWriteFormat = AL_FORMAT_STEREO8;
							else alWriteFormat = AL_FORMAT_MONO8;
						}
						size=gSource[source].uncompressedSize;
						readOffset = &gSource[source].uncompressedReadOffset;
					}
#endif					
					if((vol_gainL>volume_threshold)||(vol_gainR>volume_threshold)){ //can actually hear it, so deal with it
                        inDataPtr16 = (SInt16*)(gSource[source].samplePtr); //if 16 bit data
                        inDataPtr8 = (UInt8*)(gSource[source].samplePtr); //if 8 bit
                        
                        memset( mixbuf, 0, (nativePreferedBuffSize_X_2)*sizeof(Float32)+2*sizeof(Float32)); //clear input buffer
                        memset( volbuf, 0, (nativePreferedBuffSize_X_2)*sizeof(Float32)+2*sizeof(Float32)); //clear volume buffer
                        
                        if(ratio==1){ //multiply by volume and add straight to output
                            if(Pitch==1.0) mix_with_same_sample_rates(outDataPtr,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,numFrames,numChannels,readOffset,size,TRUE);
                            else mix_with_same_sample_rates_w_pitch(outDataPtr,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,Pitch,numFrames,numChannels,readOffset,size,TRUE);
                        }
                        else if(ratio==2){ //otherwise mix with buffers of same sampling rate
                            if(Pitch==1.0) mix_with_same_sample_rates(mixbuf2,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,numFrames,numChannels,readOffset,size,FALSE);
                            else mix_with_same_sample_rates_w_pitch(mixbuf2,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,Pitch,numFrames,numChannels,readOffset,size,FALSE);
                            mix_2=TRUE;
                        }
                        else if(ratio==3){
                            if(Pitch==1.0) mix_with_same_sample_rates(mixbuf3,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,numFrames,numChannels,readOffset,size,FALSE);
                            else mix_with_same_sample_rates_w_pitch(mixbuf3,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,Pitch,numFrames,numChannels,readOffset,size,FALSE);
                            mix_3=TRUE;
                        }
                        else if(ratio==4){
                            if(Pitch==1.0) mix_with_same_sample_rates(mixbuf4,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,numFrames,numChannels,readOffset,size,FALSE);
                            else mix_with_same_sample_rates_w_pitch(mixbuf4,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,Pitch,numFrames,numChannels,readOffset,size,FALSE);
                            mix_4=TRUE;
                        }
                        else if(ratio==5){
                            if(Pitch==1.0) mix_with_same_sample_rates(mixbuf5,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,numFrames,numChannels,readOffset,size,FALSE);
                            else mix_with_same_sample_rates_w_pitch(mixbuf5,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,Pitch,numFrames,numChannels,readOffset,size,FALSE);
                            mix_5=TRUE;
                        }
                        else if(ratio==6){
                            if(Pitch==1.0) mix_with_same_sample_rates(mixbuf6,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,numFrames,numChannels,readOffset,size,FALSE);
                            else mix_with_same_sample_rates_w_pitch(mixbuf6,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,Pitch,numFrames,numChannels,readOffset,size,FALSE);
                            mix_6=TRUE;
                        }
                    }
                    else
                        parse_through_buffer_w_out_playing(Pitch,numFrames,readOffset,size);
				//}//if loud enough
				
				IOProc_Service(source,&iBufferNum,&ratio,readOffset,&one_over_ratio,&gbufsize);

				// We might need more data to fill output buffer, so check!
				if((numFrames*ratio<nativePreferedBuffSize)&&(numFrames!=0)){
					if (gSource[source].state == AL_PLAYING){ //there's more data to be played
#ifdef VORBIS_EXTENSION
						if(gBuffer[iBufferNum].format != AL_FORMAT_VORBIS_EXT){
#endif
							//if((vol_gainL>volume_threshold)||(vol_gainR>volume_threshold)){ //can actually hear it, so deal with it
								output_offset = numChannels*numFrames; //already written this much so dont overwrite it!
								numFrames=nativePreferedBuffSize/ratio-numFrames;
								iBufferNum = gSource[source].srcBufferNum;
								gSource[source].samplePtr = (char *) gBuffer[iBufferNum].data; //get starting sample
								readOffset = &gSource[source].readOffset;
								size=gBuffer[iBufferNum].size;
								
                                if((vol_gainL>volume_threshold)||(vol_gainR>volume_threshold)){ //can actually hear it, so deal with it
                                    inDataPtr16 = (SInt16*)(gSource[source].samplePtr); //if 16 bit data
                                    inDataPtr8 = (UInt8*)(gSource[source].samplePtr); //if 8 bit
                                    memset( mixbuf, 0, (nativePreferedBuffSize_X_2)*sizeof(Float32)+2*sizeof(Float32)); //clear input buffer
                                    memset( volbuf, 0, (nativePreferedBuffSize_X_2)*sizeof(Float32)+2*sizeof(Float32)); //clear volume buffer
                                    
                                    if(ratio==1){ //multiply by volume and add straight to output
                                        if(Pitch==1.0) mix_with_same_sample_rates(outDataPtr+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,numFrames,numChannels,readOffset,size,TRUE);
                                        else mix_with_same_sample_rates_w_pitch(outDataPtr+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,Pitch,numFrames,numChannels,readOffset,size,TRUE);
                                    }
                                    else if(ratio==2){ //otherwise mix with buffers of same sampling rate
                                        if(Pitch==1.0) mix_with_same_sample_rates(mixbuf2+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,numFrames,numChannels,readOffset,size,FALSE);
                                        else mix_with_same_sample_rates_w_pitch(mixbuf2+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,Pitch,numFrames,numChannels,readOffset,size,FALSE);
                                        mix_2=TRUE;
                                    }
                                    else if(ratio==3){
                                        if(Pitch==1.0) mix_with_same_sample_rates(mixbuf3+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,numFrames,numChannels,readOffset,size,FALSE);
                                        else mix_with_same_sample_rates_w_pitch(mixbuf3+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,Pitch,numFrames,numChannels,readOffset,size,FALSE);
                                        mix_3=TRUE;
                                    }
                                    else if(ratio==4){
                                        if(Pitch==1.0) mix_with_same_sample_rates(mixbuf4+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,numFrames,numChannels,readOffset,size,FALSE);
                                        else mix_with_same_sample_rates_w_pitch(mixbuf4+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,Pitch,numFrames,numChannels,readOffset,size,FALSE);
                                        mix_4=TRUE;
                                    }
                                    else if(ratio==5){
                                        if(Pitch==1.0) mix_with_same_sample_rates(mixbuf5+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,numFrames,numChannels,readOffset,size,FALSE);
                                        else mix_with_same_sample_rates_w_pitch(mixbuf5+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,Pitch,numFrames,numChannels,readOffset,size,FALSE);
                                        mix_5=TRUE;
                                    }
                                    else if(ratio==6){
                                        if(Pitch==1.0) mix_with_same_sample_rates(mixbuf6+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,numFrames,numChannels,readOffset,size,FALSE);
                                        else mix_with_same_sample_rates_w_pitch(mixbuf6+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,Pitch,numFrames,numChannels,readOffset,size,FALSE);
                                        mix_6=TRUE;
                                    }
                                }
                                else
                                    parse_through_buffer_w_out_playing(Pitch,numFrames,readOffset,size);
						//	}
#ifdef VORBIS_EXTENSION
						}
						else{
							//if((vol_gainL>volume_threshold)||(vol_gainR>volume_threshold)){ //can actually hear it, so deal with it
								output_offset = numChannels*numFrames; //already written this much so dont overwrite it!
								numFrames=nativePreferedBuffSize/ratio-numFrames;
								iBufferNum = gSource[source].srcBufferNum;
								ratio = (unsigned int)(libGlobals.streamFormat.mSampleRate/(float)gBuffer[iBufferNum].frequency+0.5);
								if(gBuffer[iBufferNum].frequency==0){ //if in queue, get the next valid buffer
									no_smPlaySegment(source);
									gSource[source].uncompressedBufferOffset=0;
									gSource[source].uncompressedReadOffset = 0;
									iBufferNum = gSource[source].srcBufferNum;//get source's buffer num
								}
								one_over_ratio = 1.0f/(Float32)(ratio);
								gbufsize=1;
								if(gBuffer[iBufferNum].bits==16)gbufsize*=2;
								if(gBuffer[iBufferNum].channels==2)gbufsize*=2;
								
								gSource[source].samplePtr = (char *) gSource[source].uncompressedData; //get starting sample
								if (gBuffer[iBufferNum].bits == 16){
									if(gBuffer[iBufferNum].channels==2)
										alWriteFormat = AL_FORMAT_STEREO16;
									else alWriteFormat = AL_FORMAT_MONO16;
								}
								else{
									if(gBuffer[iBufferNum].channels==2)
										alWriteFormat = AL_FORMAT_STEREO8;
									else alWriteFormat = AL_FORMAT_MONO8;
								}
								size=gSource[source].uncompressedSize;
								readOffset = &gSource[source].uncompressedReadOffset;
								
                                if((vol_gainL>volume_threshold)||(vol_gainR>volume_threshold)){ //can actually hear it, so deal with it
                                
                                    inDataPtr16 = (SInt16*)(gSource[source].samplePtr); //if 16 bit data
                                    inDataPtr8 = (UInt8*)(gSource[source].samplePtr); //if 8 bit
                                    memset( mixbuf, 0, (nativePreferedBuffSize_X_2)*sizeof(Float32)+2*sizeof(Float32)); //clear input buffer
                                    memset( volbuf, 0, (nativePreferedBuffSize_X_2)*sizeof(Float32)+2*sizeof(Float32)); //clear volume buffer
    
                                    if(ratio==1){ //multiply by volume and add straight to output
                                        if(Pitch==1.0) mix_with_same_sample_rates(outDataPtr+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,numFrames,numChannels,readOffset,size,TRUE);
                                        else mix_with_same_sample_rates_w_pitch(outDataPtr+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,Pitch,numFrames,numChannels,readOffset,size,TRUE);
                                    }
                                    else if(ratio==2){ //otherwise mix with buffers of same sampling rate
                                        if(Pitch==1.0) mix_with_same_sample_rates(mixbuf2+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,numFrames,numChannels,readOffset,size,FALSE);
                                        else mix_with_same_sample_rates_w_pitch(mixbuf2+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,Pitch,numFrames,numChannels,readOffset,size,FALSE);
                                        mix_2=TRUE;
                                    }
                                    else if(ratio==3){
                                        if(Pitch==1.0) mix_with_same_sample_rates(mixbuf3+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,numFrames,numChannels,readOffset,size,FALSE);
                                        else mix_with_same_sample_rates_w_pitch(mixbuf3+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,Pitch,numFrames,numChannels,readOffset,size,FALSE);
                                        mix_3=TRUE;
                                    }
                                    else if(ratio==4){
                                        if(Pitch==1.0) mix_with_same_sample_rates(mixbuf4+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,numFrames,numChannels,readOffset,size,FALSE);
                                        else mix_with_same_sample_rates_w_pitch(mixbuf4+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,Pitch,numFrames,numChannels,readOffset,size,FALSE);
                                        mix_4=TRUE;
                                    }
                                    else if(ratio==5){
                                        if(Pitch==1.0) mix_with_same_sample_rates(mixbuf5+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,numFrames,numChannels,readOffset,size,FALSE);
                                        else mix_with_same_sample_rates_w_pitch(mixbuf5+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,Pitch,numFrames,numChannels,readOffset,size,FALSE);
                                        mix_5=TRUE;
                                    }
                                    else if(ratio==6){
                                        if(Pitch==1.0) mix_with_same_sample_rates(mixbuf6+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,numFrames,numChannels,readOffset,size,FALSE);
                                        else mix_with_same_sample_rates_w_pitch(mixbuf6+output_offset,inDataPtr16,inDataPtr8,vol_gainL,vol_gainR,Pitch,numFrames,numChannels,readOffset,size,FALSE);
                                        mix_6=TRUE;
                                    }
                                }
                                else
                                    parse_through_buffer_w_out_playing(Pitch,numFrames,readOffset,size);
							}
						//}
#endif
					}
					IOProc_Service(source,&iBufferNum,&ratio,readOffset,&one_over_ratio,&gbufsize);
				}
			}//size>0
		}//if playing
	}//for each source
	
	if(mix_2==TRUE) //we have stuff to mix in to output
		SRC_and_mix(outDataPtr,mixbuf2,2,numChannels);
	if(mix_3==TRUE) //we have stuff to mix in to output
		SRC_and_mix(outDataPtr,mixbuf3,3,numChannels);
	if(mix_4==TRUE) //we have stuff to mix in to output
		SRC_and_mix(outDataPtr,mixbuf4,4,numChannels);
	if(mix_5==TRUE) //we have stuff to mix in to output
		SRC_and_mix(outDataPtr,mixbuf5,5,numChannels);
	if(mix_6==TRUE) //we have stuff to mix in to output
		SRC_and_mix(outDataPtr,mixbuf6,6,numChannels);
	
	UnlockBufs();
    return kAudioHardwareNoError;      
}

void no_smInit(void){
	OSStatus	error = 0;
    UInt32	count;
	globalVars *def=NULL;
	Boolean	outWritable;
	
	count = sizeof(libGlobals.deviceW);		// it is required to pass the size of the data to be returned
	error = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice,  &count, (void *) &libGlobals.deviceW);
	if (error != kAudioHardwareNoError) goto Crash;
	
	/* getting streams configs */
    error = AudioDeviceGetPropertyInfo(libGlobals.deviceW, 0, 0, kAudioDevicePropertyStreamConfiguration,  &count, &outWritable);
	if (error != kAudioHardwareNoError) goto Crash;
	libGlobals.deviceWBufferList = malloc(count);
        
	error = AudioDeviceGetProperty(libGlobals.deviceW, 0, 0, kAudioDevicePropertyStreamConfiguration, &count, libGlobals.deviceWBufferList);
	if (error != kAudioHardwareNoError) goto Crash;
	
	error = AudioDeviceGetPropertyInfo(libGlobals.deviceW, 0, 0, kAudioDevicePropertyStreamFormats,  &count, &outWritable);
    if (error != kAudioHardwareNoError) goto Crash;
	
    libGlobals.deviceFormats = (AudioStreamBasicDescription *) malloc(count);
	error = AudioDeviceGetProperty(libGlobals.deviceW, 0, 0, kAudioDevicePropertyStreamFormats, &count, &libGlobals.deviceFormats);
	if (error != kAudioHardwareNoError) goto Crash;
	
	error = AudioDeviceGetPropertyInfo(libGlobals.deviceW, 0, 0, kAudioDevicePropertyStreamFormat,  &count, &outWritable);
    if (error != kAudioHardwareNoError) goto Crash;
    error = AudioDeviceGetProperty(libGlobals.deviceW, 0, 0, kAudioDevicePropertyStreamFormat, &count, &libGlobals.streamFormat);
    if (error != kAudioHardwareNoError) goto Crash;
	
	/* Getting buffer size */
	
    error = AudioDeviceGetPropertyInfo(libGlobals.deviceW, 0, 0, kAudioDevicePropertyBufferSize, &count, &outWritable);
    if (error != kAudioHardwareNoError) goto Crash;
	error = AudioDeviceGetProperty(libGlobals.deviceW, 0, 0, kAudioDevicePropertyBufferSize, &count, &libGlobals.deviceWBufferSize);
	if (error != kAudioHardwareNoError) goto Crash;
	
	originalPreferedBuffSize = libGlobals.deviceWBufferSize;
	libGlobals.deviceWBufferSize=1024*sizeof(Float32)*libGlobals.streamFormat.mChannelsPerFrame; //faster like this
    error = AudioDeviceSetProperty(	libGlobals.deviceW, 0, 0, FALSE, kAudioDevicePropertyBufferSize, count, &libGlobals.deviceWBufferSize);
	if (error != kAudioHardwareNoError) goto Crash;
	
	nativePreferedBuffSize = libGlobals.deviceWBufferSize/(sizeof (Float32) * libGlobals.streamFormat.mChannelsPerFrame); //samples per channel for output ...
	nativePreferedBuffSize_X_2 = nativePreferedBuffSize<<1;	
	
	mix_mutex = mlCreateMutex();	
	
	error = AudioDeviceAddIOProc(libGlobals.deviceW, deviceFillingProc, (void *) def);	// setup our device with an IO proc
	if (error != kAudioHardwareNoError) goto Crash;
	error = AudioDeviceStart(libGlobals.deviceW, deviceFillingProc);
    if (error != kAudioHardwareNoError) goto Crash;
	
	mixbuf=(Float32 *)malloc(nativePreferedBuffSize_X_2*sizeof(Float32)+2*sizeof(Float32));
	volbuf=(Float32 *)malloc(nativePreferedBuffSize_X_2*sizeof(Float32)+2*sizeof(Float32));
	mixbuf2=(Float32 *)malloc(nativePreferedBuffSize_X_2*sizeof(Float32)/2+2*sizeof(Float32));
	mixbuf3=(Float32 *)malloc(nativePreferedBuffSize_X_2*sizeof(Float32)/3+2*sizeof(Float32));
	mixbuf4=(Float32 *)malloc(nativePreferedBuffSize_X_2*sizeof(Float32)/4+2*sizeof(Float32));
	mixbuf5=(Float32 *)malloc(nativePreferedBuffSize_X_2*sizeof(Float32)/5+2*sizeof(Float32));
	mixbuf6=(Float32 *)malloc(nativePreferedBuffSize_X_2*sizeof(Float32)/6+2*sizeof(Float32));
	memset( mixbuf, 0, (nativePreferedBuffSize_X_2)*sizeof(Float32)+2*sizeof(Float32)); //clear input buffer
	memset( volbuf, 0, (nativePreferedBuffSize_X_2)*sizeof(Float32)+2*sizeof(Float32)); //clear volume buffer
	INITIALIZED=TRUE;
	return;
Crash :
    libGlobals.deviceW = NULL;
    exit(1);
	
}

void no_smTerminate(void){
	OSStatus	error = 0;
    UInt32	count;
	Boolean	outWritable;
	
	free(mixbuf);
	free(volbuf);
	free(mixbuf2);
	free(mixbuf3);
	free(mixbuf4);
	free(mixbuf5);
	free(mixbuf6);
	AudioDeviceStop(libGlobals.deviceW, deviceFillingProc);
	AudioDeviceRemoveIOProc(libGlobals.deviceW, deviceFillingProc);
	
	error = AudioDeviceGetPropertyInfo(libGlobals.deviceW, 0, 0, kAudioDevicePropertyBufferSize, &count, &outWritable);
    if (error != kAudioHardwareNoError) goto Crash;
	error = AudioDeviceGetProperty(libGlobals.deviceW, 0, 0, kAudioDevicePropertyBufferSize, &count, &libGlobals.deviceWBufferSize);
	if (error != kAudioHardwareNoError) goto Crash;
	error = AudioDeviceSetProperty(	libGlobals.deviceW, 0, 0, FALSE, kAudioDevicePropertyBufferSize, count, &originalPreferedBuffSize);
	if (error != kAudioHardwareNoError) goto Crash;
	
	free(libGlobals.deviceWBufferList);
	free(libGlobals.deviceFormats);
	
	mlDestroyMutex(mix_mutex);
	return;
Crash :
    libGlobals.deviceW = NULL;
    exit(1);
}

void no_smPlaySegment(unsigned int source)
{
	QueueEntry *pQE;
	
	if(!INITIALIZED) 
		no_smInit();
	iBufferNum = gSource[source].srcBufferNum;
	
	#ifdef VORBIS_EXTENSION
	if (gBuffer[iBufferNum].format == AL_FORMAT_VORBIS_EXT) { // compressed format handling
		ov_fillBuffer(source, iBufferNum);
	}
	#endif
	if (gBuffer[iBufferNum].size > 0) // have data, so play...
	{
		gSource[source].state = AL_PLAYING;
	}
	else{ // find first un-processed queue (or don't find any to process)
		pQE = gSource[source].ptrQueue;
		if (pQE != NULL)
		{
			if (pQE->bufferNum == NULL)
			{
				pQE->processed = AL_TRUE;  // if first queue entry is null buffer, mark as processed and move on...
			}
			while (pQE->processed == AL_TRUE)
			{
				pQE = pQE->pNext;
				if (pQE == NULL) break;
				if (pQE->bufferNum == NULL)
				{
					pQE->processed = AL_TRUE; // if there are null buffers in the queue, mark as processed and move to next un-processed non-null buffer...
				}
			}
		}
		
		// if there's a queue to process, do it...
		if (pQE != NULL)
		{
			pQE->processed = AL_TRUE;
			gSource[source].srcBufferNum = pQE->bufferNum;
			no_smPlaySegment(source);
		} else
			gSource[source].state = AL_STOPPED;
	}
}