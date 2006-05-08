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
#ifndef AL_ARCH_I386_MIXAUDIO16_MMX_PRK_H_
#define AL_ARCH_I386_MIXAUDIO16_MMX_PRK_H_

#include "al_siteconfig.h"

#ifdef __MMX__
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

void MixAudio16_MMX_n(ALshort *dst, alMixEntry *entries, ALuint numents);
void MixAudio16_MMX_0( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_1( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_2( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_3( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_4( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_5( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_6( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_7( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_8( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_9( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_10( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_11( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_12( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_13( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_14( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_15( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_16( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_17( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_18( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_19( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_20( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_21( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_22( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_23( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_24( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_25( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_26( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_27( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_28( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_29( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_30( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_31( ALshort *dst, alMixEntry *entries );
void MixAudio16_MMX_32( ALshort *dst, alMixEntry *entries );

#endif /* __MMX__ */

#endif /* not AL_ARCH_I386_MIXAUDIO16_MMX_PRK_H_ */
