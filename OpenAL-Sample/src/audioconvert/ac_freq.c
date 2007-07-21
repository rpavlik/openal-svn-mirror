/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * ac_freq.c
 *
 * audioconvert functions related to changing the sampling
 * rate of a buffer.
 *
 */
#include "al_siteconfig.h"

#include <AL/al.h>
#include <stdio.h>

#include "audioconvert/audioconvert.h"

/* Convert rate up by multiple of 2 */
void acFreqMUL2(acAudioCVT *cvt, ALushort format, ALushort channels)
{
	int i, j;
	ALubyte *src, *dst;
	src = (ALubyte *) cvt->buf + cvt->len_cvt;
	dst = (ALubyte *) cvt->buf + cvt->len_cvt * 2;
	
	switch(format & 0xFF) {
	case 8:
		if (format == AUDIO_U8) {
			ALubyte *src8 = (ALubyte*) src;
			ALubyte *dst8 = (ALubyte*) dst - channels;
			/* For the first sample to be processed (last sample
			   in the buffer) there's no 'next' sample in the
			   buffer to interpolate against.  So we use a value
			   extrapolated (then dampened) from the previous
			   sample in the buffer; this dampens (but doesn't
			   eliminate :() the 'click' of the first sample. */
			if (cvt->len_cvt >= channels*2) {
				for( i=channels; i; --i) {
					int ex; /* extrapolated sample, damped */
					src8 -= 1;
					dst8 -= 1;
					ex = src8[0] + (src8[0]-(int)src8[-channels])/8;
					if ( ex > 255 )
						ex = 255;
					else if ( ex < 0 )
						ex = 0;
					dst8[0] = src8[0];
					dst8[channels] = ex;
				}
				for ( i=cvt->len_cvt-channels; i; ) {
					dst8 -= channels;
					for( j=channels; j; --j,--i) {
						src8 -= 1;
						dst8 -= 1;
						dst8[0] = src8[0];
						dst8[channels] = (src8[0]+(int)src8[channels])/2;
					}
				}
			} else if (cvt->len_cvt >= channels*1) {
				for( i=channels; i; --i) {
					src8 -= 1;
					dst8 -= 1;
					dst8[0] = src8[0];
					dst8[channels] = src8[0];
				}
			}
		} else if (format == AUDIO_S8) {
			ALbyte *src8 = (ALbyte*) src;
			ALbyte *dst8 = (ALbyte*) dst - channels;
			if (cvt->len_cvt >= channels*2) {
				for( i=channels; i; --i) {
					int ex; /* extrapolated sample, damped */
					src8 -= 1;
					dst8 -= 1;
					ex = src8[0] + (src8[0]-(int)src8[-channels])/8;
					if ( ex > 127 )
						ex = 127;
					else if ( ex < -128 )
						ex = -128;
					dst8[0] = src8[0];
					dst8[channels] = ex;
				}
				for ( i=cvt->len_cvt-channels; i; ) {
					dst8 -= channels;
					for( j=channels; j; --j,--i) {
						src8 -= 1;
						dst8 -= 1;
						dst8[0] = src8[0];
						dst8[channels] = (src8[0]+(int)src8[channels])/2;
					}
				}
			} else if (cvt->len_cvt >= channels*1) {
				for( i=channels; i; --i) {
					src8 -= 1;
					dst8 -= 1;
					dst8[0] = src8[0];
					dst8[channels] = src8[0];
				}
			}
		}
		break;
	case 16:
		if (format == AUDIO_S16) {
			ALshort *src16 = (ALshort*) src;
			ALshort *dst16 = (ALshort*) dst - channels;
			if (cvt->len_cvt >= channels*4) {
				for ( i=channels; i; --i ) {
					int ex; /* extrapolated sample, damped */
					src16 -= 1;
					dst16 -= 1;
					ex = src16[0] + (src16[0]-(int)src16[-channels])/8;
					if ( ex > 32767 )
						ex = 32767;
					else if ( ex < -32768 )
						ex = -32768;
					dst16[0] = src16[0];
					dst16[channels] = ex;
				}
				for ( i=cvt->len_cvt/2-channels; i; ) {
					dst16 -= channels;
					for( j=channels; j; --j,--i) {
						src16 -= 1;
						dst16 -= 1;
						dst16[0] = src16[0];
						dst16[channels] = (src16[0]+(int)src16[channels])/2;
					}
				}
			} else if (cvt->len_cvt >= channels*2) {
				for( i=channels; i; --i) {
					src16 -= 1;
					dst16 -= 1;
					dst16[0] = src16[0];
					dst16[channels] = src16[0];
				}
			}
		} else if (format == AUDIO_U16) {
			ALushort *src16 = (ALushort*) src;
			ALushort *dst16 = (ALushort*) dst - channels;
			if (cvt->len_cvt >= channels*4) {
				for( i=channels; i; --i) {
					int ex; /* extrapolated sample, damped */
					ex = src16[0] + (src16[0]-(int)src16[-channels])/8;
					src16 -= 1;
					dst16 -= 1;
					if ( ex > 65535 )
						ex = 65535;
					else if ( ex < 0 )
						ex = 0;
					dst16[0] = src16[0];
					dst16[channels] = ex;
				}
				for ( i=cvt->len_cvt/2-channels; i; ) {
					dst16 -= channels;
					for( j=channels; j; --j,--i) {
						src16 -= 1;
						dst16 -= 1;
						dst16[0] = src16[0];
						dst16[channels] = (src16[0]+(int)src16[channels])/2;
					}
				}
			} else if (cvt->len_cvt >= channels*2) {
				for( i=channels; i; --i) {
					src16 -= 1;
					dst16 -= 1;
					dst16[0] = src16[0];
					dst16[channels] = src16[0];
				}
			}
		} else {
			/* this is a 16-bit format that doesn't correspond
			   to a native type, so sample interpolation isn't
			   completely trivial; we'll just do sample
			   duplication.  Not too hard to fix though, for
			   future work. */
			ALushort *src16 = (ALushort*) src;
			ALushort *dst16 = (ALushort*) dst;
			for ( i=cvt->len_cvt/2; i; ) {
				dst16 -= channels;
				for( j=channels; j; --j,--i) {
					src16 -= 1;
					dst16 -= 1;
					dst16[0] = src16[0];
					dst16[channels] = src16[0];
				}
			}
		}
	break;
	}
	cvt->len_cvt *= 2;
	
	if (cvt->filters[++cvt->filter_index] ) {
		cvt->filters[cvt->filter_index](cvt, format, channels);
	}
}

