/**
 * OpenAL cross platform audio library
 * Copyright (C) 1999-2003 by authors.
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




#include <al\alc.h>
#include "OpenAL32.h"


//*****************************************************************************
//*****************************************************************************
//
// Defines
//
//*****************************************************************************
//*****************************************************************************

#define AL_RESULT_FXN(fxn, resultType, resultDefVal)                        \
    resultType result = resultDefVal;                                       \
    ALCcontext* context;                                                     \
                                                                            \
    alListAcquireLock(alContextList);                                       \
    if(!alCurrentContext)                                                   \
    {                                                                       \
        alListReleaseLock(alContextList);                                   \
        return result;                                                      \
    }                                                                       \
                                                                            \
    context = alCurrentContext;                                             \
    EnterCriticalSection(&context->Lock);                                   \
    alListReleaseLock(alContextList);                                       \
                                                                            \
    result = context->AlApi.##fxn;                                          \
    LeaveCriticalSection(&context->Lock);                                   \
    return result


#define AL_VOID_FXN(fxn)                                                    \
    ALCcontext* context;                                                     \
                                                                            \
    alListAcquireLock(alContextList);                                       \
    if(!alCurrentContext)                                                   \
    {                                                                       \
        alListReleaseLock(alContextList);                                   \
        return;                                                             \
    }                                                                       \
                                                                            \
    context = alCurrentContext;                                             \
    EnterCriticalSection(&context->Lock);                                   \
    alListReleaseLock(alContextList);                                       \
                                                                            \
    context->AlApi.##fxn;                                                   \
    LeaveCriticalSection(&context->Lock);                                   \
    return




//*****************************************************************************
//*****************************************************************************
//
// AL API Buffer Entry Points
//
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
// alGenBuffers
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alGenBuffers(ALsizei n, ALuint* bufferNames)
{
    AL_VOID_FXN(alGenBuffers(n, bufferNames));
}


//*****************************************************************************
// alDeleteBuffers
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alDeleteBuffers(ALsizei n, const ALuint* bufferNames)
{
    AL_VOID_FXN(alDeleteBuffers(n, bufferNames));
}


//*****************************************************************************
// alIsBuffer
//*****************************************************************************
//
ALAPI ALboolean ALAPIENTRY alIsBuffer(ALuint bufferName)
{
    AL_RESULT_FXN(alIsBuffer(bufferName), ALboolean, AL_FALSE);
}


//*****************************************************************************
// alBufferData
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alBufferData(ALuint bufferName, ALenum format, const ALvoid* data, ALsizei size, ALsizei freq)
{
    AL_VOID_FXN(alBufferData(bufferName, format, data, size, freq));
}


//*****************************************************************************
// alGetBufferf
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alGetBufferf(ALuint bufferName, ALenum param, ALfloat* value)
{
    AL_VOID_FXN(alGetBufferf(bufferName, param, value));
}


//*****************************************************************************
// alGetBufferi
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alGetBufferi(ALuint bufferName, ALenum param, ALint* value)
{
    AL_VOID_FXN(alGetBufferi(bufferName, param, value));
}


//*****************************************************************************
//*****************************************************************************
//
// AL API Generic Entry Points
//
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
// alEnable
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alEnable(ALenum capability)
{
    AL_VOID_FXN(alEnable(capability));
}


//*****************************************************************************
// alDisable
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alDisable(ALenum capability)
{
    AL_VOID_FXN(alDisable(capability));
}


//*****************************************************************************
// alDopplerFactor
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alDopplerFactor(ALfloat value)
{
    AL_VOID_FXN(alDopplerFactor(value));
}


//*****************************************************************************
// alDopplerVelocity
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alDopplerVelocity(ALfloat value)
{
    AL_VOID_FXN(alDopplerVelocity(value));
}


//*****************************************************************************
// alDistanceModel
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alDistanceModel(ALenum value)
{
    AL_VOID_FXN(alDistanceModel(value));
}


//*****************************************************************************
// alGetBoolean
//*****************************************************************************
//
ALAPI ALboolean ALAPIENTRY alGetBoolean(ALenum param)
{
    AL_RESULT_FXN(alGetBoolean(param), ALboolean, AL_FALSE);
}


//*****************************************************************************
// alGetBooleanv
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alGetBooleanv(ALenum param, ALboolean* data)
{
    AL_VOID_FXN(alGetBooleanv(param, data));
}


//*****************************************************************************
// alGetDouble
//*****************************************************************************
//
ALAPI ALdouble ALAPIENTRY alGetDouble(ALenum param)
{
    AL_RESULT_FXN(alGetDouble(param), ALdouble, 0.0);
}


//*****************************************************************************
// alGetDoublev
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alGetDoublev(ALenum param, ALdouble* data)
{
    AL_VOID_FXN(alGetDoublev(param, data));
}

//*****************************************************************************
// alGetFloat
//*****************************************************************************
//
ALAPI ALfloat ALAPIENTRY alGetFloat(ALenum param)
{
    AL_RESULT_FXN(alGetFloat(param), ALfloat, 0.0f);
}


//*****************************************************************************
// alGetFloatv
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alGetFloatv(ALenum param, ALfloat* data)
{
    AL_VOID_FXN(alGetFloatv(param, data));
}


//*****************************************************************************
// alGetInteger
//*****************************************************************************
//
ALAPI ALint ALAPIENTRY alGetInteger(ALenum param)
{
    AL_RESULT_FXN(alGetInteger(param), ALint, 0);
}


//*****************************************************************************
// alGetIntegerv
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alGetIntegerv(ALenum param, ALint* data)
{
    AL_VOID_FXN(alGetIntegerv(param, data));
}


//*****************************************************************************
// alGetEnumValue
//*****************************************************************************
//
ALAPI ALenum ALAPIENTRY alGetEnumValue(const ALubyte* ename)
{
    AL_RESULT_FXN(alGetEnumValue(ename), ALenum, AL_INVALID_ENUM);
}


//*****************************************************************************
// alGetError
//*****************************************************************************
//
ALAPI ALenum ALAPIENTRY alGetError(ALvoid)
{
    AL_RESULT_FXN(alGetError(), ALenum, AL_NO_ERROR);
}


//*****************************************************************************
// alGetProcAddress
//*****************************************************************************
//
ALAPI ALvoid* ALAPIENTRY alGetProcAddress(const ALubyte* fname)
{
    AL_RESULT_FXN(alGetProcAddress(fname), ALvoid*, 0);
}


//*****************************************************************************
// alGetString
//*****************************************************************************
//
ALAPI const ALubyte* ALAPIENTRY alGetString(ALenum param)
{
    AL_RESULT_FXN(alGetString(param), const ALubyte*, 0);
}


//*****************************************************************************
// alIsExtensionPresent
//*****************************************************************************
//
ALAPI ALboolean ALAPIENTRY alIsExtensionPresent(const ALubyte* ename)
{
    AL_RESULT_FXN(alIsExtensionPresent(ename), ALboolean, AL_FALSE);
}


//*****************************************************************************
// alIsEnabled
//*****************************************************************************
//
ALAPI ALboolean ALAPIENTRY alIsEnabled(ALenum capability)
{
    AL_RESULT_FXN(alIsEnabled(capability), ALboolean, AL_FALSE);
}



//*****************************************************************************
//*****************************************************************************
//
// AL API Listener Entry Points
//
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
// alListenerf
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alListenerf(ALenum param, ALfloat value)
{
    AL_VOID_FXN(alListenerf(param, value));
}


//*****************************************************************************
// alListener3f
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alListener3f(ALenum param, ALfloat v1, ALfloat v2, ALfloat v3)
{
    AL_VOID_FXN(alListener3f(param, v1, v2, v3));
}


//*****************************************************************************
// alListenerfv
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alListenerfv(ALenum param, const ALfloat* values)
{
    AL_VOID_FXN(alListenerfv(param, values));
}


//*****************************************************************************
// alListeneri
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alListeneri(ALenum param, ALint value)
{
    AL_VOID_FXN(alListeneri(param, value));
}


//*****************************************************************************
// alGetListenerf
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alGetListenerf(ALenum param, ALfloat* value)
{
    AL_VOID_FXN(alGetListenerf(param, value));
}


//*****************************************************************************
// alGetListenerf
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alGetListener3f(ALenum param, ALfloat* v1, ALfloat* v2, ALfloat* v3)
{
    AL_VOID_FXN(alGetListener3f(param, v1, v2, v3));
}


//*****************************************************************************
// alGetListenerfv
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alGetListenerfv(ALenum param, ALfloat* values)
{
    AL_VOID_FXN(alGetListenerfv(param, values));
}


//*****************************************************************************
// alGetListeneri
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alGetListeneri(ALenum param, ALint* value)
{
    AL_VOID_FXN(alGetListeneri(param, value));
}



//*****************************************************************************
//*****************************************************************************
//
// AL API Source Entry Points
//
//*****************************************************************************
//*****************************************************************************


//*****************************************************************************
// alGenSources
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alGenSources(ALsizei n, ALuint* sourceNames)
{
    AL_VOID_FXN(alGenSources(n, sourceNames));
}


//*****************************************************************************
// alDeleteSources
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alDeleteSources(ALsizei n, const ALuint* sourceNames)
{
    AL_VOID_FXN(alDeleteSources(n, sourceNames));
}


//*****************************************************************************
// alIsSource
//*****************************************************************************
//
ALAPI ALboolean ALAPIENTRY alIsSource(ALuint sourceName)
{
    AL_RESULT_FXN(alIsSource(sourceName), ALboolean, AL_FALSE);
}


//*****************************************************************************
// alSourcef
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alSourcef(ALuint sourceName, ALenum param, ALfloat value)
{
    AL_VOID_FXN(alSourcef(sourceName, param, value));
}


//*****************************************************************************
// alSourcefv
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alSourcefv(ALuint sourceName, ALenum param, const ALfloat* values)
{
    AL_VOID_FXN(alSourcefv(sourceName, param, values));
}


//*****************************************************************************
// alSource3f
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alSource3f(ALuint sourceName, ALenum param, ALfloat v1, ALfloat v2, ALfloat v3)
{
    AL_VOID_FXN(alSource3f(sourceName, param, v1, v2, v3));
}


//*****************************************************************************
// alSourcei
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alSourcei(ALuint sourceName, ALenum param, ALint value)
{
    AL_VOID_FXN(alSourcei(sourceName, param, value));
}


//*****************************************************************************
// alGetSourcef
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alGetSourcef(ALuint sourceName, ALenum param, ALfloat* value)
{
    AL_VOID_FXN(alGetSourcef(sourceName, param, value));
}

//*****************************************************************************
// alGetSource3f
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alGetSource3f(ALuint sourceName, ALenum param, ALfloat* v1, ALfloat* v2, ALfloat* v3)
{
    AL_VOID_FXN(alGetSource3f(sourceName, param, v1, v2, v3));
}


//*****************************************************************************
// alGetSourcefv
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alGetSourcefv(ALuint sourceName, ALenum param, ALfloat* values)
{
    AL_VOID_FXN(alGetSourcefv(sourceName, param, values));
}


//*****************************************************************************
// alGetSourcei
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alGetSourcei(ALuint sourceName, ALenum param, ALint* value)
{
    AL_VOID_FXN(alGetSourcei(sourceName, param, value));
}


//*****************************************************************************
// alSourcePlay
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alSourcePlay(ALuint sourceName)
{
    AL_VOID_FXN(alSourcePlay(sourceName));
}


//*****************************************************************************
// alSourcePlayv
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alSourcePlayv(ALsizei n, const ALuint* sourceNames)
{
    AL_VOID_FXN(alSourcePlayv(n, sourceNames));
}


//*****************************************************************************
// alSourcePause
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alSourcePause(ALuint sourceName)
{
    AL_VOID_FXN(alSourcePause(sourceName));
}


//*****************************************************************************
// alSourcePausev
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alSourcePausev(ALsizei n, const ALuint* sourceNames)
{
    AL_VOID_FXN(alSourcePausev(n, sourceNames));
}


//*****************************************************************************
// alSourceStop
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alSourceStop(ALuint sourceName)
{
    AL_VOID_FXN(alSourceStop(sourceName));
}


//*****************************************************************************
// alSourceStopv
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alSourceStopv(ALsizei n, const ALuint* sourceNames)
{
    AL_VOID_FXN(alSourceStopv(n, sourceNames));
}


//*****************************************************************************
// alSourceRewind
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alSourceRewind(ALuint sourceName)
{
    AL_VOID_FXN(alSourceRewind(sourceName));
}


//*****************************************************************************
// alSourceRewindv
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alSourceRewindv(ALsizei n, const ALuint* sourceNames)
{
    AL_VOID_FXN(alSourceRewindv(n, sourceNames));
}


//*****************************************************************************
// alSourceQueueBuffers
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alSourceQueueBuffers(ALuint sourceName, ALsizei n, const ALuint* buffers)
{
    AL_VOID_FXN(alSourceQueueBuffers(sourceName, n, buffers));
}


//*****************************************************************************
// alSourceUnqueueBuffers
//*****************************************************************************
//
ALAPI ALvoid ALAPIENTRY alSourceUnqueueBuffers(ALuint sourceName, ALsizei n, ALuint* buffers)
{
    AL_VOID_FXN(alSourceUnqueueBuffers(sourceName, n, buffers));
}

