#ifndef _AL_EXTENSION_H_
#define _AL_EXTENSION_H_

#define ALAPI __declspec(dllexport)
#define ALAPIENTRY __cdecl

#include "AL/altypes.h"
#include "AL/alctypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ALextension_struct
{
	ALubyte		*extName;
	ALvoid		*address;
} ALextension;

typedef struct ALfunction_struct
{
	ALubyte		*funcName;
	ALvoid		*address;
} ALfunction;

typedef struct ALenum_struct
{
	ALubyte		*enumName;
	ALenum		value;
} ALenums;

#ifdef __cplusplus
}
#endif

#endif