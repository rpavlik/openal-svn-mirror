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
#ifndef AL_ARCH_I386_MIXAUDIO16_SSE2_PRK_H_
#define AL_ARCH_I386_MIXAUDIO16_SSE2_PRK_H_

#include "al_siteconfig.h"

#ifdef HAVE_SSE2
#include <AL/al.h>
#include "al_main.h"
#include "al_types.h"

/*
 * max_audioval is the maximum value of a signed short.
 */
#define max_audioval ((1<<(16-1))-1)

/*
 * min_audioval is the minimum value of a signed short.
 */
#define min_audioval -(1<<(16-1))

void MixAudio16_SSE2_n(ALshort *dst, alMixEntry *entries, ALuint numents);
void MixAudio16_SSE2_0( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_1( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_2( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_3( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_4( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_5( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_6( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_7( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_8( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_9( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_10( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_11( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_12( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_13( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_14( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_15( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_16( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_17( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_18( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_19( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_20( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_21( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_22( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_23( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_24( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_25( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_26( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_27( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_28( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_29( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_30( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_31( ALshort *dst, alMixEntry *entries );
void MixAudio16_SSE2_32( ALshort *dst, alMixEntry *entries );

#endif /* HAVE_SSE2 */

#endif /* not AL_ARCH_I386_MIXAUDIO16_SSE2_PRK_H_ */
