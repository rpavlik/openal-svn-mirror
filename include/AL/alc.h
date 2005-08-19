#ifndef _AL_ALC_H
#define _AL_ALC_H

#include <AL/alctypes.h>

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_WIN32) && !defined(_XBOX)
 #if defined (_OPENAL32LIB)
  #define ALCAPI __declspec(dllexport)
 #else
  #define ALCAPI __declspec(dllimport)
 #endif
#else
 #define ALCAPI extern
#endif

#if defined(_WIN32)
 #define ALCAPIENTRY __cdecl
#else
 #define ALCAPIENTRY
#endif

#if TARGET_OS_MAC
 #pragma export on
#endif

#define ALC_VERSION_0_1         1

#if defined(WINDOWS_AL)
typedef struct ALCdevice_struct ALCdevice;
typedef struct ALCcontext_struct ALCcontext;
#else
struct _AL_device;
typedef struct _AL_device ALCdevice;
typedef void ALCcontext;
#endif

#if !defined(ALC_NO_PROTOTYPES)

/*
 * Context Management
 */
ALCAPI ALCcontext *    ALCAPIENTRY alcCreateContext( ALCdevice *device, const ALCint* attrlist );

ALCAPI ALCboolean      ALCAPIENTRY alcMakeContextCurrent( ALCcontext *context );

ALCAPI void            ALCAPIENTRY alcProcessContext( ALCcontext *context );

ALCAPI void            ALCAPIENTRY alcSuspendContext( ALCcontext *context );

ALCAPI void            ALCAPIENTRY alcDestroyContext( ALCcontext *context );

ALCAPI ALCcontext *    ALCAPIENTRY alcGetCurrentContext( ALCvoid );

ALCAPI ALCdevice*      ALCAPIENTRY alcGetContextsDevice( ALCcontext *context );


/*
 * Device Management
 */
ALCAPI ALCdevice *     ALCAPIENTRY alcOpenDevice( const ALCchar *devicename );

ALCAPI ALCboolean      ALCAPIENTRY alcCloseDevice( ALCdevice *device );


/*
 * Error support.
 * Obtain the most recent Context error
 */
ALCAPI ALCenum         ALCAPIENTRY alcGetError( ALCdevice *device );


/* 
 * Extension support.
 * Query for the presence of an extension, and obtain any appropriate
 * function pointers and enum values.
 */
ALCAPI ALCboolean      ALCAPIENTRY alcIsExtensionPresent( ALCdevice *device, const ALCchar *extname );

ALCAPI void  *         ALCAPIENTRY alcGetProcAddress( ALCdevice *device, const ALCchar *funcname );

ALCAPI ALCenum         ALCAPIENTRY alcGetEnumValue( ALCdevice *device, const ALCchar *enumname );


/*
 * Query functions
 */
ALCAPI const ALCchar * ALCAPIENTRY alcGetString( ALCdevice *device, ALCenum param );

ALCAPI void            ALCAPIENTRY alcGetIntegerv( ALCdevice *device, ALCenum param, ALCsizei size, ALCint *data );


/*
 * Capture functions
 */
ALCAPI ALCdevice*      ALCAPIENTRY alcCaptureOpenDevice( const ALCchar *devicename, ALCuint frequency, ALCenum format, ALCsizei buffersize );

ALCAPI ALCboolean      ALCAPIENTRY alcCaptureCloseDevice( ALCdevice *device );

ALCAPI void            ALCAPIENTRY alcCaptureStart( ALCdevice *device );

ALCAPI void            ALCAPIENTRY alcCaptureStop( ALCdevice *device );

ALCAPI void            ALCAPIENTRY alcCaptureSamples( ALCdevice *device, ALCvoid *buffer, ALCsizei samples );

#else /* ALC_NO_PROTOTYPES */

