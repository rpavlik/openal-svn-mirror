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
   static int offset = 0;
   int amt = (offset + reqAmt > 19502) ? 19502 - offset : reqAmt;

   memcpy(ptr, datasource + offset, amt);
   offset += amt;
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

void ov_fillBuffer(ALbuffer *pBuffer) 
{
    int i;
    char *pC;
    
    // decompress the raw Ogg Vorbis data into pBuffer->uncompressedData
    //   also fill in uncompressedSize, channels, bits, and frequency fields
    
    pBuffer->channels = 1;
    pBuffer->bits = 16;
    pBuffer->frequency = 22050;
    pBuffer->uncompressedData = (void *)NewPtrClear(20000);
    pBuffer->uncompressedSize = 20000;
    
    // ***** dumping in a sine wave for the moment....
    for (i = 0; i < 10000; i++) {
        pC = pBuffer->uncompressedData + i;
        *pC = sin(i/100) * 32767; 
    }
}
