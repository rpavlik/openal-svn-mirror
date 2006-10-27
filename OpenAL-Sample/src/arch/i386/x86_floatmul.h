/***************************************************************************
 *   Copyright (C) 2006 by Prakash Punnoor                                 *
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
#ifndef X86_FLOATMUL_H
#define X86_FLOATMUL_H

#include "al_siteconfig.h"

#include <AL/al.h>

void _alFloatMul_portable(ALshort *bpt, ALfloat sa, ALuint len);

#ifdef HAVE_MMX
void _alFloatMul_MMX(ALshort *bpt, ALfloat sa, ALuint len);
#endif /* HAVE_MMX */

#ifdef HAVE_SSE2
void _alFloatMul_SSE2(ALshort *bpt, ALfloat sa, ALuint len);
#endif /* HAVE_SSE2 */

#endif /* X86_FLOATMUL_H */
