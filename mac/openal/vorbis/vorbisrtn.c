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

#include "globals.h"
#include <stdlib.h>
#include <string.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include "vorbisrtn.h"
#include "math.h"

size_t ov_read_func (void *ptr, size_t size, size_t nmemb, void *datasource)
{
    QueueEntry *pQE;
    int reqAmt = (int)size*(int)nmemb;
    
    // note -- datasource points to the source which needs the data, which in turn points to the buffer which will supply the data
    int amt = (((ALsource *)datasource)->readOffset + reqAmt > gBuffer[((ALsource *)datasource)->srcBufferNum].size) ? gBuffer[((ALsource *)datasource)->srcBufferNum].size - ((ALsource *)datasource)->readOffset : reqAmt;
    memcpy(ptr, (void *)gBuffer[((ALsource *)datasource)->srcBufferNum].data + ((ALsource *)datasource)->readOffset, amt);
    ((ALsource *)datasource)->readOffset += amt;
   
    if (amt < reqAmt) {  // if not all requested data transferred yet, check for a queue to grab more data from
        pQE = ((ALsource *)datasource)->ptrQueue;
        
        if (pQE != NULL) {
            while (pQE->processed == AL_TRUE) {
                pQE = pQE->pNext;
                if (pQE == NULL) break;
            }
        }
	    
        if (pQE != NULL) // process next queued buffer
        {
            pQE->processed = AL_TRUE;
            ((ALsource *)datasource)->srcBufferNum = pQE->bufferNum;
            ((ALsource *)datasource)->readOffset = 0; 
            //int supplement = ov_read_func(ptr + amt, 1, reqAmt - amt, datasource); //this is handled by the no_smService instead now
            //return amt + supplement;
			return amt;
        }
    }
            
    return amt;
}

int ov_seek_func (void *datasource, ogg_int64_t offset, int whence)
{
   return -1;
}

int ov_close_func (void *datasource)
{
   return 0;
}

long ov_tell_func (void *datasource)
{
   return 0;
}

void ov_fillBuffer(ALuint source, ALuint buffer) 
{
    vorbis_info *vi;
	void *data_ptr;
	int size,total_size,request,size_want;
     ov_callbacks callbacks;
	
    // decompress the raw Ogg Vorbis data into uncompressedData for source
    //   also fill in uncompressedSize, channels, bits, and frequency fields
    
    // create some space for uncompressed data if not already available
    if (gSource[source].uncompressedData == NULL) {
        gSource[source].uncompressedData = (void *)malloc(gBufferSize);
        gSource[source].uncompressedSize = gBufferSize;
    }
    
    // create Ogg Vorbis File struct if not already created
    if (gSource[source].pCompHdr == NULL) {
        gSource[source].pCompHdr = (void *)malloc(sizeof(OggVorbis_File));
        // setup callbacks
        callbacks.read_func = ov_read_func;
        callbacks.seek_func = ov_seek_func;
        callbacks.close_func = ov_close_func;
        callbacks.tell_func = ov_tell_func;
        if (ov_open_callbacks(&gSource[source], gSource[source].pCompHdr, NULL, 0, callbacks) < 0) {
            if(gSource[source].pCompHdr!=NULL){
				free(gSource[source].pCompHdr);
				gSource[source].pCompHdr = NULL;
			}
        }
    }
  
	// decompress some data
	size=0;
	total_size=0;
	
	if(gBuffer[buffer].size>AL_DEFAULT_INTERNAL_BUFFERS_SIZE) //we want to fill at least a buffer size worth of data
		size_want = AL_DEFAULT_INTERNAL_BUFFERS_SIZE;
    else
		size_want = gBuffer[buffer].size;
	
	// set channels/bits/frequency
    vi=ov_info(gSource[source].pCompHdr,-1);
    gBuffer[buffer].channels = vi->channels;
    gBuffer[buffer].bits = 16;
    gBuffer[buffer].frequency = vi->rate;
	
	request=size_want;
	size=ov_read(gSource[source].pCompHdr,(void *)gSource[source].uncompressedData,request,1,2,1,NULL);
	
	total_size+=size;
	data_ptr = (void *)gSource[source].uncompressedData;
	if((int)gSource[source].uncompressedSize<0)
			gSource[source].uncompressedSize=0;
	gSource[source].uncompressedReadOffset=0;
	gSource[source].samplePtr = (char *) gSource[source].uncompressedData;
	if(size>0){ //if we can still read the file
		while(total_size<size_want){ //get the amount of data we want
			request=size_want-total_size; //update how much we still need
			if((size==0)||(buffer!=gSource[source].srcBufferNum))
				break; //nothing to read, or we are jumping to a new buffer
			size=ov_read(gSource[source].pCompHdr,(void *)(gSource[source].uncompressedData+(total_size>>1)),request,1,2,1,NULL); //fill up buffer
			total_size+=size;
		}
	}
	gSource[source].uncompressedSize = total_size;
	gSource[source].uncompressedData = data_ptr;
	gSource[source].uncompressedReadOffset = 0;
	gSource[source].uncompressedBufferOffset+=total_size;
}
