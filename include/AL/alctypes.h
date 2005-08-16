#ifndef _AL_ALCTYPES_H
#define _AL_ALCTYPES_H

#if !defined(_WIN32)
struct _AL_device;
typedef struct _AL_device ALCdevice;

typedef void ALCcontext;
#endif /* _WIN32 */

/** 8-bit boolean */
typedef char ALCboolean;

/** character */
typedef char ALCchar;

/** signed 8-bit 2's complement integer */
typedef char ALCbyte;

/** unsigned 8-bit integer */
typedef unsigned char ALCubyte;

/** signed 16-bit 2's complement integer */
typedef short ALCshort;

/** unsigned 16-bit integer */
typedef unsigned short ALCushort;

/** signed 32-bit 2's complement integer */
typedef int ALCint;

/** unsigned 32-bit integer */
typedef unsigned int ALCuint;

/** non-negative 32-bit binary integer size */
typedef int ALCsizei;

/** enumerated 32-bit value */
typedef int ALCenum;

/** 32-bit IEEE754 floating-point */
typedef float ALCfloat;

/** 64-bit IEEE754 floating-point */
typedef double ALCdouble;

/** void type (for opaque pointers only) */
typedef void ALCvoid;

/* Enumerant values begin at column 50. No tabs. */

/* bad value */
#define ALC_INVALID                              0

/* Boolean False. */
#define ALC_FALSE                                0

/* Boolean True. */
#define ALC_TRUE                                 1

/**
 * followed by <int> Hz
 */
#define ALC_FREQUENCY                            0x1007

/**
 * followed by <int> Hz
 */
#define ALC_REFRESH                              0x1008

/**
 * followed by AL_TRUE, AL_FALSE
 */
#define ALC_SYNC                                 0x1009

/**
 * followed by <int> Num of requested Mono (3D) Sources
 */
#define ALC_MONO_SOURCES                         0x1010

/**
 * followed by <int> Num of requested Stereo Sources
 */
#define ALC_STEREO_SOURCES                       0x1011

/**
 * errors
 */

/**
 * No error
 */
#define ALC_NO_ERROR                             ALC_FALSE

/**
 * No device
 */
#define ALC_INVALID_DEVICE                       0xA001

/**
 * invalid context ID
 */
#define ALC_INVALID_CONTEXT                      0xA002

/**
 * bad enum
 */
#define ALC_INVALID_ENUM                         0xA003

/**
 * bad value
 */
#define ALC_INVALID_VALUE                        0xA004

/**
 * Out of memory.
 */
#define ALC_OUT_OF_MEMORY                        0xA005



/**
 * The Specifier string for default device
 */
#define ALC_DEFAULT_DEVICE_SPECIFIER             0x1004
#define ALC_DEVICE_SPECIFIER                     0x1005
#define ALC_EXTENSIONS                           0x1006

#define ALC_MAJOR_VERSION                        0x1000
#define ALC_MINOR_VERSION                        0x1001

#define ALC_ATTRIBUTES_SIZE                      0x1002
#define ALC_ALL_ATTRIBUTES                       0x1003

/**
 * Capture extension
 */
#define ALC_CAPTURE_DEVICE_SPECIFIER             0x310
#define ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER     0x311
#define ALC_CAPTURE_SAMPLES                      0x312


#endif /* _AL_ALCTYPES_H */
