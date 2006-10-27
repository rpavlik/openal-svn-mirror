#include "al_siteconfig.h"

#include <AL/al.h>
#include "al_cpu_caps.h"

#define SCALING_POWER  16
#define SCALING_FACTOR (1 << SCALING_POWER)


void _alFloatMul_portable(ALshort *bpt, ALfloat sa, ALuint len) {
	ALint scaled_sa = sa * SCALING_FACTOR;

	while(len--) {
		ALint iter = *bpt;
		iter *= scaled_sa;
		iter >>= SCALING_POWER;
		*bpt = iter;
		++bpt;
	}
}
