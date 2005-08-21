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

#include <AL/alut.h>
#include <stdlib.h>
#include <string.h>

#include "al_main.h"
#include "al_debug.h"

#include "audioconvert/audioconvert.h"

static ALboolean ReadWAVFile(const char *fname, void **pcmdata,
			ALushort *rfmt, ALushort *rchan,
			ALushort *rfreq, ALuint *rsize);
static ALboolean ReadWAVMemory(const ALvoid *data, void **pcmdata,
			       ALushort *rfmt, ALushort *rchan,
			       ALushort *rfreq, ALuint *rsize);

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
void alutLoadWAVFile(  const ALbyte *fname,
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

	if(ReadWAVFile(fname, data,
			&alFmt, &acChan, &acFreq, &acSize) == AL_FALSE) {
		_alDebug(ALD_CONVERT, __FILE__, __LINE__,
			"ReadWAVFile failed for %s", fname);
		return;
	}

	/* set output params */
	*format = (ALenum)  alFmt;
	*freq   = (ALsizei) acFreq;
	*size   = (ALsizei) acSize;
	if(loop)
	{
		*loop = AL_FALSE;
	}

	_alDebug(ALD_CONVERT, __FILE__, __LINE__,
		"alutLoadWAV %s with [alformat/size/freq] = [0x%x/%d/%d]",
		fname,
		*format, *size, *freq);
}

void alutLoadWAVMemory(const ALbyte *memory,
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
