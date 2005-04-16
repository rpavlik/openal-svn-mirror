#ifndef ALC_CONTEXT_H_
#define ALC_CONTEXT_H_

#include "altypes.h"
#include "alctypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ALC_VERSION_0_1         1

#ifdef _WIN32
 typedef struct ALCdevice_struct ALCdevice;
 typedef struct ALCcontext_struct ALCcontext;
 #ifndef _XBOX
  #ifdef _OPENAL32LIB
   #define ALCAPI __declspec(dllexport)
  #else
   #define ALCAPI __declspec(dllimport)
  #endif
  #define ALCAPIENTRY __cdecl
 #endif
#endif

#ifdef TARGET_OS_MAC
 #if TARGET_OS_MAC
  #pragma export on
 #endif
#endif

#ifndef ALCAPI
 #define ALCAPI
#endif

#ifndef ALCAPIENTRY
 #define ALCAPIENTRY
#endif


#ifndef AL_NO_PROTOTYPES

ALCAPI ALCcontext * ALCAPIENTRY alcCreateContext( ALCdevice *dev,
                                                const ALCint* attrlist );

/**
 * There is no current context, as we can mix
 *  several active contexts. But al* calls
 *  only affect the current context.
 */
ALCAPI ALCboolean ALCAPIENTRY alcMakeContextCurrent(ALCcontext *context);

/**
 * Perform processing on a synced context, non-op on a asynchronous
 * context.
 */

ALCAPI void ALCAPIENTRY alcProcessContext(ALCcontext* context);

/**
 * Suspend processing on an asynchronous context, non-op on a
 * synced context.
 */
ALCAPI void ALCAPIENTRY alcSuspendContext(ALCcontext *context);

ALCAPI void ALCAPIENTRY alcDestroyContext(ALCcontext *context);

ALCAPI ALCenum ALCAPIENTRY alcGetError( ALCdevice *dev );

ALCAPI ALCcontext * ALCAPIENTRY alcGetCurrentContext( ALCvoid );

ALCAPI ALCdevice * ALCAPIENTRY alcOpenDevice( const ALubyte *tokstr );
ALCAPI void ALCAPIENTRY alcCloseDevice( ALCdevice *dev );

ALCAPI ALCboolean ALCAPIENTRY alcIsExtensionPresent(ALCdevice *device, const ALCubyte *extName);
ALCAPI void  * ALCAPIENTRY alcGetProcAddress(ALCdevice *device, const ALCubyte *funcName);
ALCAPI ALCenum    ALCAPIENTRY alcGetEnumValue(ALCdevice *device, const ALCubyte *enumName);

ALCAPI ALCdevice* ALCAPIENTRY alcGetContextsDevice(ALCcontext *context);


/**
 * Query functions
 */

ALCAPI const ALCubyte * ALCAPIENTRY alcGetString( ALCdevice *deviceHandle, ALCenum token );
ALCAPI void ALCAPIENTRY alcGetIntegerv(ALCdevice *deviceHandle, ALCenum token, ALCsizei size, ALCint *dest);

#else /* AL_NO_PROTOTYPES */
      ALCAPI ALCcontext *    ALCAPIENTRY (*alcCreateContext)( ALCdevice *dev, const ALCint* attrlist );
      ALCAPI ALCboolean      ALCAPIENTRY (*alcMakeContextCurrent)(ALCcontext *context);
      ALCAPI void *          ALCAPIENTRY (*alcProcessContext)( ALCcontext *alcHandle );
      ALCAPI void            ALCAPIENTRY (*alcSuspendContext)( ALCcontext *alcHandle );
      ALCAPI void            ALCAPIENTRY (*alcDestroyContext)( ALCcontext* context );
      ALCAPI ALCenum         ALCAPIENTRY (*alcGetError)( ALCdevice *dev );
      ALCAPI ALCcontext *    ALCAPIENTRY (*alcGetCurrentContext)( ALCvoid );
      ALCAPI ALCdevice *     ALCAPIENTRY (*alcOpenDevice)( const ALCubyte *tokstr );
      ALCAPI void            ALCAPIENTRY (*alcCloseDevice)( ALCdevice *dev );
      ALCAPI ALCboolean      ALCAPIENTRY (*alcIsExtensionPresent)( ALCdevice *device, const ALCubyte *extName );
      ALCAPI void  *         ALCAPIENTRY (*alcGetProcAddress)(ALCdevice *device, const ALCubyte *funcName );
      ALCAPI ALCenum         ALCAPIENTRY (*alcGetEnumValue)(ALCdevice *device, const ALCubyte *enumName);
      ALCAPI ALCdevice*      ALCAPIENTRY (*alcGetContextsDevice)(ALCcontext *context);
      ALCAPI const ALCubyte* ALCAPIENTRY (*alcGetString)( ALCdevice *deviceHandle, ALCenum token );
      ALCAPI void            ALCAPIENTRY (*alcGetIntegerv*)( ALCdevice *deviceHandle, ALCenum  token , ALCsizei  size , ALCint *dest );
#endif /* AL_NO_PROTOTYPES */

#ifdef TARGET_OS_MAC
#if TARGET_OS_MAC
#pragma export off
#endif /* TARGET_OS_MAC */
#endif /* TARGET_OS_MAC */

#ifdef __cplusplus
}
#endif

#endif /* ALC_CONTEXT_H_ */
