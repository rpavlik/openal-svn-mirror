#ifndef AL_AL_CPU_CAPS_H_
#define AL_AL_CPU_CAPS_H_

#include "al_siteconfig.h"

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#include "arch/i386/x86_cpu_caps_prk.h"
#else
static __inline void _alDetectCPUCaps(void) {}
#endif

#endif /* not AL_AL_CPU_CAPS_H_ */
