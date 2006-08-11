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
#ifndef AL_ARCH_I386_X86_CPU_CAPS_PRK_H_
#define AL_ARCH_I386_X86_CPU_CAPS_PRK_H_

#include "al_siteconfig.h"

struct x86cpu_caps_s {
	int mmx;
	int sse;
	int sse2;
	int sse3;
	int sse4;
	int amd_3dnow;
	int amd_3dnowext;
	int amd_sse_mmx;
	int cyrix_mmxext;
};

extern struct x86cpu_caps_s x86cpu_caps_use;

static __inline int _alHaveMMX(void);
static __inline int _alHaveSSE(void);
static __inline int _alHaveSSE2(void);
static __inline int _alHaveSSE3(void);
static __inline int _alHaveSSE4(void);
static __inline int _alHave3DNOW(void);
static __inline int _alHave3DNOWEXT(void);
static __inline int _alHaveSSEMMX(void);


static __inline int _alHaveMMX(void)
{
	return x86cpu_caps_use.mmx;
}

static __inline int _alHaveSSE(void)
{
	return x86cpu_caps_use.sse;
}

static __inline int _alHaveSSE2(void)
{
	return x86cpu_caps_use.sse2;
}

static __inline int _alHaveSSE3(void)
{
	return x86cpu_caps_use.sse3;
}

static __inline int _alHaveSSE4(void)
{
	return x86cpu_caps_use.sse4;
}


static __inline int _alHave3DNOW(void)
{
	return x86cpu_caps_use.amd_3dnow;
}

static __inline int _alHave3DNOWEXT(void)
{
	return x86cpu_caps_use.amd_3dnowext;
}

static __inline int _alHaveSSEMMX(void)
{
	return x86cpu_caps_use.amd_sse_mmx;
}

#endif /* not AL_ARCH_I386_X86_CPU_CAPS_PRK_H_ */
