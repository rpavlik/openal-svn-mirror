#ifndef TESTLIB_H_
#define TESTLIB_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "../src/al_siteconfig.h"
#include "AL/altypes.h"
#include "../config.h"

/*
 * function pointer for LOKI extensions
 */
extern ALfloat	(*talcGetAudioChannel)(ALuint channel);
extern void	(*talcSetAudioChannel)(ALuint channel, ALfloat volume);

extern void	(*talMute)(ALvoid);
extern void	(*talUnMute)(ALvoid);

extern void	(*talReverbScale)(ALuint sid, ALfloat param);
extern void	(*talReverbDelay)(ALuint sid, ALfloat param);
extern void	(*talBombOnError)(void);

extern void	(*talBufferi)(ALuint bid, ALenum param, ALint value);

extern ALuint  (*talBufferAppendData)(ALuint bid, ALenum format, ALvoid *data, ALint freq, ALint samples);
extern ALuint  (*talBufferAppendWriteData)(ALuint bid, ALenum format, ALvoid *data, ALint freq, ALint samples, ALenum internalFormat);

extern ALboolean (*alCaptureInit) ( ALenum format, ALuint rate, ALsizei bufferSize );
extern ALboolean (*alCaptureDestroy) ( ALvoid );
extern ALboolean (*alCaptureStart) ( ALvoid );
extern ALboolean (*alCaptureStop) ( ALvoid );
extern ALsizei (*alCaptureGetData) ( ALvoid* data, ALsizei n, ALenum format, ALuint rate );

/* new ones */
extern void (*talGenStreamingBuffers)(ALsizei n, ALuint *bids );
extern ALboolean (*talutLoadRAW_ADPCMData)(ALuint bid,
				ALvoid *data, ALuint size, ALuint freq,
				ALenum format);
extern ALboolean (*talutLoadIMA_ADPCMData)(ALuint bid,
				ALvoid *data, ALuint size, ALuint freq,
				ALenum format);
extern ALboolean (*talutLoadMS_ADPCMData)(ALuint bid,
				ALvoid *data, ALuint size, ALuint freq,
				ALenum format);

void micro_sleep(unsigned int n);
void fixup_function_pointers(void);
ALboolean SourceIsPlaying(ALuint sid);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* TESTLIB_H_ */
