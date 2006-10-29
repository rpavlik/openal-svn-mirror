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

#if defined(HAVE_MMX) || defined(HAVE_SSE2)
/*
 * We use built-ins for gcc instead of Intel/MSVC style intrinsics
 * as (older) gccs are slower with them
 */
#if __GNUC__ && !__INTEL_COMPILER

#if __GNUC__ < 4
typedef short v4hi __attribute__ ((__mode__(__V4HI__)));
typedef int   v2si __attribute__ ((__mode__(__V2SI__)));

typedef short v8hi __attribute__ ((__mode__(__V8HI__)));
typedef int   v4si __attribute__ ((__mode__(__V4SI__)));
#else /* __GNUC__ >= 4 */
typedef short v4hi __attribute__ ((vector_size (8)));
typedef int   v2si __attribute__ ((vector_size (8)));

typedef short   v8hi __attribute__ ((vector_size (16)));
typedef int     v4si __attribute__ ((vector_size (16)));
#endif /* __GNUC__ >= 4 */

/* GCC 3.4 needs some explicit casts */
#define to_v4hi(X) (v4hi)X
#define to_v2si(X) (v2si)X

#define to_v8hi(X) (v8hi)X
#define to_v4si(X) (v4si)X

typedef char *psse2loadtype;

#if __GNUC__ < 4
static __inline v4hi setw(short X)
{
	union {
		short s[4];
		v4hi v;
	} u = {{X, X, X, X}};

	return u.v;
}

#ifdef __SSE__
static __inline v8hi setw128(short X)
{
	union {
		short s[8];
		v8hi v;
	} u = {{X, X, X, X, X, X, X, X}};

	return u.v;
}
#endif /* __SSE__ */
#else
#define setw(X) (__builtin_ia32_vec_init_v4hi(X, X, X, X))
#define setw128(X) (__extension__(v8hi){X, X, X, X, X, X, X, X})
#endif

#define ALIGN16(x) x __attribute__((aligned(16)))
typedef unsigned long aint;

#else /* __GNUC__ && !__INTEL_COMPILER */

#ifdef HAVE_MMX
#include <mmintrin.h>
typedef __m64 v4hi;
typedef __m64 v2si;
typedef __m64 di;

/* MSVC++ forbids explicit casts */
#define to_v4hi(X) X
#define to_v2si(X) X

#define __builtin_ia32_packssdw(X,Y)	_mm_packs_pi32(X,Y)
#define __builtin_ia32_punpcklwd(X,Y)	_mm_unpacklo_pi16(X,Y)
#define __builtin_ia32_punpckhwd(X,Y)	_mm_unpackhi_pi16(X,Y)
#define __builtin_ia32_paddd(X,Y)	_mm_add_pi32(X,Y)
#define __builtin_ia32_paddsw(X,Y)	_mm_adds_pi16(X,Y)
#define __builtin_ia32_pmulhw(X,Y)	_mm_mulhi_pi16(X,Y)
#define __builtin_ia32_psllw(X,Y)	_mm_slli_pi16(X,Y)
#define __builtin_ia32_psraw(X,Y)	_mm_srai_pi16(X,Y)
#define __builtin_ia32_emms() 		_mm_empty()

#define setw(X)	_mm_set_pi16(X, X, X, X)
#endif /* HAVE_MMX */

#ifdef HAVE_SSE2
#include <emmintrin.h>
typedef __m128i v8hi;
typedef __m128i v4si;
typedef __m128i v2di;
typedef __m128i *psse2loadtype;

/* MSVC++ forbids explicit casts */
#define to_v8hi(X) X
#define to_v4si(X) X

#define __builtin_ia32_loaddqu(X)	_mm_loadu_si128(X)
#define __builtin_ia32_packssdw128(X,Y)	_mm_packs_epi32(X,Y)
#define __builtin_ia32_punpcklwd128(X,Y)	_mm_unpacklo_epi16(X,Y)
#define __builtin_ia32_punpckhwd128(X,Y)	_mm_unpackhi_epi16(X,Y)
#define __builtin_ia32_paddd128(X,Y)	_mm_add_epi32(X,Y)
#define __builtin_ia32_paddsw128(X,Y)	_mm_adds_epi16(X,Y)
#define __builtin_ia32_pmulhw128(X,Y)	_mm_mulhi_epi16(X,Y)
#define __builtin_ia32_psllwi128(X,Y)	_mm_slli_epi16(X,Y)
#define __builtin_ia32_psrawi128(X,Y)	_mm_srai_epi16(X,Y)

#define setw128(X)	_mm_set_epi16(X, X, X, X, X, X, X, X)
#endif /* HAVE_SSE2 */

#define ALIGN16(x) __declspec(align(16)) x

/* FIXME: msvc++'s long in x86_64 isn't 8bytes? */
typedef unsigned long aint;

#endif /* __GNUC__ && !__INTEL_COMPILER */

#define MMX_ALIGN 8
#define SSE2_ALIGN 16

#endif /* HAVE_MMX || HAVE_SSE2*/

#endif /* not AL_ARCH_I386_X86_SIMD_SUPPORT_PRK_H_ */
