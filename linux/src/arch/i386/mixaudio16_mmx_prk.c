/***************************************************************************
 *   Copyright (C) 2005 by Prakash Punnoor                                 *
 *   prakash@punnoor.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "mixaudio16_mmx_prk.h"

#ifdef __MMX__
#include <string.h>

#ifdef HAVE_MMX_MEMCPY
void _alMMXmemcpy(void* dst, void* src, int n);
#endif /* HAVE_MMX_MEMCPY */


/*
 * We use built-ins for gcc instead of Intel/MSVC style intrinsics
 * as (older) gccs are slower with them
 */
#if __GNUC__ && !__INTEL_COMPILER
#if __GNUC__ < 4
typedef short v4hi __attribute__ ((__mode__(__V4HI__)));
typedef int   v2si __attribute__ ((__mode__(__V2SI__)));
typedef int   di   __attribute__ ((__mode__(__DI__)));
#else
typedef short v4hi __attribute__ ((vector_size (8)));
typedef int   v2si __attribute__ ((vector_size (8)));
typedef int   di   __attribute__ ((vector_size (8)));
#endif

#define ALIGN16(x) x __attribute__((aligned(16)))
typedef unsigned long aint;

#else /* NO GCC */
#include <mmintrin.h>
typedef __m64 v4hi;
typedef __m64 v2si;
typedef __m64 di;

#define __builtin_ia32_pand(X,Y)	_mm_and_si64(X,Y)
#define __builtin_ia32_pcmpeqw(X,Y)	_mm_cmpeq_pi16(X,Y)
#define __builtin_ia32_punpcklwd(X,Y)	_mm_unpacklo_pi16(X,Y)
#define __builtin_ia32_punpckhwd(X,Y)	_mm_unpackhi_pi16(X,Y)
#define __builtin_ia32_paddd(X,Y)	_mm_add_pi32(X,Y)
#define __builtin_ia32_paddsw(X,Y)	_mm_adds_pi16(X,Y)
#define __builtin_ia32_packssdw(X,Y)	_mm_packs_pi32(X,Y)
#define __builtin_ia32_emms() 		_mm_empty()

#define ALIGN16(x) __declspec(align(16)) x

/* FIXME: msvc++'s long in x86_64 isn't 8bytes? */
typedef unsigned long aint;
#endif /* __GNUC__ */


/* prepare sign-extension from 16bit to 32 bit for stream ST */
#define GET_SIGNMASK(ST)							\
	indata   = *(v4hi*)((ALshort*)entries[ST].data + offset);		\
	signmask = (v4hi)(di)__builtin_ia32_pand((di)indata, (di)m.v);		\
	signmask = __builtin_ia32_pcmpeqw(signmask, m.v);

/* mix stream 0 */
#define MIX_ST0									\
	GET_SIGNMASK (0);							\
	loout = (v2si)__builtin_ia32_punpcklwd(indata, signmask);		\
	hiout = (v2si)__builtin_ia32_punpckhwd(indata, signmask);

/* sign-extension and mix stream ST */
#define MIX(ST)									\
	GET_SIGNMASK(ST)							\
	temp  = (v2si)__builtin_ia32_punpcklwd(indata, signmask);		\
	loout = __builtin_ia32_paddd(loout, temp);				\
	temp  = (v2si)__builtin_ia32_punpckhwd(indata, signmask);		\
	hiout = __builtin_ia32_paddd(hiout, temp);

/* manual saturation to dst */
#define SATURATE								\
	if (sample == (short)sample)						\
		*dst = sample;							\
	else {									\
		if(sample > 0 )							\
			*dst = max_audioval;					\
		else								\
			*dst = min_audioval;					\
	}

/* manually mix LEN samples */
#define MIX_MANUAL(STREAMS, LEN)						\
	while (offset < LEN) {							\
		int sample = 0;							\
										\
		for (st = 0; st < STREAMS; ++st)				\
			sample += *((ALshort*)entries[st].data + offset);	\
										\
		SATURATE;							\
		++dst;								\
		++offset;							\
	}

/* manually mix two streams of LEN samples */
#define MIX_MANUAL2(LEN)							\
	while (offset < LEN) {							\
		int sample = *src1;						\
		++src1;								\
		sample += *src2;						\
		++src2;								\
										\
		SATURATE;							\
		++dst;								\
		++offset;\
	}


