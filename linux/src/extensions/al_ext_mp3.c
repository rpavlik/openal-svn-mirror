/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_ext_mp3.c
 *
 * Temporary hack.
 */
#include "al_siteconfig.h"

#include <AL/al.h>
#include <stdio.h>
#include <string.h>

#include "al_ext_needed.h"
#include "al_ext_mp3.h"

#include "al_buffer.h"
#include "al_ext.h"
#include "alc/alc_context.h"

#define MAX_MP3 64
#define MAX_MPEG_READ 512

#ifdef ENABLE_EXTENSION_AL_EXT_MP3

#include <SDL/SDL.h>
#include <smpeg.h>

#ifdef OPENAL_DLOPEN_SMPEG
#include <dlfcn.h>
#endif


/* maximum MAX_MP3 simultaneous sid/offset */
static struct {
	ALuint bid;
	SMPEG *mpeg;
} mp3bid[MAX_MP3];

static struct {
	ALuint sid;
	ALuint offset;
} mp3map[MAX_MP3];


static int openal_load_smpeg_library(void);

/*
 * smpeg library functions.
 */
static void (*pSMPEG_enablevideo)( SMPEG* mpeg, int enable );
static SMPEGstatus (*pSMPEG_status)( SMPEG* mpeg );
static int (*pSMPEG_wantedSpec)( SMPEG *mpeg, SDL_AudioSpec *wanted );
static void (*pSMPEG_enableaudio)( SMPEG* mpeg, int enable );
static void (*pSMPEG_actualSpec)( SMPEG *mpeg, SDL_AudioSpec *spec );
static void (*pSMPEG_rewind)( SMPEG* mpeg );
static int (*pSMPEG_playAudio)( SMPEG *mpeg, Uint8 *stream, int len );
static void (*pSMPEG_stop)( SMPEG* mpeg );
static SMPEG* (*pSMPEG_new_data)(void *data, int size, SMPEG_Info* info, int sdl_audio);
static void (*pSMPEG_play)( SMPEG* mpeg );
static void (*pSMPEG_delete)( SMPEG* mpeg );

/*
 * smpeg library handle.
 */
static void * smpeg_lib_handle = NULL;

