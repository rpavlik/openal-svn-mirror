#ifndef _AL_ALEXT_H
#define _AL_ALEXT_H

#include <AL/al.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ALAPI
#define ALAPI extern
#endif

#ifndef ALAPIENTRY
#define ALAPIENTRY
#endif

/* format base 0x10000 */
#define AL_FORMAT_IMA_ADPCM_MONO16_EXT            0x10000
#define AL_FORMAT_IMA_ADPCM_STEREO16_EXT          0x10001
#define AL_FORMAT_WAVE_EXT                        0x10002
#define AL_FORMAT_VORBIS_EXT                      0x10003

/* four point formats */
#define AL_FORMAT_QUAD8_LOKI                      0x10004
#define AL_FORMAT_QUAD16_LOKI                     0x10005

/**
 * token extensions, base 0x20000
 */
/**
 * Indicate the gain (volume amplification) applied, in a
 * normalized linear scale.  This affects the value retrieved
 * by AL_GAIN.
 *
 * Type:   ALfloat.
 * Range:  ]0.0-  ]
 * A value of 1.0 means un-attenuated/unchanged.
 * A value of 0.0 is  interpreted as zero volume - the channel
 *  is effectively disabled.
 */
#define AL_GAIN_LINEAR_LOKI                      0x20000

/*
 * types for special loaders.  This should be deprecated in favor
 * of the special format tags.
 */

typedef struct WaveFMT {
	ALushort encoding;
	ALushort channels;	/* 1 = mono, 2 = stereo */
	ALuint frequency;	/* One of 11025, 22050, or 44100 Hz */
	ALuint byterate;	/* Average bytes per second */
	ALushort blockalign;	/* Bytes per sample block */
	ALushort bitspersample;
} alWaveFMT_LOKI;

typedef struct _MS_ADPCM_decodestate {
	ALubyte hPredictor;
	ALushort iDelta;
	ALshort iSamp1;
	ALshort iSamp2;
} alMSADPCM_decodestate_LOKI;

typedef struct MS_ADPCM_decoder {
	alWaveFMT_LOKI wavefmt;
	ALushort wSamplesPerBlock;
	ALushort wNumCoef;
	ALshort aCoeff[7][2];
	/* * * */
	alMSADPCM_decodestate_LOKI state[2];
} alMSADPCM_state_LOKI;

typedef struct IMA_ADPCM_decodestate_s {
	ALint valprev;		/* Previous output value */
	ALbyte index;		/* Index into stepsize table */
} alIMAADPCM_decodestate_LOKI;

typedef struct IMA_ADPCM_decoder {
	alWaveFMT_LOKI wavefmt;
	ALushort wSamplesPerBlock;
	/* * * */
	alIMAADPCM_decodestate_LOKI state[2];
} alIMAADPCM_state_LOKI;

/**
 * Context creation extension tokens
 * base 0x200000
 */

/**
 * followed by ### of sources
 */
#define ALC_SOURCES_LOKI                         0x200000

/**
 * followed by ### of buffers
 */
#define ALC_BUFFERS_LOKI                         0x200001

/*
 *  Channel operations are probably a big no-no and destined
 *  for obsolesence.
 *
 *  base 0x300000
 */
#define	ALC_CHAN_MAIN_LOKI                       0x300000
#define	ALC_CHAN_PCM_LOKI                        0x300001
#define	ALC_CHAN_CD_LOKI                         0x300002

/* loki */

ALAPI ALfloat   ALAPIENTRY alcGetAudioChannel_LOKI( ALuint channel );
ALAPI void      ALAPIENTRY alcSetAudioChannel_LOKI( ALuint channel, ALfloat volume );
ALAPI void      ALAPIENTRY alBombOnError_LOKI( void );
ALAPI void      ALAPIENTRY alBufferi_LOKI( ALuint bid, ALenum param, ALint value );
ALAPI void      ALAPIENTRY alBufferDataWithCallback_LOKI( ALuint bid, int ( *Callback ) ( ALuint, ALuint, ALshort *, ALenum, ALint, ALint ) );
ALAPI void      ALAPIENTRY alBufferWriteData_LOKI( ALuint buffer, ALenum format, ALvoid *data, ALsizei size, ALsizei freq, ALenum internalFormat );
ALAPI void      ALAPIENTRY alGenStreamingBuffers_LOKI( ALsizei n, ALuint *samples );
ALAPI ALsizei   ALAPIENTRY alBufferAppendData_LOKI( ALuint buffer, ALenum format, ALvoid *data, ALsizei size, ALsizei freq );
ALAPI ALsizei   ALAPIENTRY alBufferAppendWriteData_LOKI( ALuint buffer, ALenum format, ALvoid *data, ALsizei size, ALsizei freq, ALenum internalFormat );