#define MixAudio16_MMX_HEAD(STREAMS)						\
	ALuint samples = entries[0].bytes;					\
	ALuint samples_main;							\
	ALuint samples_pre;							\
	ALuint samples_post;							\
	ALuint offset;								\
	ALuint st;								\
										\
	v4hi indata;								\
	v4hi signmask;								\
	v2si loout;								\
	v2si hiout;								\
	v2si temp;								\
										\
	samples /= sizeof(ALshort);						\
	offset = 0;								\
	if (samples < 40) {							\
		MIX_MANUAL(STREAMS, samples);					\
		return;								\
	}									\
										\
	samples_pre = MMX_ALIGN - (aint)dst % MMX_ALIGN;			\
	samples_pre /= sizeof(ALshort);						\
	samples_post = (samples - samples_pre) % 4;				\
	samples_main = samples - samples_post;					\
										\
	MIX_MANUAL(STREAMS, samples_pre);					\
										\
	while (offset < samples_main) {						\
										\
		MIX_ST0;

#define MixAudio16_MMX_N_TAIL(STREAMS)						\
		offset += 4;							\
	}									\
	__builtin_ia32_emms();							\
										\
	MIX_MANUAL(STREAMS, samples);						\
	return;

#define MixAudio16_MMX_TAIL(STREAMS)						\
		*(v4hi*)dst = __builtin_ia32_packssdw(loout, hiout);		\
		dst += 4;							\
		MixAudio16_MMX_N_TAIL(STREAMS);

/* sign-extension and mix stream; for generic function */
#define MIX_N									\
	indata   = *(v4hi*)((ALshort*)src->data + offset);			\
	signmask = (v4hi)(di)__builtin_ia32_pand((di)indata, (di)m.v);		\
	signmask = __builtin_ia32_pcmpeqw(signmask, m.v);			\
	temp  = (v2si)__builtin_ia32_punpcklwd(indata, signmask);		\
	loout = __builtin_ia32_paddd(loout, temp);				\
	temp  = (v2si)__builtin_ia32_punpckhwd(indata, signmask);		\
	hiout = __builtin_ia32_paddd(hiout, temp);

#define MMX_ALIGN 8


union {
	unsigned short s[4];
	v4hi v;
} ALIGN16(m) = {{0x8000, 0x8000, 0x8000, 0x8000}};


void MixAudio16_MMX_n(ALshort *dst, alMixEntry *entries, ALuint numents)
{
	ALuint mod_streams = (numents + 3) % 4;
	alMixEntry *src;
	MixAudio16_MMX_HEAD(numents);
	st = 1;
	src = entries + 1;
	
	switch (mod_streams) {
		do {
	case 0:
			MIX_N;
			++st;
			++src;
	case 3:
			MIX_N;
			++st;
			++src;
	case 2:
			MIX_N;
			++st;
			++src;
	case 1:
			MIX_N;
			++st;
			++src;
		} while (st < numents);
	}
	*(v4hi*)dst = __builtin_ia32_packssdw(loout, hiout);
	dst += 4;
	MixAudio16_MMX_N_TAIL(numents);
}

void MixAudio16_MMX_0(UNUSED(ALshort *dst), UNUSED(alMixEntry *entries))
{
	return;
}

void MixAudio16_MMX_1(ALshort *dst, alMixEntry *entries)
{
	int len = entries->bytes;
	
#ifdef HAVE_MMX_MEMCPY
	if (len < 300) {
		memcpy(dst, entries->data, len);
		return;
	}
	_alMMXmemcpy(dst, entries->data, len);
#else
	memcpy(dst, entries->data, len);
#endif /* HAVE_MMX_MEMCPY */
	return;
}

void MixAudio16_MMX_2(ALshort *dst, alMixEntry *entries)
{

	ALuint samples = entries[0].bytes;
	ALuint samples_main;
	ALuint samples_pre;
	ALuint samples_post;
	ALuint offset;
	
	ALshort *src1 = entries[0].data;
	ALshort *src2 = entries[1].data;
	
	samples /= sizeof(ALshort);
	offset = 0;
	if (samples < 40) {
		MIX_MANUAL2(samples);
		return;
	}
	
	samples_pre = MMX_ALIGN - (aint)dst % MMX_ALIGN;
	samples_pre /= sizeof(ALshort);
	samples_post = (samples - samples_pre) % 16;
	samples_main = samples - samples_post;

#define PADDSW									\
	*(v4hi*)dst = __builtin_ia32_paddsw(*(v4hi*)src1, *(v4hi*)src2);	\
	src1 += 4;								\
	src2 += 4;								\
	dst += 4;
	
	
	MIX_MANUAL2(samples_pre);

	while(offset < samples_main) {
		PADDSW;
		offset += 4 ;
		PADDSW;
		offset += 4;
		PADDSW;
		offset += 4;
		PADDSW;
		offset += 4;
	}
	__builtin_ia32_emms();
	
	MIX_MANUAL2(samples);
	return;
}

