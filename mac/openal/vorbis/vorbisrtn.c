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
#include <mach-o/dyld.h>
#define EXTERN extern
#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"
#include "vorbisrtn.h"
#include "math.h"

ALboolean ov_setVorbisFunctionPointers(void *pLib) 
{
    NSSymbol tmpSymbol;

    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_info_init", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_info_init = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_info_init == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_info_clear", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_info_clear = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_info_clear == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_info_blocksize", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_info_blocksize = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_info_blocksize == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_comment_init", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_comment_init = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_comment_init == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_comment_add", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_comment_add = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_comment_add == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_comment_add_tag", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_comment_add_tag = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_comment_add_tag == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_comment_query", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_comment_query = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_comment_query == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_comment_query_count", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_comment_query_count = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_comment_query_count == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_comment_clear", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_comment_clear = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_comment_clear == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_block_init", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_block_init = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_block_init == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_block_clear", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_block_clear = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_block_clear == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_dsp_clear", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_dsp_clear = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_dsp_clear == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_analysis_init", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_analysis_init = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_analysis_init == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_commentheader_out", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_commentheader_out = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_commentheader_out == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_analysis_headerout", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_analysis_headerout = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_analysis_headerout == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_analysis_buffer", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_analysis_buffer = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_analysis_buffer == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_analysis_wrote", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_analysis_wrote = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_analysis_wrote == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_analysis_blockout", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_analysis_blockout = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_analysis_blockout == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_analysis", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_analysis = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_analysis == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_bitrate_addblock", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_bitrate_addblock = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_bitrate_addblock == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_bitrate_flushpacket", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_bitrate_flushpacket = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_bitrate_flushpacket == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_synthesis_headerin", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_synthesis_headerin = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_synthesis_headerin == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_synthesis_init", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_synthesis_init = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_synthesis_init == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_synthesis", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_synthesis = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_synthesis == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_synthesis_trackonly", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_synthesis_trackonly = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_synthesis_trackonly == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_synthesis_blockin", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_synthesis_blockin = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_synthesis_blockin == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_synthesis_pcmout", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_synthesis_pcmout = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_synthesis_pcmout == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_synthesis_read", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_synthesis_read = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_synthesis_read == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_vorbis_packet_blocksize", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    vorbis_packet_blocksize = NSAddressOfSymbol(tmpSymbol);
    if (vorbis_packet_blocksize == NULL) { return AL_FALSE; }

    return AL_TRUE;
}

