#ifndef AL_CPU_CAPS_H
#define AL_CPU_CAPS_H

void _alDetectCPUCaps(void);

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#include "arch/i386/x86_cpu_caps_prk.h"
#endif

#endif /* AL_CPU_CAPS_H */
