#include "al_siteconfig.h"

#include <AL/al.h>

#include "al_main.h"

void _alFloatMul( ALshort *bpt, ALfloat sa, ALuint len) {
	while(len--) {
#if USE_LRINT
		bpt[len] = lrintf(bpt[len] * sa);
#else
		bpt[len] *= sa;
#endif
	}

	return;
}
