#ifndef TEST_TESTLIB_H
#define TEST_TESTLIB_H

#include <AL/alext.h>

#ifdef __cplusplus
extern "C" {
#endif				/* __cplusplus */

/* function pointers for LOKI extensions */
extern PFNALCGETAUDIOCHANNELPROC palcGetAudioChannel;
extern PFNALCSETAUDIOCHANNELPROC palcSetAudioChannel;

extern PFNALREVERBSCALEPROC palReverbScale;
extern PFNALREVERBDELAYPROC palReverbDelay;

extern PFNALBOMBONERRORPROC palBombOnError;

extern PFNALBUFFERIPROC palBufferi;
extern PFNALBUFFERWRITEDATAPROC palBufferWriteData;
extern PFNALBUFFERAPPENDWRITEDATAPROC palBufferAppendWriteData;

extern PFNALCAPTUREINITPROC palCaptureInit;
extern PFNALCAPTUREDESTROYPROC palCaptureDestroy;
extern PFNALCAPTURESTARTPROC palCaptureStart;
extern PFNALCAPTURESTOPPROC palCaptureStop;
extern PFNALCAPTUREGETDATAPROC palCaptureGetData;

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
#endif				/* __cplusplus */

#endif				/* TEST_TESTLIB_H */