ALCAPI ALCcontext *    (ALCAPIENTRY *alcCreateContext)( ALCdevice *device, const ALCint* attrlist );
ALCAPI ALCboolean      (ALCAPIENTRY *alcMakeContextCurrent)( ALCcontext *context );
ALCAPI void            (ALCAPIENTRY *alcProcessContext)( ALCcontext *context );
ALCAPI void            (ALCAPIENTRY *alcSuspendContext)( ALCcontext *context );
ALCAPI void            (ALCAPIENTRY *alcDestroyContext)( ALCcontext *context );
ALCAPI ALCcontext *    (ALCAPIENTRY *alcGetCurrentContext)( ALCvoid );
ALCAPI ALCdevice *     (ALCAPIENTRY *alcGetContextsDevice)( ALCcontext *context );
ALCAPI ALCdevice *     (ALCAPIENTRY *alcOpenDevice)( const ALCchar *devicename );
ALCAPI ALCboolean      (ALCAPIENTRY *alcCloseDevice)( ALCdevice *device );
ALCAPI ALCenum         (ALCAPIENTRY *alcGetError)( ALCdevice *device );
ALCAPI ALCboolean      (ALCAPIENTRY *alcIsExtensionPresent)( ALCdevice *device, const ALCchar *extname );
ALCAPI void *          (ALCAPIENTRY *alcGetProcAddress)( ALCdevice *device, const ALCchar *funcname );
ALCAPI ALCenum         (ALCAPIENTRY *alcGetEnumValue)( ALCdevice *device, const ALCchar *enumname );
ALCAPI const ALCchar*  (ALCAPIENTRY *alcGetString)( ALCdevice *device, ALCenum param );
ALCAPI void            (ALCAPIENTRY *alcGetIntegerv)( ALCdevice *device, ALCenum param, ALCsizei size, ALCint *dest );
ALCAPI ALCdevice *     (ALCAPIENTRY *alcCaptureOpenDevice)( const ALCchar *devicename, ALCuint frequency, ALCenum format, ALCsizei buffersize );
ALCAPI ALCboolean      (ALCAPIENTRY *alcCaptureCloseDevice)( ALCdevice *device );
ALCAPI void            (ALCAPIENTRY *alcCaptureStart)( ALCdevice *device );
ALCAPI void            (ALCAPIENTRY *alcCaptureStop)( ALCdevice *device );
ALCAPI void            (ALCAPIENTRY *alcCaptureSamples)( ALCdevice *device, ALCvoid *buffer, ALCsizei samples );

/* Type definitions */
typedef ALCcontext *   (ALCAPIENTRY *LPALCCREATECONTEXT) (ALCdevice *device, const ALCint *attrlist);
typedef ALCboolean     (ALCAPIENTRY *LPALCMAKECONTEXTCURRENT)( ALCcontext *context );
typedef void           (ALCAPIENTRY *LPALCPROCESSCONTEXT)( ALCcontext *context );
typedef void           (ALCAPIENTRY *LPALCSUSPENDCONTEXT)( ALCcontext *context );
typedef void           (ALCAPIENTRY *LPALCDESTROYCONTEXT)( ALCcontext *context );
typedef ALCcontext *   (ALCAPIENTRY *LPALCGETCURRENTCONTEXT)( ALCvoid );
typedef ALCdevice *    (ALCAPIENTRY *LPALCGETCONTEXTSDEVICE)( ALCcontext *context );
typedef ALCdevice *    (ALCAPIENTRY *LPALCOPENDEVICE)( const ALCchar *devicename );
typedef ALCboolean     (ALCAPIENTRY *LPALCCLOSEDEVICE)( ALCdevice *device );
typedef ALCenum        (ALCAPIENTRY *LPALCGETERROR)( ALCdevice *device );
typedef ALCboolean     (ALCAPIENTRY *LPALCISEXTENSIONPRESENT)( ALCdevice *device, const ALCchar *extname );
typedef void *         (ALCAPIENTRY *LPALCGETPROCADDRESS)(ALCdevice *device, const ALCchar *funcname );
typedef ALCenum        (ALCAPIENTRY *LPALCGETENUMVALUE)(ALCdevice *device, const ALCchar *enumname );
typedef const ALCchar* (ALCAPIENTRY *LPALCGETSTRING)( ALCdevice *device, ALCenum param );
typedef void           (ALCAPIENTRY *LPALCGETINTEGERV)( ALCdevice *device, ALCenum param, ALCsizei size, ALCint *dest );
typedef ALCdevice *    (ALCAPIENTRY *LPALCCAPTUREOPENDEVICE)( const ALCchar *devicename, ALCuint frequency, ALCenum format, ALCsizei buffersize );
typedef ALCboolean     (ALCAPIENTRY *LPALCCAPTURECLOSEDEVICE)( ALCdevice *device );
typedef void           (ALCAPIENTRY *LPALCCAPTURESTART)( ALCdevice *device );
typedef void           (ALCAPIENTRY *LPALCCAPTURESTOP)( ALCdevice *device );
typedef void           (ALCAPIENTRY *LPALCCAPTURESAMPLES)( ALCdevice *device, ALCvoid *buffer, ALCsizei samples );

#endif /* ALC_NO_PROTOTYPES */

#if TARGET_OS_MAC
 #pragma export off
#endif

#if defined(__cplusplus)
}
#endif

#endif /* _AL_ALC_H */