void MixAudio16_MMX_3(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(3);
	MIX(1);
	MIX(2);
	MixAudio16_MMX_TAIL(3);
}

void MixAudio16_MMX_4(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(4);
	MIX(1);
	MIX(2);
	MIX(3);
	MixAudio16_MMX_TAIL(4);
}

void MixAudio16_MMX_5(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(5);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MixAudio16_MMX_TAIL(5);
}

void MixAudio16_MMX_6(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(6);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MixAudio16_MMX_TAIL(6);
}

void MixAudio16_MMX_7(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(7);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MixAudio16_MMX_TAIL(7);
}

void MixAudio16_MMX_8(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(8);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MixAudio16_MMX_TAIL(8);
}

void MixAudio16_MMX_9(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(9);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MixAudio16_MMX_TAIL(9);
}

void MixAudio16_MMX_10(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(10);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MixAudio16_MMX_TAIL(10);
}

void MixAudio16_MMX_11(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(11);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MIX(10);
	MixAudio16_MMX_TAIL(11);
}

void MixAudio16_MMX_12(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(12);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MIX(10);
	MIX(11);
	MixAudio16_MMX_TAIL(12);
}

void MixAudio16_MMX_13(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(13);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MIX(10);
	MIX(11);
	MIX(12);
	MixAudio16_MMX_TAIL(13);
}

void MixAudio16_MMX_14(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(14);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MIX(10);
	MIX(11);
	MIX(12);
	MIX(13);
	MixAudio16_MMX_TAIL(14);
}

void MixAudio16_MMX_15(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(15);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MIX(10);
	MIX(11);
	MIX(12);
	MIX(13);
	MIX(14);
	MixAudio16_MMX_TAIL(15);
}

void MixAudio16_MMX_16(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(16);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MIX(10);
	MIX(11);
	MIX(12);
	MIX(13);
	MIX(14);
	MIX(15);
	MixAudio16_MMX_TAIL(16);
}

void MixAudio16_MMX_17(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(17);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MIX(10);
	MIX(11);
	MIX(12);
	MIX(13);
	MIX(14);
	MIX(15);
	MIX(16);
	MixAudio16_MMX_TAIL(17);
}

void MixAudio16_MMX_18(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(18);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MIX(10);
	MIX(11);
	MIX(12);
	MIX(13);
	MIX(14);
	MIX(15);
	MIX(16);
	MIX(17);
	MixAudio16_MMX_TAIL(18);
}

void MixAudio16_MMX_19(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(19);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MIX(10);
	MIX(11);
	MIX(12);
	MIX(13);
	MIX(14);
	MIX(15);
	MIX(16);
	MIX(17);
	MIX(18);
	MixAudio16_MMX_TAIL(19);
}

void MixAudio16_MMX_20(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(20);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MIX(10);
	MIX(11);
	MIX(12);
	MIX(13);
	MIX(14);
	MIX(15);
	MIX(16);
	MIX(17);
	MIX(18);
	MIX(19);
	MixAudio16_MMX_TAIL(20);
}

void MixAudio16_MMX_21(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(21);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MIX(10);
	MIX(11);
	MIX(12);
	MIX(13);
	MIX(14);
	MIX(15);
	MIX(16);
	MIX(17);
	MIX(18);
	MIX(19);
	MIX(20);
	MixAudio16_MMX_TAIL(21);
}

void MixAudio16_MMX_22(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(22);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MIX(10);
	MIX(11);
	MIX(12);
	MIX(13);
	MIX(14);
	MIX(15);
	MIX(16);
	MIX(17);
	MIX(18);
	MIX(19);
	MIX(20);
	MIX(21);
	MixAudio16_MMX_TAIL(22);
}

void MixAudio16_MMX_23(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(23);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MIX(10);
	MIX(11);
	MIX(12);
	MIX(13);
	MIX(14);
	MIX(15);
	MIX(16);
	MIX(17);
	MIX(18);
	MIX(19);
	MIX(20);
	MIX(21);
	MIX(22);
	MixAudio16_MMX_TAIL(23);
}

