#ifndef TEST_TESTLIB_H
#define TEST_TESTLIB_H

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>
#include <AL/alext.h>

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif /* M_PI */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* function pointers for LOKI extensions */
extern PFNALCGETAUDIOCHANNELPROC palcGetAudioChannel;
extern PFNALCSETAUDIOCHANNELPROC palcSetAudioChannel;

extern PFNALREVERBSCALEPROC palReverbScale;
extern PFNALREVERBDELAYPROC palReverbDelay;

extern PFNALBOMBONERRORPROC palBombOnError;

extern PFNALBUFFERIPROC palBufferi;
extern PFNALBUFFERWRITEDATAPROC palBufferWriteData;
extern PFNALBUFFERAPPENDWRITEDATAPROC palBufferAppendWriteData;


/* new ones */
extern PFNALGENSTREAMINGBUFFERSPROC palGenStreamingBuffers;
extern PFNALUTLOADRAW_ADPCMDATAPROC palutLoadRAW_ADPCMData;

void microSleep( unsigned int n );
void getExtensionEntries( void );
ALboolean sourceIsPlaying( ALuint sid );
void _RotatePointAboutAxis( const ALfloat angle, ALfloat *point,
                              const ALfloat *axis );
ALuint CreateBufferFromFile( const char *fileName );
void testInit( int *argcp, char **argv );
void testInitWithoutContext( int *argcp, char **argv );
void testExit( void );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* TEST_TESTLIB_H */
