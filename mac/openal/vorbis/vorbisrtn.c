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
#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"
#include "vorbisrtn.h"
#include "math.h"

size_t ov_read_func (void *ptr, size_t size, size_t nmemb, void *datasource)
{
    int buffer, source;
    int reqAmt = (int)size*(int)nmemb;
   
   // figure out which buffer this data is coming from and which source the buffer is attached to,
   // then set source's read position correctly and hand back the appropriate data
    for (buffer = 0; buffer < AL_MAXBUFFERS; buffer++) {
        if (gBuffer[buffer].data == datasource) {
            break; 
        }
    }
    
    if (buffer != AL_MAXBUFFERS) {
        for (source = 0; source < AL_MAXSOURCES	; source++) {
            if (gSource[source].srcBufferNum == buffer) {
                break;
            }
        }
        if (source != AL_MAXSOURCES) {
            // have buffer and source, so let's continue...
            int amt = (gSource[source].readOffset + reqAmt > gBuffer[buffer].size) ? gBuffer[buffer].size - gSource[source].readOffset : reqAmt;

            memcpy(ptr, datasource + gSource[source].readOffset, amt);
            gSource[source].readOffset += amt;
            return amt;
        }
    }
    return 0;
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

void ov_fillBuffer(ALsource *pSource, ALbuffer *pBuffer) 
{
    vorbis_info *vi;
    FILE *fh;
    
    // decompress the raw Ogg Vorbis data into pBuffer->uncompressedData
    //   also fill in uncompressedSize, channels, bits, and frequency fields
    
    // create some space for uncompressed data if not already available
    if (pBuffer->uncompressedData == NULL) {
        pBuffer->uncompressedData = (void *)NewPtrClear(gBufferSize);
        pBuffer->uncompressedSize = gBufferSize;
    }
    
    // create Ogg Vorbis File struct if not already created
    if (pSource->pCompHdr == NULL) {
        pSource->pCompHdr = (void *)NewPtrClear(sizeof(OggVorbis_File));
        // setup callbacks
        ov_callbacks callbacks;
        callbacks.read_func = ov_read_func;
        callbacks.seek_func = ov_seek_func;
        callbacks.close_func = ov_close_func;
        callbacks.tell_func = ov_tell_func;
        if (ov_open_callbacks(pBuffer->data, pSource->pCompHdr, NULL, 0, callbacks) < 0) {
            DisposePtr(pSource->pCompHdr);
            pSource->pCompHdr = NULL;
        }
    }
    
    // set channels/bits/frequency
    vi=ov_info(pSource->pCompHdr,-1);
    pBuffer->channels = vi->channels;
    pBuffer->bits = 16;
    pBuffer->frequency = vi->rate;
  
    // decompress some data
    pBuffer->uncompressedSize=ov_read(pSource->pCompHdr,(void *)pBuffer->uncompressedData,gBufferSize,1,2,1,NULL);
}
