/**********************************************************************************************************************************
*
*   OpenAL cross platform audio library
*   Copyright © 2004, Apple Computer, Inc. All rights reserved.
*
*   Redistribution and use in source and binary forms, with or without modification, are permitted provided 
*   that the following conditions are met:
*
*   1.  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. 
*   2.  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
*       disclaimer in the documentation and/or other materials provided with the distribution. 
*   3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of its contributors may be used to endorse or promote 
*       products derived from this software without specific prior written permission. 
*
*   THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
*   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS 
*   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
*   TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
*   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE 
*   USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
**********************************************************************************************************************************/

#ifndef __OAL_IMP__
#define __OAL_IMP__

#include "al.h"
#include "alc.h"
#include "oalOSX.h"
#include <Carbon/Carbon.h>
#include <map>

#ifdef __cplusplus
extern "C" {
#endif

#define AL_STATIC                                 0x1028
#define AL_STREAMING                              0x1029
#define AL_UNDETERMINED                           0x1030
	
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Continue exporting these deprectaed APIs to prevent runtime link errors

#ifdef TARGET_OS_MAC
   #if TARGET_OS_MAC
       #pragma export on
   #endif
#endif


#ifdef TARGET_OS_MAC
   #if TARGET_OS_MAC
      #pragma export off
   #endif
#endif

#ifdef __cplusplus
}
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// development build flags
#define	USE_AU_TRACER 					0
#define LOG_GRAPH_AND_MIXER_CHANGES		0
#define GET_OVERLOAD_NOTIFICATIONS 		0
#define	LOG_IO 							0

#if LOG_IO
	#include  "AudioLogger.h"
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define AL_MAXBUFFERS 1024	
#define AL_MAXSOURCES 256 

#define kDefaultMaximumMixerBusCount    64
#define kDopplerDefault                 0	

#define	THROW_RESULT		if(result != noErr) throw static_cast<OSStatus>(result);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void	alSetError (ALenum errorCode);
void	SetPreferred3DMixerPresent (bool inPreferredMixerIsPresent);
bool	IsPreferred3DMixerPresent ();

#endif

