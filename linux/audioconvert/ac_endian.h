/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * ac_endian.h
 *
 * This file contains macros and prototypes for endian management.
 */
#include "al_siteconfig.h"
#include "AL/altypes.h"

#define ac_swap16(D) ((ALushort) (((D)<<8) | ((D)>>8)))
#define ac_swap32(D) (ALuint) ((((D)<<24) | (((D)<<8)&0x00FF0000) | (((D)>>8)&0x0000FF00) | ((D)>>24)))

ALubyte *cp16le(ALubyte *rawdata, ALushort *reader16);
ALubyte *cp32le(ALubyte *rawdata, ALuint *reader32);

#ifdef WORDS_BIGENDIAN
#define swap16le(x) ac_swap16(x)
#define swap32le(x) ac_swap32(x)
#define swap16be(x) (x)
#define swap32be(x) (x)
#else
#define swap16le(x) (x)
#define swap32le(x) (x)
#define swap16be(x) ac_swap16(x)
#define swap32be(x) ac_swap32(x)

#endif /* __big_endian */
