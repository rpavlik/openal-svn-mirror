/***************************************************************************
 *   Copyright (C) 2005 - 2006 by Prakash Punnoor                          *
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
#include <string.h>
#include <stdlib.h>
#include "al_cpu_caps.h"
#include "al_debug.h"

typedef unsigned int uint;
int _alDetectx86CPUCaps(uint* caps1, uint* caps2, uint* caps3);

/* caps1 */
#define MMX_BIT             23
#define SSE_BIT             25
#define SSE2_BIT            26

/* caps2 */
#define SSE3_BIT             0
#define SSE4_BIT             9

/* caps3 */
#define	AMD_3DNOW_BIT       31
#define	AMD_3DNOWEXT_BIT    30
#define AMD_SSE_MMX_BIT     22
#define CYRIX_MMXEXT_BIT    24


struct x86cpu_caps_s x86cpu_caps = { 0, 0, 0, 0, 0, 0, 0, 0, 0};
struct x86cpu_caps_s x86cpu_caps_use = { 1, 1, 1, 1, 1, 1, 1, 1, 1};

void _alDetectCPUCaps(void)
{
	/* compiled in SIMD routines */
#ifdef HAVE_MMX
	x86cpu_caps.mmx = 1;
#endif
#ifdef HAVE_SSE
	x86cpu_caps.sse = 1;
#endif
#ifdef HAVE_SSE2
	x86cpu_caps.sse2 = 1;
#endif
#ifdef HAVE_SSE3
	x86cpu_caps.sse3 = 1;
#endif
#ifdef HAVE_SSE4
	x86cpu_caps.sse4 = 1;
#endif
#ifdef HAVE_3DNOW
	x86cpu_caps.amd_3dnow = 1;
#endif
#ifdef HAVE_SSE_MMX
	x86cpu_caps.amd_sse_mmx = 1;
#endif
#ifdef HAVE_3DNOWEXT
	x86cpu_caps.amd_3dnowext = 1;
#endif
	/* compiled in SIMD routines */
	
	/* runtime detection */
#ifdef HAVE_CPU_CAPS_DETECTION
	{
		uint caps1, caps2, caps3;
		
		if (_alDetectx86CPUCaps(&caps1, &caps2, &caps3)) {
			
			x86cpu_caps.mmx &= (caps1 >> MMX_BIT) & 1;
			x86cpu_caps.sse &= (caps1 >> SSE_BIT) & 1;
			x86cpu_caps.sse2 &= (caps1 >> SSE2_BIT) & 1;
			
			x86cpu_caps.sse3 &= (caps2 >> SSE3_BIT) & 1;
			x86cpu_caps.sse4 &= (caps2 >> SSE4_BIT) & 1;
			
			x86cpu_caps.amd_3dnow &= (caps3 >> AMD_3DNOW_BIT) & 1;
			x86cpu_caps.amd_3dnowext &= (caps3 >> AMD_3DNOWEXT_BIT) & 1;
			x86cpu_caps.amd_sse_mmx &= (caps3 >> AMD_SSE_MMX_BIT) & 1;
			/* FIXME: For Cyrix MMXEXT detect Cyrix CPU first! */
			/*
			x86cpu_caps.cyrix_mmxext = (caps3 >> CYRIX_MMXEXT_BIT) & 1;
			*/
		}
	}
#endif /*HAVE_CPU_CAPS_DETECTION*/
	/* end runtime detection */
	
	/* check environment vars */
	{
		char *env;
		
		env = getenv("OPENAL_DISABLE_MMX");
		if (env)
			x86cpu_caps_use.mmx = !atoi(env);
		x86cpu_caps_use.mmx &= x86cpu_caps.mmx;
		
		env = getenv("OPENAL_DISABLE_SSE");
		if (env)
			x86cpu_caps_use.sse = !atoi(env);
		x86cpu_caps_use.sse &= x86cpu_caps.sse;
		
		env = getenv("OPENAL_DISABLE_SSE2");
		if (env)
			x86cpu_caps_use.sse2 = !atoi(env);
		x86cpu_caps_use.sse2 &= x86cpu_caps.sse2;
		
		env = getenv("OPENAL_DISABLE_SSE3");
		if (env)
			x86cpu_caps_use.sse3 = !atoi(env);
		x86cpu_caps_use.sse3 &= x86cpu_caps.sse3;
		
		env = getenv("OPENAL_DISABLE_SSE4");
		if (env)
			x86cpu_caps_use.sse4 = !atoi(env);
		x86cpu_caps_use.sse4 &= x86cpu_caps.sse4;
		
		env = getenv("OPENAL_DISABLE_3DNOW");
		if (env)
			x86cpu_caps_use.amd_3dnow = !atoi(env);
		x86cpu_caps_use.amd_3dnow &= x86cpu_caps.amd_3dnow;
		
		env = getenv("OPENAL_DISABLE_3DNOWEXT");
		if (env)
			x86cpu_caps_use.amd_3dnowext = !atoi(env);
		x86cpu_caps_use.amd_3dnowext &= x86cpu_caps.amd_3dnowext;
		
		env = getenv("OPENAL_DISABLE_SSE_MMX");
		if (env)
			x86cpu_caps_use.amd_sse_mmx = !atoi(env);
		x86cpu_caps_use.amd_sse_mmx &= x86cpu_caps.amd_sse_mmx;
		
		env = getenv("OPENAL_DISABLE_SIMD");
		if (env  &&  atoi(env))
			memset(&x86cpu_caps_use, 0, sizeof x86cpu_caps_use);
	}
	/* end check environment vars */

	_alDebug(ALD_CONFIG, __FILE__, __LINE__,"mmx found %i  use %i",
	         x86cpu_caps.mmx, x86cpu_caps_use.mmx);
	_alDebug(ALD_CONFIG, __FILE__, __LINE__,"sse found %i  use %i",
	         x86cpu_caps.sse, x86cpu_caps_use.sse);
	_alDebug(ALD_CONFIG, __FILE__, __LINE__,"sse2 found %i  use %i",
	         x86cpu_caps.sse2, x86cpu_caps_use.sse2);
	_alDebug(ALD_CONFIG, __FILE__, __LINE__,"sse3 found %i  use %i",
	         x86cpu_caps.sse3, x86cpu_caps_use.sse3);
	_alDebug(ALD_CONFIG, __FILE__, __LINE__,"sse4 found %i  use %i",
			 x86cpu_caps.sse4, x86cpu_caps_use.sse4);
	_alDebug(ALD_CONFIG, __FILE__, __LINE__,"amd_3dnow found %i  use %i",
	         x86cpu_caps.amd_3dnow, x86cpu_caps_use.amd_3dnow);
	_alDebug(ALD_CONFIG, __FILE__, __LINE__,"amd_3dnowext found %i  use %i",
	         x86cpu_caps.amd_3dnowext, x86cpu_caps_use.amd_3dnowext);
	_alDebug(ALD_CONFIG, __FILE__, __LINE__,"amd_sse_mmx found %i  use %i",
	         x86cpu_caps.amd_sse_mmx, x86cpu_caps_use.amd_sse_mmx);
	/*
	_alDebug(ALD_CONFIG, __FILE__, __LINE__,"cyrix_mmxext found %i  use %i",
	         x86cpu_caps.cyrix_mmxext, x86cpu_caps_use.cyrix_mmxext);
	*/

}
