#ifndef AL_AL_CPU_CAPS_H_
#define AL_AL_CPU_CAPS_H_

#include "al_siteconfig.h"

#include <AL/al.h>

#ifdef  HAVE_X86
#include "arch/i386/x86_cpu_caps_prk.h"
#include "arch/i386/x86_floatmul.h"
#else
static __inline void _alDetectCPUCaps(void) {}
void _alFloatMul_portable(ALshort *bpt, ALfloat sa, ALuint len);
#endif

#endif /* not AL_AL_CPU_CAPS_H_ */
