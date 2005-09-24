#include "al_siteconfig.h"

#include <AL/al.h>

#define SCALING_POWER  14
#define SCALING_FACTOR (1 << SCALING_POWER)

void _alFloatMul(ALshort *bpt, ALfloat sa, ALuint len);

void _alFloatMul(ALshort *bpt, ALfloat sa, ALuint len) {
	ALint scaled_sa = sa * SCALING_FACTOR;
	ALint iter;

	while(len--) {
		iter = *bpt;
		iter *= scaled_sa;
		iter >>= SCALING_POWER;
		*bpt = iter;
		++bpt;
	}

	return;
}
