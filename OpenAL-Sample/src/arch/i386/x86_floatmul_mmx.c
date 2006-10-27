/***************************************************************************
 *   MMX routine                                                           *
 *   Copyright (C) 2005-2006 by Prakash Punnoor                            *
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

/* MMX routine needs 16 */
#define SCALING_POWER  16
#define SCALING_FACTOR (1 << SCALING_POWER)

#define MIN_ENTER_SIMD_LEN 48


void _alFloatMul_MMX(ALshort *bpt, ALfloat sa, ALuint len) {
	ALint scaled_sa = sa * SCALING_FACTOR;
	ALint iter;
	
	if (len >= MIN_ENTER_SIMD_LEN) {
		v4hi v_sa;
		ALuint samples_main;
		ALuint samples_pre;
		ALuint samples_post;
		
		
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
			v_sa = setw(scaled_sa);
			
			while (samples_main--) {
				*(v4hi*)bpt = __builtin_ia32_pmulhw(*(v4hi*)bpt, v_sa);
				bpt += 4;
				*(v4hi*)bpt = __builtin_ia32_pmulhw(*(v4hi*)bpt, v_sa);
				bpt += 4;
			}
		} else {
			/* we lose 1 bit here, but well... */
			v4hi temp;
			short sa2 = scaled_sa >> 1;
			v_sa = setw(sa2);
			
			while (samples_main--) {
				/* work-around gcc 3.3.x bug */
				const long num_shift = 1L;
				
				temp = __builtin_ia32_pmulhw(*(v4hi*)bpt, v_sa);
				*(v4hi*)bpt = __builtin_ia32_psllw(temp, num_shift);
				bpt += 4;
				temp = __builtin_ia32_pmulhw(*(v4hi*)bpt, v_sa);
				*(v4hi*)bpt = __builtin_ia32_psllw(temp, num_shift);
				bpt += 4;
			}
		}
		__builtin_ia32_emms();
	}

	while(len--) {
		iter = *bpt;
		iter *= scaled_sa;
		iter >>= SCALING_POWER;
		*bpt = iter;
		++bpt;
	}
}
