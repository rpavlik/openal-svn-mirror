/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * ac_endian.c
 *
 * Functions related to handling endian differences.
 *
 */
#include "al_siteconfig.h"

#include <string.h>

#include "ac_endian.h"
#include "AL/altypes.h"

ALubyte *cp16le(ALubyte *rawdata, ALushort *reader16) {
	memcpy(reader16, rawdata, sizeof *reader16);
	
        *reader16 = swap16le(*reader16);

	return rawdata + sizeof *reader16; 
} 

ALubyte *cp32le(ALubyte *rawdata, ALuint *reader32) {
	memcpy(reader32, rawdata, sizeof *reader32);

	*reader32 = swap32le(*reader32);
	return rawdata + sizeof *reader32; 
}
