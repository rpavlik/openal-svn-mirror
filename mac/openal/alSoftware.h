/**
 * OpenAL cross platform audio library
 * Copyright (C) 1999-2000 by authors.
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA  02111-1307, USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */

#ifndef _SW_H_
#define _SW_H_

ALAPI ALint	ALAPIENTRY alF2L(ALfloat value);
ALAPI ALshort	ALAPIENTRY alF2S(ALfloat value);
ALAPI ALvoid	ALAPIENTRY alCrossproduct(ALfloat *inVector1,ALfloat *inVector2,ALfloat *outVector);
ALAPI ALfloat	ALAPIENTRY alDotproduct(ALfloat *inVector1,ALfloat *inVector2);
ALAPI ALvoid	ALAPIENTRY alNormalize(ALfloat *inVector);
ALAPI ALvoid	ALAPIENTRY alMatrixVector(ALfloat *vector,ALfloat matrix[3][3]);
ALAPI ALvoid	ALAPIENTRY alCalculateSourceParameters(ALuint source,ALuint channels,ALfloat *drysend,ALfloat *wetsend,ALfloat *pitch);

#endif
