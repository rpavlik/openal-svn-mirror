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
#ifndef AL_ARCH_I386_X86_SIMD_SUPPORT_PRK_H_
#define AL_ARCH_I386_X86_SIMD_SUPPORT_PRK_H_

#include "al_siteconfig.h"

#ifdef __MMX__
/*
 * We use built-ins for gcc instead of Intel/MSVC style intrinsics
 * as (older) gccs are slower with them
 */
#if __GNUC__ && !__INTEL_COMPILER

#if __GNUC__ < 4
typedef short v4hi __attribute__ ((__mode__(__V4HI__)));
typedef int   v2si __attribute__ ((__mode__(__V2SI__)));
typedef int   di   __attribute__ ((__mode__(__DI__)));
#else /* __GNUC__ >= 4 */
typedef short v4hi __attribute__ ((vector_size (8)));
typedef int   v2si __attribute__ ((vector_size (8)));
typedef int   di   __attribute__ ((vector_size (8)));
#endif /* __GNUC__ >= 4 */

/* GCC 3.4 needs some explicit casts */
#define to_v4hi(X) (v4hi)X
#define to_v2si(X) (v2si)X
#define to_di(X)   (di)X

#define ALIGN16(x) x __attribute__((aligned(16)))
typedef unsigned long aint;

#else /* !__GNUC__ || __INTEL_COMPILER */

#include <mmintrin.h>
typedef __m64 v4hi;
typedef __m64 v2si;
typedef __m64 di;

/* MSVC++ forbids explicit casts */
#define to_v4hi(X) X
#define to_v2si(X) X
#define to_di(X)   X

#define __builtin_ia32_pand(X,Y)	_mm_and_si64(X,Y)
#define __builtin_ia32_pcmpeqw(X,Y)	_mm_cmpeq_pi16(X,Y)
#define __builtin_ia32_packssdw(X,Y)	_mm_packs_pi32(X,Y)
#define __builtin_ia32_punpcklwd(X,Y)	_mm_unpacklo_pi16(X,Y)
#define __builtin_ia32_punpckhwd(X,Y)	_mm_unpackhi_pi16(X,Y)
#define __builtin_ia32_paddd(X,Y)	_mm_add_pi32(X,Y)
#define __builtin_ia32_paddsw(X,Y)	_mm_adds_pi16(X,Y)
#define __builtin_ia32_pmulhw(X,Y)	_mm_mulhi_pi16(X,Y)
#define __builtin_ia32_psllw(X,Y)	_mm_slli_pi16(X,Y)
#define __builtin_ia32_emms() 		_mm_empty()

#define ALIGN16(x) __declspec(align(16)) x

/* FIXME: msvc++'s long in x86_64 isn't 8bytes? */
typedef unsigned long aint;

#endif /* !__GNUC__ || __INTEL_COMPILER */

#define MMX_ALIGN 8

#endif /* __MMX__ */

#endif /* not AL_ARCH_I386_X86_SIMD_SUPPORT_PRK_H_ */
