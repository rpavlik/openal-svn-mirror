/* -*- mode: C; tab-width:8; c-basic-offset:8 -*-
 * vi:set ts=8:
 *
 * al_ext_vorbis.c
 *
 * Temporary hack.
 */
#include "al_siteconfig.h"

#include <AL/al.h>

/* for alutLoadVorbis_LOKI */
#include "AL/alext.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "al_ext_needed.h"
#include "al_ext_vorbis.h"
#include "al_debug.h"
#include "al_buffer.h"
#include "al_ext.h"
#include "alc/alc_context.h"

#define MAX_VORBIS 64

#ifdef ENABLE_EXTENSION_AL_EXT_VORBIS
#include <vorbis/vorbisfile.h>

#include "al_dlopen.h"

static size_t ovfd_read(void *ptr, size_t size, size_t nmemb, void *datasource);
static int ovfd_seek(void *datasource, int64_t offset, int whence);
static int ovfd_close(void *datasource);
static long ovfd_tell(void *datasource);

typedef struct {
	OggVorbis_File of;

	void *data;

	struct {
		int size;
		int offset; /* set by the source */
	} fh;
} VorbHandle;

static ov_callbacks ov_fromdata = {
	ovfd_read,
	ovfd_seek,
	ovfd_close,
	ovfd_tell
};

/* maximum MAX_VORBIS simultaneous sid/offset */
static struct {
	ALuint bid;
	VorbHandle *vorb;
} vorbid[MAX_VORBIS];

static struct {
	ALuint sid;
	ALuint offset;
	ALint current_section;
} vorbmap[MAX_VORBIS];

#ifdef OPENAL_EXTENSION

/*
 * we are not being build into the library, therefore define the
 * table that informs openal how to register the extensions upon
 * dlopen.
 */
struct { ALubyte *name; void *addr; } alExtension_03282000 [] = {
	AL_EXT_PAIR(alutLoadVorbis_LOKI),
	{ NULL, NULL }
};

void alExtInit_03282000(void) {
	int i;

	for(i = 0; i < MAX_VORBIS; i++) {
		vorbmap[i].sid     = 0;
		vorbmap[i].offset  = 0;

		vorbid[i].bid  = 0;
		vorbid[i].vorbis = NULL;
	}

	return;
}

void alExtFini_03282000(void) {
	int i;

	fprintf(stderr, "alExtFini_03282000 STUB\n");

	for(i = 0; i < MAX_VORBIS; i++) {
		if(vorbid[i].vorbis != NULL) {
			/* Do something */
		}
	}

	return;
}

#endif

static int signed_format(ALenum format);

static void VorbHandle_delete(VorbHandle *vorb);

static int  vorbid_get(ALuint bid, VorbHandle **vorbp);
static int  vorbid_insert(ALuint bid, VorbHandle *vorb);
static void vorbid_remove(ALuint bid);

static int  vorbmap_get(ALuint sid, ALuint *offset, ALint *current_section);
static int  vorbmap_insert(ALuint sid);
static void vorbmap_update(int i, ALuint offset, ALint current_section);
static void vorbmap_remove(ALuint sid);


static int openal_load_vorbisfile_library(void);

/*
 * vorbisfile library functions.
 */
static int (*pov_clear)(OggVorbis_File *vf);
static int (*pov_open_callbacks)(void *datasource, OggVorbis_File *vf, char *initial, long ibytes, ov_callbacks callbacks);
static vorbis_info *(*pov_info)(OggVorbis_File *vf, int link);
static long (*pov_read)(OggVorbis_File *vf, char *buffer, int length, int bigendianp, int word, int sgned, int *bitstream);

#define VORBISFILE_LIBRARY "libvorbisfile.so"

#ifdef OPENAL_DLOPEN_VORBIS
#define myDlopen(n) alDLOpen_(n)
#define myDlerror() alDLError_()
#define myDlsym(h,t,s) (t alDLFunSym_(h,#s))
#define myDlclose(h) alDLClose_(h)
#else
#define myDlopen(n) ((AL_DLHandle)0xF00DF00D)
#define myDlerror() ""
#define myDlsym(h,t,s) (&s)
#define myDlclose(h)
#endif

