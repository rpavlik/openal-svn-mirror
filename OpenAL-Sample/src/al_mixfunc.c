/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_mixfunc.c
 *
 * Handling of the ALMixFunc datatype, which abstract the use of
 * specialzed mixing functions.  Mixing functions are specialized
 * on the bases of how many seperate source streams are contained
 * in an ALMixManager object, which determines a dispatch function.
 *
 */
#include "al_siteconfig.h"

#include <stdio.h>

#include "al_main.h"
#include "al_mixfunc.h"
#include "al_types.h"
#include "al_cpu_caps.h"
#include "mixaudio16.h"

/*
 * _alMixFuncInit( ALMixFunc *mf, ALuint size )
 *
 * _alMixFuncInit initializes the already allocated ALMixFunc object (mf) to
 * contain at least size mixing functions.
 */
ALboolean _alMixFuncInit( ALMixFunc *mf, ALuint size ) {
	if(mf == NULL) {
		return AL_FALSE;
	}

	if(size == 0) {
		return AL_FALSE;
	}

	if(size > MAXMIXSOURCES) {
		return AL_FALSE;
	}
	
#ifdef __SSE2__
	if (_alHaveSSE2()) {
		/* FIXME: we really ignore size. */
		mf->max      = MMXMIXSOURCES;
		mf->funcs[0] = MixAudio16_SSE2_0;
		mf->funcs[1] = MixAudio16_SSE2_1;
		mf->funcs[2] = MixAudio16_SSE2_2;
		mf->funcs[3] = MixAudio16_SSE2_3;
		mf->funcs[4] = MixAudio16_SSE2_4;
		mf->funcs[5] = MixAudio16_SSE2_5;
		mf->funcs[6] = MixAudio16_SSE2_6;
		mf->funcs[7] = MixAudio16_SSE2_7;
		mf->funcs[8] = MixAudio16_SSE2_8;
		mf->funcs[9] = MixAudio16_SSE2_9;
		
		mf->funcs[10] = MixAudio16_SSE2_10;
		mf->funcs[11] = MixAudio16_SSE2_11;
		mf->funcs[12] = MixAudio16_SSE2_12;
		mf->funcs[13] = MixAudio16_SSE2_13;
		mf->funcs[14] = MixAudio16_SSE2_14;
		mf->funcs[15] = MixAudio16_SSE2_15;
		mf->funcs[16] = MixAudio16_SSE2_16;
		mf->funcs[17] = MixAudio16_SSE2_17;
		mf->funcs[18] = MixAudio16_SSE2_18;
		mf->funcs[19] = MixAudio16_SSE2_19;
		
		mf->funcs[20] = MixAudio16_SSE2_20;
		mf->funcs[21] = MixAudio16_SSE2_21;
		mf->funcs[22] = MixAudio16_SSE2_22;
		mf->funcs[23] = MixAudio16_SSE2_23;
		mf->funcs[24] = MixAudio16_SSE2_24;
		mf->funcs[25] = MixAudio16_SSE2_25;
		mf->funcs[26] = MixAudio16_SSE2_26;
		mf->funcs[27] = MixAudio16_SSE2_27;
		mf->funcs[28] = MixAudio16_SSE2_28;
		mf->funcs[29] = MixAudio16_SSE2_29;
		
		mf->funcs[30] = MixAudio16_SSE2_30;
		mf->funcs[31] = MixAudio16_SSE2_31;
		mf->funcs[32] = MixAudio16_SSE2_32;
		
		mf->func_n = MixAudio16_SSE2_n;
		
		return AL_TRUE;
	}
#endif /* __SSE2__ */

#ifdef __MMX__
	if (_alHaveMMX()) {
		/* FIXME: we really ignore size. */
		mf->max      = MMXMIXSOURCES;
		mf->funcs[0] = MixAudio16_MMX_0;
		mf->funcs[1] = MixAudio16_MMX_1;
		mf->funcs[2] = MixAudio16_MMX_2;
		mf->funcs[3] = MixAudio16_MMX_3;
		mf->funcs[4] = MixAudio16_MMX_4;
		mf->funcs[5] = MixAudio16_MMX_5;
		mf->funcs[6] = MixAudio16_MMX_6;
		mf->funcs[7] = MixAudio16_MMX_7;
		mf->funcs[8] = MixAudio16_MMX_8;
		mf->funcs[9] = MixAudio16_MMX_9;
		
		mf->funcs[10] = MixAudio16_MMX_10;
		mf->funcs[11] = MixAudio16_MMX_11;
		mf->funcs[12] = MixAudio16_MMX_12;
		mf->funcs[13] = MixAudio16_MMX_13;
		mf->funcs[14] = MixAudio16_MMX_14;
		mf->funcs[15] = MixAudio16_MMX_15;
		mf->funcs[16] = MixAudio16_MMX_16;
		mf->funcs[17] = MixAudio16_MMX_17;
		mf->funcs[18] = MixAudio16_MMX_18;
		mf->funcs[19] = MixAudio16_MMX_19;
		
		mf->funcs[20] = MixAudio16_MMX_20;
		mf->funcs[21] = MixAudio16_MMX_21;
		mf->funcs[22] = MixAudio16_MMX_22;
		mf->funcs[23] = MixAudio16_MMX_23;
		mf->funcs[24] = MixAudio16_MMX_24;
		mf->funcs[25] = MixAudio16_MMX_25;
		mf->funcs[26] = MixAudio16_MMX_26;
		mf->funcs[27] = MixAudio16_MMX_27;
		mf->funcs[28] = MixAudio16_MMX_28;
		mf->funcs[29] = MixAudio16_MMX_29;
		
		mf->funcs[30] = MixAudio16_MMX_30;
		mf->funcs[31] = MixAudio16_MMX_31;
		mf->funcs[32] = MixAudio16_MMX_32;
	
		mf->func_n = MixAudio16_MMX_n;
	
		return AL_TRUE;
	}
#endif /* __MMX__ */

	/* FIXME: we really ignore size. */
	mf->max      = GENMIXSOURCES;
	mf->funcs[0] = MixAudio16_0;
	mf->funcs[1] = MixAudio16_1;
	mf->funcs[2] = MixAudio16_2;
	mf->funcs[3] = MixAudio16_3;
	mf->funcs[4] = MixAudio16_4;
	mf->funcs[5] = MixAudio16_5;
	mf->funcs[6] = MixAudio16_6;
	mf->funcs[7] = MixAudio16_7;
	mf->funcs[8] = MixAudio16_8;
#ifndef USE_LIGHT_GEN_MIXING
	mf->funcs[9] = MixAudio16_9;

	mf->funcs[10] = MixAudio16_10;
	mf->funcs[11] = MixAudio16_11;
	mf->funcs[12] = MixAudio16_12;
	mf->funcs[13] = MixAudio16_13;
	mf->funcs[14] = MixAudio16_14;
	mf->funcs[15] = MixAudio16_15;
	mf->funcs[16] = MixAudio16_16;
	mf->funcs[17] = MixAudio16_17;
	mf->funcs[18] = MixAudio16_18;
	mf->funcs[19] = MixAudio16_19;

	mf->funcs[20] = MixAudio16_20;
	mf->funcs[21] = MixAudio16_21;
	mf->funcs[22] = MixAudio16_22;
	mf->funcs[23] = MixAudio16_23;
	mf->funcs[24] = MixAudio16_24;
	mf->funcs[25] = MixAudio16_25;
	mf->funcs[26] = MixAudio16_26;
	mf->funcs[27] = MixAudio16_27;
	mf->funcs[28] = MixAudio16_28;
	mf->funcs[29] = MixAudio16_29;
	
	mf->funcs[30] = MixAudio16_30;
	mf->funcs[31] = MixAudio16_31;
	mf->funcs[32] = MixAudio16_32;
	mf->funcs[33] = MixAudio16_33;
	mf->funcs[34] = MixAudio16_34;
	mf->funcs[35] = MixAudio16_35;
	mf->funcs[36] = MixAudio16_36;
	mf->funcs[37] = MixAudio16_37;
	mf->funcs[38] = MixAudio16_38;
	mf->funcs[39] = MixAudio16_39;
	mf->funcs[40] = MixAudio16_40;

	mf->funcs[41] = MixAudio16_41;
	mf->funcs[42] = MixAudio16_42;
	mf->funcs[43] = MixAudio16_43;
	mf->funcs[44] = MixAudio16_44;
	mf->funcs[45] = MixAudio16_45;
	mf->funcs[46] = MixAudio16_46;
	mf->funcs[47] = MixAudio16_47;
	mf->funcs[48] = MixAudio16_48;
	mf->funcs[49] = MixAudio16_49;
	
	mf->funcs[50] = MixAudio16_50;
	mf->funcs[51] = MixAudio16_51;
	mf->funcs[52] = MixAudio16_52;
	mf->funcs[53] = MixAudio16_53;
	mf->funcs[54] = MixAudio16_54;
	mf->funcs[55] = MixAudio16_55;
	mf->funcs[56] = MixAudio16_56;
	mf->funcs[57] = MixAudio16_57;
	mf->funcs[58] = MixAudio16_58;
	mf->funcs[59] = MixAudio16_59;
	
	mf->funcs[60] = MixAudio16_60;
	mf->funcs[61] = MixAudio16_61;
	mf->funcs[62] = MixAudio16_62;
	mf->funcs[63] = MixAudio16_63;
	mf->funcs[64] = MixAudio16_64;
#endif /* !USE_LIGHT_GEN_MIXING */
	mf->func_n = MixAudio16_n;

	return AL_TRUE;
}

/*
 * _alMixFuncDestroy( ALMixFunc *mf );
 *
 * _alMixFuncDestroy performs any needed finalization on the ALMixFunc object
 * mf.
 */
void _alMixFuncDestroy( UNUSED(ALMixFunc *mf) ) {

	return;
}
