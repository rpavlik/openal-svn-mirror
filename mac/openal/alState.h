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
 
#ifndef _STATE_H_
#define _STATE_H_

#pragma export on

// functions
ALAPI ALvoid ALAPIENTRY alEnable (ALenum capability);
ALAPI ALvoid ALAPIENTRY alDisable (ALenum capability);
ALAPI ALboolean ALAPIENTRY alIsEnabled(ALenum capability);
ALAPI ALboolean ALAPIENTRY alGetBoolean (ALenum pname);
ALAPI ALdouble ALAPIENTRY alGetDouble (ALenum pname);
ALAPI ALfloat ALAPIENTRY alGetFloat (ALenum pname);
ALAPI ALint ALAPIENTRY alGetInteger (ALenum pname);
ALAPI ALvoid ALAPIENTRY alGetBooleanv (ALenum pname, ALboolean *data);
ALAPI ALvoid ALAPIENTRY alGetDoublev (ALenum pname, ALdouble *data);
ALAPI ALvoid ALAPIENTRY alGetFloatv (ALenum pname, ALfloat *data);
ALAPI ALvoid ALAPIENTRY alGetIntegerv (ALenum pname, ALint *data);
ALAPI const ALubyte * ALAPIENTRY alGetString (ALenum pname);
ALAPI ALvoid ALAPIENTRY alDopplerFactor (ALfloat value);
ALAPI ALvoid ALAPIENTRY alDopplerVelocity (ALfloat value);
ALAPI ALvoid ALAPIENTRY alDistanceScale (ALfloat value);
ALAPI ALvoid ALAPIENTRY alPropagationSpeed (ALfloat value);
ALAPI ALvoid ALAPIENTRY	alDistanceModel (ALenum value);

#pragma export off

#endif

