/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * This is the header that apps should include when linking against
 * audioconvert or such.
 */
#ifndef AL_AUDIOCONVERT_AUDIOCONVERT_H_
#define AL_AUDIOCONVERT_AUDIOCONVERT_H_

#include "al_siteconfig.h"
#include <AL/al.h>
#include "audioconvert/ac_adpcm.h"

/* new data types */
typedef struct _acAudioCVT {
	int needed;			/* Set to 1 if conversion possible */
	ALushort src_format;		/* Source audio format */
	ALushort dst_format;		/* Target audio format */
	ALushort src_channels;		/* Source channel count */
	ALushort dst_channels;		/* Target channel count */
	double rate_incr;		/* Rate conversion increment */
	void   *buf;			/* Buffer to hold entire audio data */
	int    len;			/* Length of original audio buffer */
	int    len_cvt;			/* Length of converted audio buffer */
	int    len_mult;		/* buffer must be len*len_mult big */
	double len_ratio; 	/* Given len, final size is len*len_ratio */
	void (*filters[10])(struct _acAudioCVT *cvt, ALushort format, ALushort channels);
	int filter_index;		/* Current audio conversion function */
} acAudioCVT;

/* Audio format flags (defaults to LSB byte order) */
#define AUDIO_U8	0x0008	/* Unsigned 8-bit samples */
#define AUDIO_S8	0x8008	/* Signed 8-bit samples */
#define AUDIO_U16LSB	0x0010	/* Unsigned 16-bit samples */
#define AUDIO_S16LSB	0x8010	/* Signed 16-bit samples */
#define AUDIO_U16MSB	0x1010	/* As above, but big-endian byte order */
#define AUDIO_S16MSB	0x9010	/* As above, but big-endian byte order */

/* Native audio byte ordering */
#ifndef WORDS_BIGENDIAN
#define AUDIO_U16	AUDIO_U16LSB
#define AUDIO_S16	AUDIO_S16LSB
#else /* not WORDS_BIGENDIAN */
#define AUDIO_U16	AUDIO_U16MSB
#define AUDIO_S16	AUDIO_S16MSB
#endif /* not WORDS_BIGENDIAN */

/* macros */
#define acFormatBits(fmt)           (fmt & 0xff)

/* bit changes */
void acConvert16MSB(acAudioCVT *cvt, ALushort format, ALushort channels);
void acConvert16LSB(acAudioCVT *cvt, ALushort format, ALushort channels);
void acConvert8(acAudioCVT *cvt, ALushort format, ALushort channels);

/* frequency changes */
void acFreqMUL2(acAudioCVT *cvt, ALushort format, ALushort channels);
void acFreqDIV2(acAudioCVT *cvt, ALushort format, ALushort channels);
void acFreqSLOW(acAudioCVT *cvt, ALushort format, ALushort channels);

/* channel changes */
void acConvertStereo(acAudioCVT *cvt, ALushort format, ALushort channels);
void acConvertMono(acAudioCVT *cvt, ALushort format, ALushort channels);

/* misc */
int   ac_is_wave(void *data);
void *ac_guess_info(void *data, ALuint *size,
		ALushort *fmt, ALushort *channels, ALushort *freq);
void *ac_guess_wave_info(void *data, ALuint *size,
		ALushort *fmt, ALushort *channels, ALushort *freq);
void *ac_wave_to_pcm(const void *data, ALuint *size,
		ALushort *fmt, ALushort *channels, ALushort *freq);

int ac_isWAVE_ANY_adpcm(void *datap, ALuint size);
int ac_isWAVE_IMA_adpcm(void *data, ALuint size);
int ac_isWAVE_MS_adpcm(void *data, ALuint size);
void *ac_getWAVEadpcm_info(void *data, ALuint *size, void *spec);

/* sdl helper stuff */
int acBuildAudioCVT(acAudioCVT *cvt,
	ALushort src_format, ALubyte src_channels, ALuint src_rate,
	ALushort dst_format, ALubyte dst_channels, ALuint dst_rate);

int acConvertAudio(acAudioCVT *);
void *acLoadWAV(const void *data, ALuint *size, void **udata,
		ALushort *format,
		ALushort *channels,
		ALushort *freq);

#endif /* not AL_AUDIOCONVERT_AUDIOCONVERT_H_ */
