#ifndef _LAL_EXTTYPES_H_
#define _LAL_EXTTYPES_H_

#define LAL_OPENAL                                1

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
	ALushort channels;               /* 1 = mono, 2 = stereo */
	ALuint frequency;              /* One of 11025, 22050, or 44100 Hz */
	ALuint byterate;               /* Average bytes per second */
	ALushort blockalign;             /* Bytes per sample block */
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
	ALint valprev;	/* Previous output value */
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


typedef void (*PFNALCSETAUDIOCHANNELPROC)(ALuint channel, ALfloat volume);
typedef ALfloat (*PFNALCGETAUDIOCHANNELPROC)(ALuint channel);
typedef void (*PFNALBOMBONERRORPROC)(void);

typedef void (*PFNALBUFFERIPROC)(ALuint bid, ALenum param, ALint value);

typedef void (*PFNALBUFFERDATAWITHCALLBACKPROC)(ALuint bid,
		int (*Callback)(ALuint, ALuint, ALshort *, ALenum, ALint, ALint));

typedef void (*PFNALBUFFERWRITEDATAPROC)( ALuint   buffer,
                   ALenum   format,
                   ALvoid*  data,
                   ALsizei  size,
                   ALsizei  freq,
                   ALenum   internalFormat );

typedef void (*PFNALGENSTREAMINGBUFFERSPROC)( ALsizei n, ALuint *samples );

typedef ALsizei (*PFNALBUFFERAPPENDDATAPROC)( ALuint   buffer,
                            ALenum   format,
                            ALvoid*    data,
                            ALsizei  size,
                            ALsizei  freq );

typedef ALsizei (*PFNALBUFFERAPPENDWRITEDATAPROC)( ALuint   buffer,
                            ALenum   format,
                            ALvoid*  data,
                            ALsizei  size,
                            ALsizei  freq,
			    ALenum internalFormat );

/* captures */

typedef ALboolean (*PFNALCAPTUREINITPROC)( ALenum format, ALuint rate, ALsizei bufferSize );

typedef ALboolean (*PFNALCAPTUREDESTROYPROC)( ALvoid );

typedef ALboolean (*PFNALCAPTURESTARTPROC)( ALvoid );

typedef ALboolean (*PFNALCAPTURESTOPPROC)( ALvoid );

/* Non-blocking device read */
typedef ALsizei (*PFNALCAPTUREGETDATAPROC)( ALvoid* data, ALsizei n, ALenum format, ALuint rate );

/* vorbis */
typedef  ALboolean (*PFNALUTLOADVORBISPROC)(ALuint bid, ALvoid *data, ALint size);

/* custom loaders */
typedef ALboolean (*PFNALUTLOADRAW_ADPCMDATAPROC)( ALuint bid,
				ALvoid *data, ALuint size, ALuint freq,
				ALenum format);

typedef ALboolean (*ALUTLOADIMA_ADPCMDATAPROC)(ALuint bid,
				ALvoid *data, ALuint size,
				alIMAADPCM_state_LOKI *ias);

typedef ALboolean (*ALUTLOADMS_ADPCMDATAPROC)(ALuint bid,
				void *data, int size,
				alMSADPCM_state_LOKI *mss);

#endif /* _LAL_EXTTYPES_H_ */
