/***************************************************************************
 *   MMX, SSE2 routine                                                           *
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

#include <AL/al.h>
#include "al_cpu_caps.h"
#include "x86_simd_support_prk.h"

/* MMX and SSE2 routines need 16 */
#define SCALING_POWER  16
#define SCALING_FACTOR (1 << SCALING_POWER)

void _alFloatMul(ALshort *bpt, ALfloat sa, ALuint len);

void _alFloatMul(ALshort *bpt, ALfloat sa, ALuint len) {
	ALint scaled_sa = sa * SCALING_FACTOR;
	ALint iter;
	
#ifdef __SSE2__
	if (_alHaveSSE2()) {
		v8hi v_sa;
		ALuint samples_main;
		ALuint samples_pre;
		ALuint samples_post;
		
		
		samples_pre = SSE2_ALIGN - (aint)bpt % SSE2_ALIGN;
		samples_pre /= sizeof(ALshort);
		samples_main = len - samples_pre;
		samples_post = samples_main % 16;
		samples_main = samples_main / 16;
		len = samples_post;
		
		while(samples_pre--) {
			iter = *bpt;
			iter *= scaled_sa;
			iter >>= SCALING_POWER;
			*bpt = iter;
			++bpt;
		}
		if (scaled_sa < (1 << 15)) {
			/* we do signed multiplication, so 1 << 15 is the max */
			v_sa = setw128(scaled_sa);
			
			while (samples_main--) {
				*(v8hi*)bpt = __builtin_ia32_pmulhw128(*(v8hi*)bpt, v_sa);
				bpt += 8;
				*(v8hi*)bpt = __builtin_ia32_pmulhw128(*(v8hi*)bpt, v_sa);
				bpt += 8;
			}
		} else {
			/* we lose 1 bit here, but well... */
			v8hi temp;
			short sa2 = scaled_sa >> 1;
			v_sa = setw128(sa2);
			
			while (samples_main--) {
				/* work-around gcc 3.3.x bug */
				const long num_shift = 1L;
				
				temp = __builtin_ia32_pmulhw128(*(v8hi*)bpt, v_sa);
				*(v8hi*)bpt = __builtin_ia32_psllwi128(temp, num_shift);
				bpt += 8;
				temp = __builtin_ia32_pmulhw128(*(v8hi*)bpt, v_sa);
				*(v8hi*)bpt = __builtin_ia32_psllwi128(temp, num_shift);
				bpt += 8;
			}
		}
	}
#ifdef __MMX__
	else
#endif /* __MMX__ */
#endif /* __SSE2__ */
#ifdef __MMX__
	if (_alHaveMMX()) {
		union {
			short s[4];
			v4hi v;
		} ALIGN16(v_sa);
		ALuint samples_main;
		ALuint samples_pre;
		ALuint samples_post;
		v4hi temp;
		
		
		samples_pre = MMX_ALIGN - (aint)bpt % MMX_ALIGN;
		samples_pre /= sizeof(ALshort);
		samples_main = len - samples_pre;
		samples_post = samples_main % 8;
		samples_main = samples_main / 8;
		len = samples_post;
		
		while(samples_pre--) {
			iter = *bpt;
			iter *= scaled_sa;
			iter >>= SCALING_POWER;
			*bpt = iter;
			++bpt;
		}
		
		if (scaled_sa < (1 << 15)) {
			/* we do signed multiplication, so 1 << 15 is the max */
			v_sa.s[0] = scaled_sa;
			v_sa.s[1] = v_sa.s[0];
			v_sa.s[2] = scaled_sa;
			v_sa.s[3] = v_sa.s[0];
			
			while (samples_main--) {
				*(v4hi*)bpt = __builtin_ia32_pmulhw(*(v4hi*)bpt, v_sa.v);
				bpt += 4;
				*(v4hi*)bpt = __builtin_ia32_pmulhw(*(v4hi*)bpt, v_sa.v);
				bpt += 4;
			}
		} else {
			/* we lose 1 bit here, but well... */
			v_sa.s[0] = scaled_sa >> 1;
			v_sa.s[1] = v_sa.s[0];
			v_sa.s[2] = v_sa.s[0];
			v_sa.s[3] = v_sa.s[0];
			
			while (samples_main--) {
				/* work-around gcc 3.3.x bug */
				const long num_shift = 1L;
				
				temp = __builtin_ia32_pmulhw(*(v4hi*)bpt, v_sa.v);
				*(v4hi*)bpt = __builtin_ia32_psllw(temp, num_shift);
				bpt += 4;
				temp = __builtin_ia32_pmulhw(*(v4hi*)bpt, v_sa.v);
				*(v4hi*)bpt = __builtin_ia32_psllw(temp, num_shift);
				bpt += 4;
			}
		}
		__builtin_ia32_emms();
	}
#endif /* __MMX__ */

	while(len--) {
		iter = *bpt;
		iter *= scaled_sa;
		iter >>= SCALING_POWER;
		*bpt = iter;
		++bpt;
	}
}