#define OPENAL_LOAD_VORBISFILE_SYMBOL(h,t,s)		\
  p##s = myDlsym(h, t, s);			\
  if (p##s == NULL) \
    { \
      _alDebug (ALD_CONTEXT, __FILE__, __LINE__, \
                "could not resolve symbol '%s': %s", \
                #s, myDlerror ()); \
      myDlclose (h); \
      h = NULL; \
      return 0; \
    }

static int openal_load_vorbisfile_library(void)
{
	static AL_DLHandle handle = (AL_DLHandle)0;
    
	/* already loaded? */
	if (handle != (AL_DLHandle)0)
		return 1;

	/* clear error state */
	(void) myDlerror ();

	handle = myDlopen (VORBISFILE_LIBRARY);
	if (handle == (AL_DLHandle) 0)
		{
			_alDebug (ALD_CONTEXT, __FILE__, __LINE__,
				  "could not open '%s': %s", VORBISFILE_LIBRARY, myDlerror ());
			return 0;
		}

	OPENAL_LOAD_VORBISFILE_SYMBOL(handle, (int (*)(OggVorbis_File*)), ov_clear);
	OPENAL_LOAD_VORBISFILE_SYMBOL(handle, (int (*)(void*, OggVorbis_File*, char*, long, ov_callbacks)), ov_open_callbacks);
	OPENAL_LOAD_VORBISFILE_SYMBOL(handle, (vorbis_info *(*)(OggVorbis_File*, int)), ov_info);
	OPENAL_LOAD_VORBISFILE_SYMBOL(handle, (long (*)(OggVorbis_File*, char*, int, int, int, int, int*)), ov_read);

	return 1;
}


ALboolean alutLoadVorbis_LOKI(ALuint bid,
			      const ALvoid *data,
			      ALint size) {
	static void (*palBufferi_LOKI)(ALuint, ALenum, ALint) = NULL;
	VorbHandle *vorb;
	int err;
	vorbis_info *vi;

	if(palBufferi_LOKI == NULL) {
		palBufferi_LOKI = (void (*)(ALuint, ALenum, ALint))
			alGetProcAddress((const ALchar *) "alBufferi_LOKI");

		if(palBufferi_LOKI == NULL) {
			fprintf(stderr, "Need alBufferi_LOKI\n");
			return AL_FALSE;
		}
		
		if (!openal_load_vorbisfile_library())
			return AL_FALSE;
	}

	vorb = malloc(sizeof *vorb);
	if(vorb == NULL) {
		fprintf(stderr, "vorbis problems\n");

		return AL_FALSE;
	}

	vorb->data = malloc(size);
	if(vorb->data == NULL) {
		fprintf(stderr, "vorbis out of memory \n");
		free(vorb);
		return AL_FALSE;
	}

	memcpy(vorb->data, data, size);

	vorb->fh.offset = 0;
	vorb->fh.size   = size;

	/* NOTE: Ogg Vorbis' header are not const-correct, so we cast */
	err = pov_open_callbacks(vorb, &vorb->of, (ALvoid*)data, size, ov_fromdata);
	if(err < 0) {
		fprintf(stderr, "vorbis problems\n");
		free(vorb->data);
		free(vorb);
		return AL_FALSE;
	}

	/* set multi channel stuff */
	vi = pov_info(&vorb->of, 0);
	if(vi != NULL) {
		palBufferi_LOKI(bid, AL_CHANNELS, vi->channels);
	}

	/* insert new bid */
	vorbid_insert(bid, vorb);

	_alBufferDataWithCallback_LOKI(bid,
				Vorbis_Callback,
				vorbmap_remove,
				vorbid_remove);

	return AL_TRUE;
}


ALint Vorbis_Callback(UNUSED(ALuint sid),
		ALuint bid,
		ALshort *outdata,
		ALenum format,
		UNUSED(ALint freq),
		ALint samples) {
	VorbHandle *vorb;
	int index;
	ALuint offset;
	long ret = 0;
	int bps = _alGetBitsFromFormat(format)>>3; /* bytes per sample */
	int bytesToRead = samples * bps;
	ALint current_section;
	ALint retval = 0;
	char *datap;

	datap = (char *) outdata;

	index = vorbid_get(bid, &vorb);
	if(index == -1) {
		fprintf(stderr, "weird vorbid_get\n");
		return -1;
	}

	index = vorbmap_get(sid, &offset, &current_section);
	if(index == -1) {
		/* first time */
		index = vorbmap_insert(sid);

		offset = 0;
		current_section = 0;
	}

	vorb->fh.offset = offset; /* set per sid offset */

	while(bytesToRead > 0) {
		/* FIXME: handle format differences etc
		 *
		 * should be below VDRATB now */
		ret = pov_read(&vorb->of,
			      datap,
			      bytesToRead,
#ifdef WORDS_BIGENDIAN
			      1,
#else
			      0,
#endif
			      bps,
			      signed_format(format),
			      &current_section);

		if(ret == OV_HOLE)
			continue;

		if(ret <= 0) {
			/* eof or error */
			vorbmap_update(index, 0, 0);
			return 0;
		}

		bytesToRead -= ret;
		retval      += ret;
		datap       += ret;
	}

	vorbmap_update(index, vorb->fh.offset, current_section);

	return retval / bps;
}

static int vorbid_get(ALuint bid, VorbHandle **vorbp) {
	int i;

	for(i = 0; i < MAX_VORBIS; i++) {
		if(vorbid[i].bid == bid) {
			*vorbp = vorbid[i].vorb;

			return i;
		}
	}

	return -1;
}

static int vorbid_insert(ALuint bid, VorbHandle *vorb) {
	int i;

	for(i = 0; i < MAX_VORBIS; i++) {
		if(vorbid[i].bid == bid) {
			if(vorbid[i].vorb != NULL) {
				VorbHandle_delete(vorbid[i].vorb);
				vorbid[i].vorb = NULL;
			}

			vorbid[i].bid = 0; /* flag for next */
		}

		if(vorbid[i].bid == 0) {
			vorbid[i].bid  = bid;
			vorbid[i].vorb = vorb;

			return i;
		}
	}

	return 0;
}

static void vorbid_remove(ALuint bid) {
	int i = 0;

	for(i = 0; i < MAX_VORBIS; i++) {
		if(vorbid[i].bid == (ALuint) bid) {
			if(vorbid[i].vorb != NULL) {
				VorbHandle_delete(vorbid[i].vorb);
				vorbid[i].vorb = NULL;
			}

			vorbid[i].bid = 0;
			return;
		}
	}

	return;
}

/* FIXME: make binary search */
static int vorbmap_get(ALuint sid, ALuint *offset, ALint *current_section) {
	int i;

	for(i = 0; i < MAX_VORBIS; i++) {
		if(vorbmap[i].sid == sid) {
			*offset		 = vorbmap[i].offset;
			*current_section = vorbmap[i].current_section;

			return i;
		}
	}

	return -1;
}

/* FIXME: sorted insert for binary search */
static int vorbmap_insert(ALuint sid) {
	int i;

	for(i = 0; i < MAX_VORBIS; i++) {
		if((vorbmap[i].sid == 0) ||
		   (vorbmap[i].sid == sid)) {
			vorbmap[i].sid    = sid;
			vorbmap[i].offset  = AL_FALSE;

			return i;
		}
	}

	return 0;
}

static void vorbmap_update(int i, ALuint offset, ALint current_section) {
	if(i < 0) {
		return;
	}

	if(i >= MAX_VORBIS) {
		return;
	}

	vorbmap[i].offset          = offset;
	vorbmap[i].current_section = current_section;

	return;
}

static void vorbmap_remove(ALuint sid) {
	int i;

	for(i = 0; i < MAX_VORBIS; i++) {
		if(vorbmap[i].sid == sid) {
			vorbmap[i].sid   = 0;
			vorbmap[i].offset = AL_FALSE;

			return;
		}
	}

	return;
}

static void VorbHandle_delete(VorbHandle *vorb) {
	pov_clear(&vorb->of);
	free(vorb->data);
	free(vorb);

	return;
}

static size_t ovfd_read(void *ptr, size_t size, size_t nmemb, void *datasource){
	VorbHandle *vb = datasource;
	int bytesToRead;
	char *datap;

	bytesToRead = size * nmemb;
	datap = vb->data;

	if(vb->fh.offset >= vb->fh.size) {
		/* no more reading, we're at the end */
		return 0;
	}

	if(vb->fh.offset + bytesToRead >= vb->fh.size) {
		bytesToRead = vb->fh.size - vb->fh.offset;
	}

	datap += vb->fh.offset;

	memcpy(ptr, datap, bytesToRead);

	vb->fh.offset += bytesToRead;

	return bytesToRead;
}

static int ovfd_seek(void *datasource, int64_t offset, int whence) {
	VorbHandle *vb = datasource;

	switch(whence) {
		case SEEK_SET:
			if(vb->fh.size < offset) {
				return -1;
			}
			vb->fh.offset = offset;
			break;
		case SEEK_CUR:
			if(vb->fh.size < vb->fh.offset + offset) {
				return -1;
			}

			vb->fh.offset += offset;
			break;
		case SEEK_END:
			if(vb->fh.size < vb->fh.size + offset) {
				return -1;
			}

			vb->fh.offset = vb->fh.size + offset;
			break;
	}

	return 0;
}

static int ovfd_close(void *datasource) {
	VorbHandle *vb = datasource;

	vb->fh.offset = 0;

	return 0;
}

static long ovfd_tell(void *datasource) {
	VorbHandle *vb = datasource;

	return vb->fh.offset;
}

static int signed_format(ALenum format) {
	switch(format) {
		case AL_FORMAT_MONO16:
		case AL_FORMAT_STEREO16:
			return 1;
			break;
		default:
			break;
	}

	return 0;
}

#else

/* without vorbis support, we don't do jack */

#endif /* ENABLE_EXTENSION_AL_EXT_VORBIS */