static int openal_load_smpeg_library(void)
{
#ifdef OPENAL_DLOPEN_SMPEG
        char * error = NULL;
#endif
    
	if (smpeg_lib_handle != NULL)
		return 1;  /* already loaded. */

	#ifdef OPENAL_DLOPEN_SMPEG
		#define OPENAL_LOAD_SMPEG_SYMBOL(x) p##x = dlsym(smpeg_lib_handle, #x); \
                                                   error = dlerror(); \
                                                   if (p##x == NULL) { \
                                                           fprintf(stderr,"Could not resolve smpeg symbol %s: %s\n", #x, ((error!=NULL)?(error):("(null)"))); \
                                                           dlclose(smpeg_lib_handle); smpeg_lib_handle = NULL; \
                                                           return 0; }
                dlerror(); /* clear error state */
		smpeg_lib_handle = dlopen("libsmpeg.so", RTLD_LAZY | RTLD_GLOBAL);
                error = dlerror();
		if (smpeg_lib_handle == NULL) {
                        fprintf(stderr,"Could not open smpeg library: %s\n",((error!=NULL)?(error):("(null)")));
			return 0;
                }
	#else
		#define OPENAL_LOAD_SMPEG_SYMBOL(x) p##x = x;
		smpeg_lib_handle = (void *) 0xF00DF00D;
	#endif

        OPENAL_LOAD_SMPEG_SYMBOL(SMPEG_enablevideo);
        OPENAL_LOAD_SMPEG_SYMBOL(SMPEG_status);
        OPENAL_LOAD_SMPEG_SYMBOL(SMPEG_wantedSpec);
        OPENAL_LOAD_SMPEG_SYMBOL(SMPEG_enableaudio);
        OPENAL_LOAD_SMPEG_SYMBOL(SMPEG_actualSpec);
        OPENAL_LOAD_SMPEG_SYMBOL(SMPEG_rewind);
        OPENAL_LOAD_SMPEG_SYMBOL(SMPEG_playAudio);
        OPENAL_LOAD_SMPEG_SYMBOL(SMPEG_stop);
        OPENAL_LOAD_SMPEG_SYMBOL(SMPEG_new_data);
        OPENAL_LOAD_SMPEG_SYMBOL(SMPEG_play);
        OPENAL_LOAD_SMPEG_SYMBOL(SMPEG_delete);

	return 1;
}


#ifdef OPENAL_EXTENSION

/*
 * we are not being build into the library, therefore define the
 * table that informs openal how to register the extensions upon
 * dlopen.
 */
struct { ALubyte *name; void *addr; } alExtension_03282000 [] = {
	AL_EXT_PAIR(alutLoadMP3_LOKI),
	{ NULL, NULL }
};

void alExtInit_03282000(void) {
	int i;

	for(i = 0; i < MAX_MP3; i++) {
		mp3map[i].sid    = 0;
		mp3map[i].offset = 0;

		mp3bid[i].bid  = 0;
		mp3bid[i].mpeg = NULL;
	}

	return;
}

void alExtFini_03282000(void) {
	int i;

	fprintf(stderr, "alExtFini_03282000 STUB\n");

	for(i = 0; i < MAX_MP3; i++) {
		if(mp3bid[i].mpeg != NULL) {
			/* Do something */
		}
	}

	return;
}

#endif

void MP3_DestroyCallback_Sid(ALuint sid);
void MP3_DestroyCallback_Bid(ALuint bid);

static int  mp3bid_get(ALuint bid, SMPEG **mpegp);
static int  mp3bid_insert(ALuint bid, SMPEG *mpeg);
static void mp3bid_remove(ALuint bid);

static int  mp3map_get(ALuint sid, ALuint *offset);
static int  mp3map_insert(ALuint sid);
static void mp3map_update(int i, ALuint offset);
static void mp3map_remove(ALuint sid);

ALboolean alutLoadMP3_LOKI(ALuint bid, ALvoid *data, ALint size) {
	static void (*palBufferi_LOKI)(ALuint, ALenum, ALint) = NULL;
	SMPEG *newMpeg;
	SDL_AudioSpec spec;

	if(palBufferi_LOKI == NULL) {
		palBufferi_LOKI = (void (*)(ALuint, ALenum, ALint))
			    alGetProcAddress((const ALchar *) "alBufferi_LOKI");

		if(palBufferi_LOKI == NULL) {
			fprintf(stderr, "Need alBufferi_LOKI\n");
			return AL_FALSE;
		}
		
		if (!openal_load_smpeg_library())
			return AL_FALSE;

	}

	newMpeg = pSMPEG_new_data( data, size, NULL, 0 );

	pSMPEG_wantedSpec( newMpeg, &spec );

	_alcDCLockContext();

	spec.freq     = canon_speed;
	spec.format   = AUDIO_S16; /* FIXME */

	_alcDCUnlockContext();

	/* implicitly multichannel */
	palBufferi_LOKI( bid, AL_CHANNELS, spec.channels );

	pSMPEG_actualSpec( newMpeg, &spec );

	/* insert new bid */
	mp3bid_insert( bid, newMpeg );

	_alBufferDataWithCallback_LOKI(bid,
				MP3_Callback,
				mp3map_remove,
				mp3bid_remove);

	return AL_TRUE;
}


ALint MP3_Callback(ALuint sid,
		ALuint bid,
		ALshort *outdata,
		ALenum format,
		UNUSED(ALint freq),
		ALint samples) {
	int first;
	int second;
	SMPEG *mpeg;
	ALuint offset;
	int bytesRequested = samples * sizeof( ALshort );
	int bytesPlayed;
	int bps; /* bytes per sample */
	int i;

	if(samples > MAX_MPEG_READ) {
		first  = MP3_Callback(sid, bid, outdata, format, freq, MAX_MPEG_READ);
		second = MP3_Callback(sid, bid, outdata + MAX_MPEG_READ, format, freq, samples - MAX_MPEG_READ);
		return first + second;

	}

	bps = _alGetBitsFromFormat( format );

	/* get buffer specific information */
	i = mp3bid_get( bid, &mpeg );
	if(i == -1) {
		fprintf(stderr, "No buffer id %d in data structures\n", bid);

		return -1; /* weird */
	}

	/* get source specific information */
	i = mp3map_get( sid, &offset );
	if(i == -1) {
		i = mp3map_insert( sid );

		offset = AL_FALSE;

		pSMPEG_enableaudio( mpeg, 1 );
		pSMPEG_enablevideo( mpeg, 0 ); /* sanity check */
	}

	if( pSMPEG_status(mpeg) != SMPEG_PLAYING ) {
		pSMPEG_play( mpeg );
	}

	memset( outdata, 0, (size_t)bytesRequested );

	bytesPlayed = pSMPEG_playAudio( mpeg, (ALubyte *) outdata, bytesRequested );
	bytesPlayed /= 2;

	if(bytesPlayed < samples) {
		pSMPEG_stop( mpeg );
		pSMPEG_rewind( mpeg );

		return bytesPlayed;
	}

	mp3map_update(i, offset + samples);

	return samples;
}

static int mp3bid_get(ALuint bid, SMPEG **mpegp) {
	int i;

	for(i = 0; i < MAX_MP3; i++) {
		if(mp3bid[i].bid == bid) {
			*mpegp = mp3bid[i].mpeg;

			return i;
		}
	}

	return -1;
}

static int mp3bid_insert(ALuint bid, SMPEG *mpeg) {
	int i;

	for(i = 0; i < MAX_MP3; i++) {
		if(mp3bid[i].bid == bid) {
			if(mp3bid[i].mpeg != NULL) {
				pSMPEG_stop( mp3bid[i].mpeg );
				pSMPEG_delete( mp3bid[i].mpeg );
				mp3bid[i].mpeg = NULL;
			}

			mp3bid[i].bid = 0; /* flag for next */
		}

		if(mp3bid[i].bid == 0) {
			mp3bid[i].bid  = bid;
			mp3bid[i].mpeg = mpeg;

			return i;
		}
	}

	return 0;
}

static void mp3bid_remove( ALuint bid ) {
	int i = 0;

	for(i = 0; i < MAX_MP3; i++) {
		if(mp3bid[i].bid == (ALuint) bid) {
			if(mp3bid[i].mpeg != NULL) {
				pSMPEG_stop( mp3bid[i].mpeg );
				pSMPEG_delete(mp3bid[i].mpeg);
				mp3bid[i].mpeg = NULL;
			}

			mp3bid[i].bid = 0;
			return;
		}
	}

	return;
}

/* FIXME: make binary search */
static int mp3map_get(ALuint sid, ALuint *offset) {
	int i;

	for(i = 0; i < MAX_MP3; i++) {
		if(mp3map[i].sid == sid) {
			*offset = mp3map[i].offset;

			return i;
		}
	}

	return -1;
}

/* FIXME: sorted insert for binary search */
static int mp3map_insert(ALuint sid) {
	int i;

	for(i = 0; i < MAX_MP3; i++) {
		if((mp3map[i].sid == 0) ||
		   (mp3map[i].sid == sid)) {
			mp3map[i].sid    = sid;
			mp3map[i].offset  = AL_FALSE;

			return i;
		}
	}

	return 0;
}

static void mp3map_update(int i, ALuint offset) {
	if(i < 0) {
		return;
	}

	if(i >= MAX_MP3) {
		return;
	}

	mp3map[i].offset = offset;

	return;
}

static void mp3map_remove(ALuint sid) {
	int i;

	for(i = 0; i < MAX_MP3; i++) {
		if(mp3map[i].sid == sid) {
			mp3map[i].sid   = 0;
			mp3map[i].offset = AL_FALSE;

			return;
		}
	}

	return;
}

#else

/* without smpeg support, we don't do jack */

#endif /* ENABLE_EXTENSION_AL_EXT_MP3 */