/* Capture api */

ALAPI ALboolean ALAPIENTRY alCaptureInit_EXT( ALenum format, ALuint rate, ALsizei bufferSize );
ALAPI ALboolean ALAPIENTRY alCaptureDestroy_EXT( void );
ALAPI ALboolean ALAPIENTRY alCaptureStart_EXT( void );
ALAPI ALboolean ALAPIENTRY alCaptureStop_EXT( void );

/* Non-blocking device read */

ALAPI ALsizei   ALAPIENTRY alCaptureGetData_EXT( ALvoid *data, ALsizei n, ALenum format, ALuint rate );

/* custom loaders */

ALAPI ALboolean ALAPIENTRY alutLoadVorbis_LOKI( ALuint bid, const ALvoid *data, ALint size );

/* function pointers */

typedef ALfloat   ( ALAPIENTRY *PFNALCGETAUDIOCHANNELPROC ) ( ALuint channel );
typedef void      ( ALAPIENTRY *PFNALCSETAUDIOCHANNELPROC ) ( ALuint channel, ALfloat volume );
typedef void      ( ALAPIENTRY *PFNALBOMBONERRORPROC ) ( void );
typedef void      ( ALAPIENTRY *PFNALBUFFERIPROC ) ( ALuint bid, ALenum param, ALint value );
typedef void      ( ALAPIENTRY *PFNALBUFFERDATAWITHCALLBACKPROC ) ( ALuint bid, int ( *Callback ) ( ALuint, ALuint, ALshort *, ALenum, ALint, ALint ) );
typedef void      ( ALAPIENTRY *PFNALBUFFERWRITEDATAPROC ) ( ALuint buffer, ALenum format, ALvoid *data, ALsizei size, ALsizei freq, ALenum internalFormat );
typedef void      ( ALAPIENTRY *PFNALGENSTREAMINGBUFFERSPROC ) ( ALsizei n, ALuint *samples );
typedef ALsizei   ( ALAPIENTRY *PFNALBUFFERAPPENDDATAPROC ) ( ALuint buffer, ALenum format, ALvoid *data, ALsizei size, ALsizei freq );
typedef ALsizei   ( ALAPIENTRY *PFNALBUFFERAPPENDWRITEDATAPROC ) ( ALuint buffer, ALenum format, ALvoid *data, ALsizei size, ALsizei freq, ALenum internalFormat );

typedef ALboolean ( ALAPIENTRY *PFNALCAPTUREINITPROC ) ( ALenum format, ALuint rate, ALsizei bufferSize );
typedef ALboolean ( ALAPIENTRY *PFNALCAPTUREDESTROYPROC ) ( void );
typedef ALboolean ( ALAPIENTRY *PFNALCAPTURESTARTPROC ) ( void );
typedef ALboolean ( ALAPIENTRY *PFNALCAPTURESTOPPROC ) ( void );

typedef ALsizei   ( ALAPIENTRY *PFNALCAPTUREGETDATAPROC ) ( ALvoid *data, ALsizei n, ALenum format, ALuint rate );

typedef ALboolean ( ALAPIENTRY *PFNALUTLOADVORBISPROC ) ( ALuint bid, ALvoid *data, ALint size );
typedef ALboolean ( ALAPIENTRY *PFNALUTLOADRAW_ADPCMDATAPROC ) ( ALuint bid, ALvoid *data, ALuint size, ALuint freq, ALenum format );
typedef ALboolean ( ALAPIENTRY *ALUTLOADIMA_ADPCMDATAPROC ) ( ALuint bid, ALvoid *data, ALuint size, alIMAADPCM_state_LOKI *ias );
typedef ALboolean ( ALAPIENTRY *ALUTLOADMS_ADPCMDATAPROC ) ( ALuint bid, void *data, int size, alMSADPCM_state_LOKI *mss );

typedef void      ( ALAPIENTRY *PFNALREVERBSCALEPROC ) ( ALuint sid, ALfloat param );
typedef void      ( ALAPIENTRY *PFNALREVERBDELAYPROC ) ( ALuint sid, ALfloat param );

#ifdef __cplusplus
}
#endif

#endif				/* _AL_ALEXT_H */
