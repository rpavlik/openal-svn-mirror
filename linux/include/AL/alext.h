#ifndef _AL_ALEXT_H
#define _AL_ALEXT_H

#include "AL/altypes.h"
#include "alexttypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ALAPI
#define ALAPI extern
#endif

#ifndef ALAPIENTRY
#define ALAPIENTRY
#endif

/* loki */

ALAPI ALfloat alcGetAudioChannel_LOKI(ALuint channel);
ALAPI void alcSetAudioChannel_LOKI(ALuint channel, ALfloat volume);
ALAPI void alBombOnError_LOKI(void);
ALAPI void alBufferi_LOKI(ALuint bid, ALenum param, ALint value);
ALAPI void alBufferDataWithCallback_LOKI(ALuint bid,
					 int (*Callback)(ALuint, ALuint, ALshort *, ALenum, ALint, ALint));

ALAPI void alBufferWriteData_LOKI( ALuint   buffer,
                   ALenum   format,
                   ALvoid*  data,
                   ALsizei  size,
                   ALsizei  freq,
                   ALenum   internalFormat );
ALAPI void ALAPIENTRY alGenStreamingBuffers_LOKI( ALsizei n, ALuint *samples );
ALAPI ALsizei alBufferAppendData_LOKI( ALuint   buffer,
				       ALenum   format,
				       ALvoid*    data,
				       ALsizei  size,
				       ALsizei  freq );

ALAPI ALsizei alBufferAppendWriteData_LOKI( ALuint   buffer,
					    ALenum   format,
					    ALvoid*  data,
					    ALsizei  size,
					    ALsizei  freq,
					    ALenum internalFormat );

/* Capture api */

ALAPI ALboolean alCaptureInit_EXT( ALenum format, ALuint rate, ALsizei bufferSize );
ALAPI ALboolean alCaptureDestroy_EXT( void );
ALAPI ALboolean alCaptureStart_EXT( void );
ALAPI ALboolean alCaptureStop_EXT( void );

/* Non-blocking device read */
ALAPI ALsizei alCaptureGetData_EXT( ALvoid* data, ALsizei n, ALenum format, ALuint rate );

/* custom loaders */
ALAPI ALboolean alutLoadVorbis_LOKI(ALuint bid, const ALvoid *data, ALint size);
ALAPI ALboolean ALAPIENTRY alutLoadRAW_ADPCMData_LOKI(ALuint bid, ALvoid *data,
						      ALuint size, ALuint freq,
						      ALenum format);

ALAPI ALboolean ALAPIENTRY alutLoadIMA_ADPCMData_LOKI(ALuint bid, ALvoid *data,
						      ALuint size,
						      alIMAADPCM_state_LOKI *ias);
ALAPI ALboolean ALAPIENTRY alutLoadMS_ADPCMData_LOKI(ALuint bid,
						     void *data, int size,
						     alMSADPCM_state_LOKI *mss);


#ifdef __cplusplus
}
#endif

#endif /* _AL_ALEXT_H */
