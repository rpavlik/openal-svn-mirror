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

#include "al_siteconfig.h"
#include "mixaudio16_sse2_prk.h"

#ifdef __SSE2__
#include <string.h>
#include "x86_simd_support_prk.h"

#if defined(HAVE_MMX_MEMCPY) && defined(__MMX__)
void _alMMXmemcpy(void* dst, void* src, unsigned int n);
#endif /* HAVE_MMX_MEMCPY */

/* GCC 3.4 doesn't perform well with static inlines and intrinsics
   so we use some define magic */

/* prepare sign-extension from 16bit to 32 bit for stream ST */
#define GET_SIGNMASK(ST)							\
	indata   = to_v8hi(__builtin_ia32_loaddqu((char*)((ALshort*)entries[ST].data + offset)));\
	signmask = __builtin_ia32_psrawi128(indata, num_shift);

/* mix stream 0 */
#define MIX_ST0									\
	GET_SIGNMASK (0);							\
	loout = to_v4si(__builtin_ia32_punpcklwd128(indata, signmask));		\
	hiout = to_v4si(__builtin_ia32_punpckhwd128(indata, signmask));

/* sign-extension and mix stream ST */
#define MIX(ST)									\
	GET_SIGNMASK(ST)							\
	temp  = to_v4si(__builtin_ia32_punpcklwd128(indata, signmask));		\
	loout = __builtin_ia32_paddd128(loout, temp);				\
	temp  = to_v4si(__builtin_ia32_punpckhwd128(indata, signmask));		\
	hiout = __builtin_ia32_paddd128(hiout, temp);

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


#define MixAudio16_SSE2_HEAD(STREAMS)						\
	ALuint samples = entries[0].bytes;					\
	ALuint samples_main;							\
	ALuint samples_pre;							\
	ALuint samples_post;							\
	ALuint offset;								\
	ALuint st;								\
										\
	v8hi indata;								\
	v8hi signmask;								\
	v4si loout;								\
	v4si hiout;								\
	v4si temp;								\
										\
	/* work-around gcc 3.3.x bug */						\
	const long num_shift = 16L;						\
										\
	samples /= sizeof(ALshort);						\
	offset = 0;								\
	if (samples < 40) {							\
		MIX_MANUAL(STREAMS, samples);					\
		return;								\
	}									\
										\
	samples_pre = SSE2_ALIGN - (aint)dst % SSE2_ALIGN;			\
	samples_pre /= sizeof(ALshort);						\
	samples_post = (samples - samples_pre) % 8;				\
	samples_main = samples - samples_post;					\
										\
	MIX_MANUAL(STREAMS, samples_pre);					\
										\
	while (offset < samples_main) {						\
										\
		MIX_ST0;

#define MixAudio16_SSE2_N_TAIL(STREAMS)						\
		offset += 8;							\
	}									\
										\
	MIX_MANUAL(STREAMS, samples);						\
	return;

#define MixAudio16_SSE2_TAIL(STREAMS)						\
		*(v8hi*)dst = to_v8hi(__builtin_ia32_packssdw128(loout, hiout));\
		dst += 8;							\
		MixAudio16_SSE2_N_TAIL(STREAMS);

/* sign-extension and mix stream; for generic function */
#define MIX_N									\
	indata   = to_v8hi(__builtin_ia32_loaddqu((char*)((ALshort*)src->data + offset)));\
	signmask = __builtin_ia32_psrawi128(indata, num_shift);			\
	temp  = to_v4si(__builtin_ia32_punpcklwd128(indata, signmask));		\
	loout = __builtin_ia32_paddd128(loout, temp);				\
	temp  = to_v4si(__builtin_ia32_punpckhwd128(indata, signmask));		\
	hiout = __builtin_ia32_paddd128(hiout, temp);


void MixAudio16_SSE2_n(ALshort *dst, alMixEntry *entries, ALuint numents)
{
	ALuint mod_streams = (numents + 3) % 4;
	alMixEntry *src;
	MixAudio16_SSE2_HEAD(numents);
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
	*(v8hi*)dst = to_v8hi(__builtin_ia32_packssdw128(loout, hiout));
	dst += 8;
	MixAudio16_SSE2_N_TAIL(numents);
}

void MixAudio16_SSE2_0(UNUSED(ALshort *dst), UNUSED(alMixEntry *entries))
{
	return;
}

