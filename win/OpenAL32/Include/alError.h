#ifndef _AL_ERROR_H_
#define _AL_ERROR_H_

#define ALAPI __declspec(dllexport)
#define ALAPIENTRY __cdecl

#include "AL/altypes.h"

#ifdef __cplusplus
extern "C" {
#endif

ALvoid alSetError(ALenum errorCode);

#ifdef __cplusplus
}
#endif

#endif