/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_ext_vorbis.h
 *
 * Hack.
 */
#ifndef AL_EXTENSIONS_AL_EXT_VORBIS_H_
#define AL_EXTENSIONS_AL_EXT_VORBIS_H_

#include "al_siteconfig.h"
#include <AL/al.h>

/*
 * Vorbis_Callback( ALuint sid, ALuint bid,
 *                  ALshort *outdata,
 *                  ALenum format, ALint freq, ALint samples )
 *
 * Callback which enabled alutLoadVorbis_LOKI to work.
 *
 */
ALint Vorbis_Callback(ALuint sid, ALuint bid,
			     ALshort *outdata,
			     ALenum format, ALint freq, ALint samples);

#ifndef OPENAL_EXTENSION

#ifdef ENABLE_EXTENSION_AL_EXT_VORBIS


/*
 * we are being built into the standard library, so inform
 * the extension registrar
 */
#define BUILTIN_EXT_VORBIS                                       \
	AL_EXT_PAIR(alutLoadVorbis_LOKI)

/* initialization and destruction functions
 *
 * none needed for vorbis
 */

#define BUILTIN_EXT_VORBIS_INIT
#define BUILTIN_EXT_VORBIS_FINI

#else /* not ENABLE_EXTENSION_AL_EXT_VORBIS */

/*
 * Without libvorvis (--enable-vorbis) we can't do vorbis, so we
 * don't define any extensions, or init/fini functions.
 */

#define BUILTIN_EXT_VORBIS_INIT
#define BUILTIN_EXT_VORBIS_FINI

#endif /* not ENABLE_EXTENSION_AL_EXT_VORBIS */

#endif /* not OPENAL_EXTENSION */

#endif /* AL_EXTENSIONS_AL_EXT_VORBIS_H_ */