void MixAudio16_SSE2_1(ALshort *dst, alMixEntry *entries)
{
	unsigned int len = entries->bytes;
	
#if defined(HAVE_MMX_MEMCPY) && defined(__MMX__)
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

void MixAudio16_SSE2_2(ALshort *dst, alMixEntry *entries)
{

	ALuint samples = entries[0].bytes;
	ALuint samples_main;
	ALuint samples_pre;
	ALuint samples_post;
	ALuint offset;

	v8hi v1, v2;
	
	ALshort *src1 = entries[0].data;
	ALshort *src2 = entries[1].data;
	
	samples /= sizeof(ALshort);
	offset = 0;
	if (samples < 40) {
		MIX_MANUAL2(samples);
		return;
	}
	
	samples_pre = SSE2_ALIGN - (aint)dst % SSE2_ALIGN;
	samples_pre /= sizeof(ALshort);
	samples_post = (samples - samples_pre) % 16;
	samples_main = samples - samples_post;

#define PADDSW128								\
	v1 = to_v8hi(__builtin_ia32_loaddqu((char*)src1));				\
	v2 = to_v8hi(__builtin_ia32_loaddqu((char*)src2));				\
	*(v8hi*)dst = __builtin_ia32_paddsw128(v1, v2);				\
	src1 += 8;								\
	src2 += 8;								\
	dst += 8;
	
	
	MIX_MANUAL2(samples_pre);

	while(offset < samples_main) {
		PADDSW128;
		offset += 8 ;
		PADDSW128;
		offset += 8;
	}
	
	MIX_MANUAL2(samples);
	return;
}

void MixAudio16_SSE2_3(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(3);
	MIX(1);
	MIX(2);
	MixAudio16_SSE2_TAIL(3);
}

void MixAudio16_SSE2_4(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(4);
	MIX(1);
	MIX(2);
	MIX(3);
	MixAudio16_SSE2_TAIL(4);
}

void MixAudio16_SSE2_5(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(5);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MixAudio16_SSE2_TAIL(5);
}

void MixAudio16_SSE2_6(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(6);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MixAudio16_SSE2_TAIL(6);
}

void MixAudio16_SSE2_7(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(7);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MixAudio16_SSE2_TAIL(7);
}

void MixAudio16_SSE2_8(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(8);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MixAudio16_SSE2_TAIL(8);
}

void MixAudio16_SSE2_9(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(9);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MixAudio16_SSE2_TAIL(9);
}

void MixAudio16_SSE2_10(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(10);
	MIX(1);
	MIX(2);
	MIX(3);
	MIX(4);
	MIX(5);
	MIX(6);
	MIX(7);
	MIX(8);
	MIX(9);
	MixAudio16_SSE2_TAIL(10);
}

void MixAudio16_SSE2_11(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(11);
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
	MixAudio16_SSE2_TAIL(11);
}

void MixAudio16_SSE2_12(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(12);
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
	MixAudio16_SSE2_TAIL(12);
}

void MixAudio16_SSE2_13(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(13);
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
	MixAudio16_SSE2_TAIL(13);
}

void MixAudio16_SSE2_14(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(14);
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
	MixAudio16_SSE2_TAIL(14);
}

void MixAudio16_SSE2_15(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(15);
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
	MixAudio16_SSE2_TAIL(15);
}

void MixAudio16_SSE2_16(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(16);
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
	MixAudio16_SSE2_TAIL(16);
}

void MixAudio16_SSE2_17(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(17);
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
	MixAudio16_SSE2_TAIL(17);
}

void MixAudio16_SSE2_18(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(18);
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
	MixAudio16_SSE2_TAIL(18);
}

void MixAudio16_SSE2_19(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(19);
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
	MixAudio16_SSE2_TAIL(19);
}

void MixAudio16_SSE2_20(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(20);
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
	MixAudio16_SSE2_TAIL(20);
}

void MixAudio16_SSE2_21(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(21);
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
	MixAudio16_SSE2_TAIL(21);
}

void MixAudio16_SSE2_22(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(22);
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
	MixAudio16_SSE2_TAIL(22);
}

void MixAudio16_SSE2_23(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(23);
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
	MixAudio16_SSE2_TAIL(23);
}

void MixAudio16_SSE2_24(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(24);
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
	MixAudio16_SSE2_TAIL(24);
}

void MixAudio16_SSE2_25(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(25);
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
	MixAudio16_SSE2_TAIL(25);
}

void MixAudio16_SSE2_26(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(26);
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
	MixAudio16_SSE2_TAIL(26);
}

void MixAudio16_SSE2_27(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(27);
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
	MixAudio16_SSE2_TAIL(27);
}

void MixAudio16_SSE2_28(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(28);
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
	MixAudio16_SSE2_TAIL(28);
}

void MixAudio16_SSE2_29(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(29);
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
	MixAudio16_SSE2_TAIL(29);
}

void MixAudio16_SSE2_30(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(30);
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
	MixAudio16_SSE2_TAIL(30);
}

void MixAudio16_SSE2_31(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(31);
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
	MixAudio16_SSE2_TAIL(31);
}

void MixAudio16_SSE2_32(ALshort *dst, alMixEntry *entries)
{
	MixAudio16_SSE2_HEAD(32);
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
	MixAudio16_SSE2_TAIL(32);
}

#endif /* __SSE2__ */