ALboolean ov_setOggFunctionPointers(void *pLib) 
{
    NSSymbol tmpSymbol;

    tmpSymbol = NSLookupSymbolInImage(pLib, "_oggpack_writeinit", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    oggpack_writeinit = NSAddressOfSymbol(tmpSymbol);
    if (oggpack_writeinit == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_oggpack_writetrunc", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    oggpack_writetrunc = NSAddressOfSymbol(tmpSymbol);
    if (oggpack_writetrunc == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_oggpack_writealign", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    oggpack_writealign = NSAddressOfSymbol(tmpSymbol);
    if (oggpack_writealign == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_oggpack_writecopy", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    oggpack_writecopy = NSAddressOfSymbol(tmpSymbol);
    if (oggpack_writecopy == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_oggpack_reset", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    oggpack_reset = NSAddressOfSymbol(tmpSymbol);
    if (oggpack_reset == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_oggpack_writeclear", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    oggpack_writeclear = NSAddressOfSymbol(tmpSymbol);
    if (oggpack_writeclear == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_oggpack_readinit", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    oggpack_readinit = NSAddressOfSymbol(tmpSymbol);
    if (oggpack_readinit == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_oggpack_write", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    oggpack_write = NSAddressOfSymbol(tmpSymbol);
    if (oggpack_write == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_oggpack_look", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    oggpack_look = NSAddressOfSymbol(tmpSymbol);
    if (oggpack_look == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_oggpack_look1", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    oggpack_look1 = NSAddressOfSymbol(tmpSymbol);
    if (oggpack_look1 == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_oggpack_adv", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    oggpack_adv = NSAddressOfSymbol(tmpSymbol);
    if (oggpack_adv == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_oggpack_adv1", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    oggpack_adv1 = NSAddressOfSymbol(tmpSymbol);
    if (oggpack_adv1 == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_oggpack_read", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    oggpack_read = NSAddressOfSymbol(tmpSymbol);
    if (oggpack_read == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_oggpack_read1", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    oggpack_read1 = NSAddressOfSymbol(tmpSymbol);
    if (oggpack_read1 == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_oggpack_bytes", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    oggpack_bytes = NSAddressOfSymbol(tmpSymbol);
    if (oggpack_bytes == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_oggpack_bits", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    oggpack_bits = NSAddressOfSymbol(tmpSymbol);
    if (oggpack_bits == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_oggpack_get_buffer", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    oggpack_get_buffer = NSAddressOfSymbol(tmpSymbol);
    if (oggpack_get_buffer == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_stream_packetin", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_stream_packetin = NSAddressOfSymbol(tmpSymbol);
    if (ogg_stream_packetin == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_stream_pageout", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_stream_pageout = NSAddressOfSymbol(tmpSymbol);
    if (ogg_stream_pageout == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_stream_flush", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_stream_flush = NSAddressOfSymbol(tmpSymbol);
    if (ogg_stream_flush == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_sync_init", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_sync_init = NSAddressOfSymbol(tmpSymbol);
    if (ogg_sync_init == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_sync_clear", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_sync_clear = NSAddressOfSymbol(tmpSymbol);
    if (ogg_sync_clear == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_sync_reset", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_sync_reset = NSAddressOfSymbol(tmpSymbol);
    if (ogg_sync_reset == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_sync_destroy", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_sync_destroy = NSAddressOfSymbol(tmpSymbol);
    if (ogg_sync_destroy == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_sync_buffer", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_sync_buffer = NSAddressOfSymbol(tmpSymbol);
    if (ogg_sync_buffer == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_sync_wrote", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_sync_wrote = NSAddressOfSymbol(tmpSymbol);
    if (ogg_sync_wrote == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_sync_pageseek", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_sync_pageseek = NSAddressOfSymbol(tmpSymbol);
    if (ogg_sync_pageseek == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_sync_pageout", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_sync_pageout = NSAddressOfSymbol(tmpSymbol);
    if (ogg_sync_pageout == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_stream_pagein", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_stream_pagein = NSAddressOfSymbol(tmpSymbol);
    if (ogg_stream_pagein == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_stream_packetout", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_stream_packetout = NSAddressOfSymbol(tmpSymbol);
    if (ogg_stream_packetout == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_stream_packetpeek", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_stream_packetpeek = NSAddressOfSymbol(tmpSymbol);
    if (ogg_stream_packetpeek == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_stream_init", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_stream_init = NSAddressOfSymbol(tmpSymbol);
    if (ogg_stream_init == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_stream_clear", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_stream_clear = NSAddressOfSymbol(tmpSymbol);
    if (ogg_stream_clear == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_stream_reset", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_stream_reset = NSAddressOfSymbol(tmpSymbol);
    if (ogg_stream_reset == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_stream_reset_serialno", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_stream_reset_serialno = NSAddressOfSymbol(tmpSymbol);
    if (ogg_stream_reset_serialno == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_stream_destroy", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_stream_destroy = NSAddressOfSymbol(tmpSymbol);
    if (ogg_stream_destroy == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_stream_eos", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_stream_eos = NSAddressOfSymbol(tmpSymbol);
    if (ogg_stream_eos == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_page_checksum_set", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_page_checksum_set = NSAddressOfSymbol(tmpSymbol);
    if (ogg_page_checksum_set == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_page_version", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_page_version = NSAddressOfSymbol(tmpSymbol);
    if (ogg_page_version == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_page_continued", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_page_continued = NSAddressOfSymbol(tmpSymbol);
    if (ogg_page_continued == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_page_bos", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_page_bos = NSAddressOfSymbol(tmpSymbol);
    if (ogg_page_bos == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_page_eos", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_page_eos = NSAddressOfSymbol(tmpSymbol);
    if (ogg_page_eos == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_page_granulepos", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_page_granulepos = NSAddressOfSymbol(tmpSymbol);
    if (ogg_page_granulepos == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_page_serialno", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_page_serialno = NSAddressOfSymbol(tmpSymbol);
    if (ogg_page_serialno == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_page_pageno", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_page_pageno = NSAddressOfSymbol(tmpSymbol);
    if (ogg_page_pageno == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_page_packets", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_page_packets = NSAddressOfSymbol(tmpSymbol);
    if (ogg_page_packets == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ogg_packet_clear", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ogg_packet_clear = NSAddressOfSymbol(tmpSymbol);
    if (ogg_packet_clear == NULL) { return AL_FALSE; }

    return AL_TRUE;
}

ALboolean ov_setVorbisFileFunctionPointers(void *pLib) 
{
    NSSymbol tmpSymbol;

    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_clear", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_clear = NSAddressOfSymbol(tmpSymbol);
    if (ov_clear == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_open", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_open = NSAddressOfSymbol(tmpSymbol);
    if (ov_open == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_open_callbacks", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_open_callbacks = NSAddressOfSymbol(tmpSymbol);
    if (ov_open_callbacks == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_test", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_test = NSAddressOfSymbol(tmpSymbol);
    if (ov_test == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_test_callbacks", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_test_callbacks = NSAddressOfSymbol(tmpSymbol);
    if (ov_test_callbacks == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_test_open", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_test_open = NSAddressOfSymbol(tmpSymbol);
    if (ov_test_open == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_bitrate", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_bitrate = NSAddressOfSymbol(tmpSymbol);
    if (ov_bitrate == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_bitrate_instant", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_bitrate_instant = NSAddressOfSymbol(tmpSymbol);
    if (ov_bitrate_instant == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_streams", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_streams = NSAddressOfSymbol(tmpSymbol);
    if (ov_streams == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_seekable", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_seekable = NSAddressOfSymbol(tmpSymbol);
    if (ov_seekable == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_serialnumber", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_serialnumber = NSAddressOfSymbol(tmpSymbol);
    if (ov_serialnumber == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_raw_total", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_raw_total = NSAddressOfSymbol(tmpSymbol);
    if (ov_raw_total == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_pcm_total", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_pcm_total = NSAddressOfSymbol(tmpSymbol);
    if (ov_pcm_total == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_time_total", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_time_total = NSAddressOfSymbol(tmpSymbol);
    if (ov_time_total == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_raw_seek", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_raw_seek = NSAddressOfSymbol(tmpSymbol);
    if (ov_raw_seek == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_pcm_seek", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_pcm_seek = NSAddressOfSymbol(tmpSymbol);
    if (ov_pcm_seek == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_pcm_seek_page", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_pcm_seek_page = NSAddressOfSymbol(tmpSymbol);
    if (ov_pcm_seek_page == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_time_seek", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_time_seek = NSAddressOfSymbol(tmpSymbol);
    if (ov_time_seek == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_time_seek_page", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_time_seek_page = NSAddressOfSymbol(tmpSymbol);
    if (ov_time_seek_page == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_raw_tell", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_raw_tell = NSAddressOfSymbol(tmpSymbol);
    if (ov_raw_tell == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_pcm_tell", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_pcm_tell = NSAddressOfSymbol(tmpSymbol);
    if (ov_pcm_tell == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_time_tell", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_time_tell = NSAddressOfSymbol(tmpSymbol);
    if (ov_time_tell == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_info", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_info = NSAddressOfSymbol(tmpSymbol);
    if (ov_info == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_comment", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_comment = NSAddressOfSymbol(tmpSymbol);
    if (ov_comment == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_read_float", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_read_float = NSAddressOfSymbol(tmpSymbol);
    if (ov_read_float == NULL) { return AL_FALSE; }
    
    tmpSymbol = NSLookupSymbolInImage(pLib, "_ov_read", NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
    ov_read = NSAddressOfSymbol(tmpSymbol);
    if (ov_read == NULL) { return AL_FALSE; }

    return AL_TRUE;
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
