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
    int reqAmt = (int)size*(int)nmemb;
    
    // note -- datasource points to the source which needs the data, which in turn points to the buffer which will supply the data
    
    int amt = (((ALsource *)datasource)->readOffset + reqAmt > gBuffer[((ALsource *)datasource)->srcBufferNum].size) ? gBuffer[((ALsource *)datasource)->srcBufferNum].size - ((ALsource *)datasource)->readOffset : reqAmt;
    memcpy(ptr, (void *)gBuffer[((ALsource *)datasource)->srcBufferNum].data + ((ALsource *)datasource)->readOffset, amt);
    ((ALsource *)datasource)->readOffset += amt;
            
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
    FILE *fh;
    
    // decompress the raw Ogg Vorbis data into pBuffer->uncompressedData
    //   also fill in uncompressedSize, channels, bits, and frequency fields
    
    // create some space for uncompressed data if not already available
    if (gBuffer[buffer].uncompressedData == NULL) {
        gBuffer[buffer].uncompressedData = (void *)NewPtrClear(gBufferSize);
        gBuffer[buffer].uncompressedSize = gBufferSize;
    }
    
    // create Ogg Vorbis File struct if not already created
    if (gSource[source].pCompHdr == NULL) {
        gSource[source].pCompHdr = (void *)NewPtrClear(sizeof(OggVorbis_File));
        // setup callbacks
        ov_callbacks callbacks;
        callbacks.read_func = ov_read_func;
        callbacks.seek_func = ov_seek_func;
        callbacks.close_func = ov_close_func;
        callbacks.tell_func = ov_tell_func;
        if (ov_open_callbacks(&gSource[source], gSource[source].pCompHdr, NULL, 0, callbacks) < 0) {
            DisposePtr(gSource[source].pCompHdr);
            gSource[source].pCompHdr = NULL;
        }
    }
    
    // set channels/bits/frequency
    vi=ov_info(gSource[source].pCompHdr,-1);
    gBuffer[buffer].channels = vi->channels;
    gBuffer[buffer].bits = 16;
    gBuffer[buffer].frequency = vi->rate;
  
    // decompress some data
    gBuffer[buffer].uncompressedSize=ov_read(gSource[source].pCompHdr,(void *)gBuffer[buffer].uncompressedData,gBufferSize,1,2,1,NULL);
}
