/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * alut_wav.c
 *
 * Loki's wave file loader.
 *
 * FIXME: error handling?
 */
#include "al_siteconfig.h"

#include <AL/al.h>
#include <AL/alut.h>

#include "al_types.h"
#include "al_main.h"
#include "al_buffer.h"
#include "al_debug.h"
#include "alc/alc_context.h"

#include "audioconvert.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string.h>

static ALboolean ReadWAVFile(const char *fname, void **pcmdata,
			ALushort *rfmt, ALushort *rchan,
			ALushort *rfreq, ALuint *rsize);
static ALboolean ReadWAVMemory(const ALvoid *data, void **pcmdata,
			       ALushort *rfmt, ALushort *rchan,
			       ALushort *rfreq, ALuint *rsize);

ALboolean alutLoadWAV( const char *fname,
                        void **wave,
			ALsizei *format,
			ALsizei *size,
			ALsizei *bits,
			ALsizei *freq ) {
	ALushort alFmt  = 0;
	ALushort acChan = 0;
	ALushort acFreq = 0;
	ALuint   acSize = 0;

	if(ReadWAVFile(fname, wave,
			&alFmt, &acChan, &acFreq, &acSize) == AL_FALSE) {
		_alDebug(ALD_CONVERT, __FILE__, __LINE__,
			"ReadWAVFile failed for %s", fname);
		return AL_FALSE;
	}

	/* set output params */
	*format = (ALsizei) alFmt;
	*freq   = (ALsizei) acFreq;
	*size   = (ALsizei) acSize;
	*bits   = (ALsizei) _al_formatbits(alFmt);

	_alDebug(ALD_CONVERT, __FILE__, __LINE__,
		"alutLoadWAV %s with [alformat/size/bits/freq] = [0x%x/%d/%d]",
		fname,
		*format, *size, *freq);

	return AL_TRUE;
}

static ALboolean ReadWAVFile(const char *fname,
			     void **pcmdata,
			     ALushort *rfmt,
			     ALushort *rchan,
			     ALushort *rfreq,
			     ALuint *rsize) {
	void *data;
	ALint len;  /* length of pcm portion */
	void *udata;

	if((rfmt == NULL) || (rchan == NULL) || (rfreq == NULL)) {
		/* bad mojo */
		return AL_FALSE;
	}

	len = _alSlurp(fname, &data);
	if(len < 0) {
		/* couldn't slurp audio file */
		_alDebug(ALD_CONVERT, __FILE__, __LINE__,
			"Could not slurp %s", fname);

		return AL_FALSE;
	}

	if(acLoadWAV(data, (ALuint *) &len, &udata,
		rfmt, rchan, rfreq) == NULL) {
		_alDebug(ALD_CONVERT, __FILE__, __LINE__,
			"Could not buffer and convert data");

		free(data);
		return AL_FALSE;
	}

	free(data);

	*rfmt    = _al_AC2ALFMT(*rfmt, *rchan);
	*rsize   = len;
	*pcmdata = udata;

	_alDebug(ALD_CONVERT, __FILE__, __LINE__,
		"ReadWAVFile [freq/size/acformat] = [%d/%d/0x%x]",
		*rfreq, *rsize, *rfmt);

	return AL_TRUE;
}

static ALboolean ReadWAVMemory(const ALvoid *data,
			       void **pcmdata,
			       ALushort *rfmt,
			       ALushort *rchan,
			       ALushort *rfreq,
			       ALuint *rsize)
{
	void *mutable;
	ALuint len;
	void *udata;

	if((rfmt == NULL) || (rchan == NULL) || (rfreq == NULL)) {
		/* bad mojo */
		return AL_FALSE;
	}

	/*
	 * o/~ I'm an asshole o/~
	 *
	 * Don't worry, it should be safe
	 */
	memcpy(&mutable, &data, sizeof(void *));

	if(acLoadWAV(mutable,
		     &len,
		     &udata,
		     rfmt,
		     rchan,
		     rfreq) == NULL)
	{
		_alDebug(ALD_CONVERT, __FILE__, __LINE__,
			"Could not buffer and convert data");

		return AL_FALSE;
	}

	*rfmt    = _al_AC2ALFMT(*rfmt, *rchan);
	*rsize   = len;
	*pcmdata = udata;

	_alDebug(ALD_CONVERT, __FILE__, __LINE__,
		"ReadWAVMemory [freq/size/acformat] = [%d/%d/0x%x]",
		*rfreq, *rsize, *rfmt);

	return AL_TRUE;
}

/* I'm such a nice guy */
void alutLoadWAVFile(ALbyte *fname,
		       ALenum *format,
		       ALvoid **data,
		       ALsizei *size,
		       ALsizei *freq,
		       ALboolean *loop)
{
	ALboolean ret;
	ALsizei bits_dummy;

	ret = alutLoadWAV( (const char *) fname,
			   data,
			   format,
			   size,
			   &bits_dummy,
			   freq );

	if(loop)
	{
		*loop = AL_FALSE;
	}

	return;
}

void alutLoadWAVMemory(ALbyte *memory,
		       ALenum *format,
		       ALvoid **data,
		       ALsizei *size,
		       ALsizei *freq,
		       ALboolean *loop)
{
	ALushort alFmt  = 0;
	ALushort acChan = 0;
	ALushort acFreq = 0;
	ALuint   acSize = 0;

	if(!ReadWAVMemory(memory, data,
			&alFmt, &acChan, &acFreq, &acSize))
	{
		_alDebug(ALD_CONVERT, __FILE__, __LINE__,
			"ReadWAVMemory failed");

		return;
	}

	/* set output params */
	*format = (ALsizei) alFmt;
	*freq   = (ALsizei) acFreq;
	*size   = (ALsizei) acSize;
	*loop   = AL_FALSE; /* JIV FIXME */
	/* *bits   = (ALsizei) _al_formatbits(alFmt); */

	_alDebug(ALD_CONVERT, __FILE__, __LINE__,
		"alutLoadWAVMemory with [format/size/bits/freq] = [0x%x/%d/%d]",
		*format, *size, *freq);

	return;
}

void alutUnloadWAV(UNUSED(ALenum format),
		   ALvoid *data,
		   UNUSED(ALsizei size),
		   UNUSED(ALsizei freq))
{
	free(data);
}
