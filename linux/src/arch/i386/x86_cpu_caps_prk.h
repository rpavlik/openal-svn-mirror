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
#ifndef X86_CPU_CAPS_H
#define X86_CPU_CAPS_H

struct x86cpu_caps_s {
	int mmx;
	int sse;
	int sse2;
	int sse3;
	int amd_3dnow;
	int amd_3dnowext;
	int amd_sse_mmx;
	int cyrix_mmxext;
};

extern struct x86cpu_caps_s x86cpu_caps;
extern struct x86cpu_caps_s x86cpu_caps_use;

static __inline int _alHaveMMX(void);
static __inline int _alHaveSSE(void);
static __inline int _alHaveSSE2(void);
static __inline int _alHaveSSE3(void);
static __inline int _alHave3DNOW(void);
static __inline int _alHave3DNOWEXT(void);
static __inline int _alHaveSSEMMX(void);


static __inline int _alHaveMMX()
{
	return x86cpu_caps.mmx & x86cpu_caps_use.mmx;
}

static __inline int _alHaveSSE()
{
	return x86cpu_caps.sse & x86cpu_caps_use.sse;
}

static __inline int _alHaveSSE2()
{
	return x86cpu_caps.sse2 & x86cpu_caps_use.sse2;
}

static __inline int _alHaveSSE3()
{
	return x86cpu_caps.sse3 & x86cpu_caps_use.sse3;
}

static __inline int _alHave3DNOW()
{
	return x86cpu_caps.amd_3dnow & x86cpu_caps_use.amd_3dnow;
}

static __inline int _alHave3DNOWEXT()
{
	return x86cpu_caps.amd_3dnowext & x86cpu_caps_use.amd_3dnowext;
}

static __inline int _alHaveSSEMMX()
{
	return x86cpu_caps.amd_sse_mmx & x86cpu_caps_use.amd_sse_mmx;
}

#endif /* X86_CPU_CAPS_H */