void MixAudio16_MMX_24(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(24);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MIX(10);
	MIX(11);
	MIX(12);
	MIX(13);
	MIX(14);
	MIX(15);
	MIX(16);
	MIX(17);
	MIX(18);
	MIX(19);
	MIX(20);
	MIX(21);
	MIX(22);
	MIX(23);
	MixAudio16_MMX_TAIL(24);
}

void MixAudio16_MMX_25(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(25);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MIX(10);
	MIX(11);
	MIX(12);
	MIX(13);
	MIX(14);
	MIX(15);
	MIX(16);
	MIX(17);
	MIX(18);
	MIX(19);
	MIX(20);
	MIX(21);
	MIX(22);
	MIX(23);
	MIX(24);
	MixAudio16_MMX_TAIL(25);
}

void MixAudio16_MMX_26(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(26);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MIX(10);
	MIX(11);
	MIX(12);
	MIX(13);
	MIX(14);
	MIX(15);
	MIX(16);
	MIX(17);
	MIX(18);
	MIX(19);
	MIX(20);
	MIX(21);
	MIX(22);
	MIX(23);
	MIX(24);
	MIX(25);
	MixAudio16_MMX_TAIL(26);
}

void MixAudio16_MMX_27(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(27);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MIX(10);
	MIX(11);
	MIX(12);
	MIX(13);
	MIX(14);
	MIX(15);
	MIX(16);
	MIX(17);
	MIX(18);
	MIX(19);
	MIX(20);
	MIX(21);
	MIX(22);
	MIX(23);
	MIX(24);
	MIX(25);
	MIX(26);
	MixAudio16_MMX_TAIL(27);
}

void MixAudio16_MMX_28(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(28);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MIX(10);
	MIX(11);
	MIX(12);
	MIX(13);
	MIX(14);
	MIX(15);
	MIX(16);
	MIX(17);
	MIX(18);
	MIX(19);
	MIX(20);
	MIX(21);
	MIX(22);
	MIX(23);
	MIX(24);
	MIX(25);
	MIX(26);
	MIX(27);
	MixAudio16_MMX_TAIL(28);
}

void MixAudio16_MMX_29(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(29);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MIX(10);
	MIX(11);
	MIX(12);
	MIX(13);
	MIX(14);
	MIX(15);
	MIX(16);
	MIX(17);
	MIX(18);
	MIX(19);
	MIX(20);
	MIX(21);
	MIX(22);
	MIX(23);
	MIX(24);
	MIX(25);
	MIX(26);
	MIX(27);
	MIX(28);
	MixAudio16_MMX_TAIL(29);
}

void MixAudio16_MMX_30(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(30);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MIX(10);
	MIX(11);
	MIX(12);
	MIX(13);
	MIX(14);
	MIX(15);
	MIX(16);
	MIX(17);
	MIX(18);
	MIX(19);
	MIX(20);
	MIX(21);
	MIX(22);
	MIX(23);
	MIX(24);
	MIX(25);
	MIX(26);
	MIX(27);
	MIX(28);
	MIX(29);
	MixAudio16_MMX_TAIL(30);
}

void MixAudio16_MMX_31(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(31);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MIX(10);
	MIX(11);
	MIX(12);
	MIX(13);
	MIX(14);
	MIX(15);
	MIX(16);
	MIX(17);
	MIX(18);
	MIX(19);
	MIX(20);
	MIX(21);
	MIX(22);
	MIX(23);
	MIX(24);
	MIX(25);
	MIX(26);
	MIX(27);
	MIX(28);
	MIX(29);
	MIX(30);
	MixAudio16_MMX_TAIL(31);
}

void MixAudio16_MMX_32(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_MMX_HEAD(32);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MIX(10);
	MIX(11);
	MIX(12);
	MIX(13);
	MIX(14);
	MIX(15);
	MIX(16);
	MIX(17);
	MIX(18);
	MIX(19);
	MIX(20);
	MIX(21);
	MIX(22);
	MIX(23);
	MIX(24);
	MIX(25);
	MIX(26);
	MIX(27);
	MIX(28);
	MIX(29);
	MIX(30);
	MIX(31);
	MixAudio16_MMX_TAIL(32);
}
#endif /* __MMX__ */