/* Convert rate down by multiple of 2 */
void acFreqDIV2(acAudioCVT *cvt, ALushort format, ALushort channels)
{
	int i, j;
	ALubyte *src, *dst;

	src = cvt->buf;
	dst = cvt->buf;
	switch(format & 0xFF) {
		case 8:
			for ( i=cvt->len_cvt/2; i; ) {
				for ( j=channels; j; --j,--i ) {
					dst[0] = src[0];
					src += 1;
					dst += 1;
				}
				src += channels;
			}
			break;
		case 16:
			for ( i=cvt->len_cvt/4; i; ) {
				for ( j=channels; j; --j,--i ) {
					dst[0] = src[0];
					dst[1] = src[1];
					src += 2;
					dst += 2;
				}
				src += channels*2;
			}
			break;
	}

	cvt->len_cvt /= 2;

	if (cvt->filters[++cvt->filter_index] ) {
		cvt->filters[cvt->filter_index](cvt, format, channels);
	}
}

/* Very slow rate conversion routine */
void acFreqSLOW(acAudioCVT *cvt, ALushort format, ALushort channels)
{
	double ipos;
	int i, j, clen;

	clen = (int) ((double)((int)cvt->len_cvt / channels) / cvt->rate_incr);
	if(cvt->rate_incr > 1.0) {
		switch(format & 0xFF) {
			case 8: {
				ALubyte *output = cvt->buf;

				clen *= channels;
				ipos = 0.0;
				for ( i=clen; i; ) {
					for ( j=0; j<channels; ++j,--i ) {
						*output=((ALubyte *)cvt->buf)[(int)ipos * channels + j];
						output += 1;
					}
					ipos += cvt->rate_incr;
				}
			}
			break;

			case 16: {
				ALushort *output;

				clen &= ~1;
				clen *= channels;
				output = (ALushort *) cvt->buf;
				ipos = 0.0;
				for ( i=clen/2; i; ) {
					for ( j=0; j<channels; ++j,--i ) {
						*output=((ALushort *)cvt->buf)[(int)ipos * channels + j];
						output += 1;
					}
					ipos += cvt->rate_incr;
				}
			}
			break;

			default: {
				/* unexpected */
			}
			break;
		}
	} else {
		switch (format & 0xFF) {
			case 8: {
				ALubyte *output;

				clen *= channels;
				output = (ALubyte *) cvt->buf + clen;
				ipos = (double)(cvt->len_cvt / channels);
				for ( i=clen; i; ) {
					for ( j=0; j<channels; ++j,--i ) {
						output -= 1;
						*output=((ALubyte *)cvt->buf)[(int)ipos * channels - j - 1];
					}
					ipos -= cvt->rate_incr;
				}
			}
			break;

			case 16: {
				ALushort *output;

				clen &= ~1;
				clen *= channels;
				output = (ALushort *) cvt->buf;
				output += clen / sizeof *output;

				ipos = (double)(cvt->len_cvt/2/channels);
				for ( i=clen/2; i; ) {
					for ( j=0; j<channels; ++j,--i ) {
						output -= 1;
						*output=((ALushort *)cvt->buf)[(int)ipos * channels - j - 1];
					}
					ipos -= cvt->rate_incr;
				}
			}
			break;

			default: {
				/* unexpected */
			}
			break;
		}
	}
	cvt->len_cvt = clen;

	if (cvt->filters[++cvt->filter_index] ) {
		cvt->filters[cvt->filter_index](cvt, format, channels);
	}
}
