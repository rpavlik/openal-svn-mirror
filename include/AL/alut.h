#ifndef _ALUT_H_
#define _ALUT_H_

#include "altypes.h"
#include "aluttypes.h"

#ifdef _WIN32
#define ALUTAPI
#define ALUTAPIENTRY    __cdecl
#define AL_CALLBACK
#else  /* _WIN32 */

#ifdef TARGET_OS_MAC
#if TARGET_OS_MAC
#pragma export on
#endif /* TARGET_OS_MAC */
#endif /* TARGET_OS_MAC */

#ifndef ALUTAPI
#define ALUTAPI
#endif

#ifndef ALUTAPIENTRY
#define ALUTAPIENTRY
#endif

#ifndef AL_CALLBACK
#define AL_CALLBACK
#endif 

#endif /* _WIN32 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef AL_NO_PROTOTYPES

ALUTAPI void ALUTAPIENTRY alutInit(int *argc, char *argv[]);
ALUTAPI void ALUTAPIENTRY alutExit(ALvoid);

  /* ***** GH
TEMPORARY ifdef -- Linux-implementation-specific function
  */
#ifdef __GNUC__
ALUTAPI ALboolean ALUTAPIENTRY alutLoadWAV( const char *fname,
                        ALvoid **wave,
			ALsizei *format,
			ALsizei *size,
			ALsizei *bits,
			ALsizei *freq );
#endif

ALUTAPI void ALUTAPIENTRY alutLoadWAVFile(ALbyte *file,
				      ALenum *format,
				      ALvoid **data,
				      ALsizei *size,
				      ALsizei *freq,
				      ALboolean *loop);
ALUTAPI void ALUTAPIENTRY alutLoadWAVMemory(ALbyte *memory,
					ALenum *format,
					ALvoid **data,
					ALsizei *size,
					ALsizei *freq,
					ALboolean *loop);
ALUTAPI void ALUTAPIENTRY alutUnloadWAV(ALenum format,
				    ALvoid *data,
				    ALsizei size,
				    ALsizei freq);

#else
ALUTAPI void      ALUTAPIENTRY (*alutInit)(int *argc, char *argv[]);
ALUTAPI void 	  ALUTAPIENTRY (*alutExit)(ALvoid);

  /* ***** GH
TEMPORARY ifdef -- Linux-implementation-specific function
  */
#ifdef __GNUC__
ALUTAPI ALboolean ALUTAPIENTRY (*alutLoadWAV)( const char *fname,
                        ALvoid **wave,
			ALsizei *format,
			ALsizei *size,
			ALsizei *bits,
			ALsizei *freq );
#endif

ALUTAPI void      ALUTAPIENTRY (*alutLoadWAVFile(ALbyte *file,ALenum *format,ALvoid **data,ALsizei *size,ALsizei *freq,ALboolean *loop);
ALUTAPI void      ALUTAPIENTRY (*alutLoadWAVMemory)(ALbyte *memory,ALenum *format,ALvoid **data,ALsizei *size,ALsizei *freq,ALboolean *loop);
ALUTAPI void      ALUTAPIENTRY (*alutUnloadWAV)(ALenum format,ALvoid *data,ALsizei size,ALsizei freq);


#endif /* AL_NO_PROTOTYPES */

#ifdef TARGET_OS_MAC
#if TARGET_OS_MAC
#pragma export off
#endif /* TARGET_OS_MAC */
#endif /* TARGET_OS_MAC */

#ifdef __cplusplus
}
#endif

#endif
