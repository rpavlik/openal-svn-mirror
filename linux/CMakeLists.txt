# cmake project file by Prakash Punnoor
CMAKE_MINIMUM_REQUIRED(VERSION 2.0)

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/admin/CMakeModules")
PROJECT(OpenAL C)
SET(PACKAGE "openal")
SET(PACKAGE_TARNAME "openal")
SET(PACKAGE_NAME "openal")
SET(PACKAGE_MAJOR_VERSION "0")
SET(PACKAGE_MINOR_VERSION "0")
SET(PACKAGE_BUILD_VERSION "8")
SET(PACKAGE_VERSION "${PACKAGE_MAJOR_VERSION}.${PACKAGE_MINOR_VERSION}.${PACKAGE_BUILD_VERSION}")

SET(PACKAGE_STRING "openal")
SET(PACKAGE_BUGREPORT "openal-devel@opensource.creative.com")
SET(OPERATING_SYSTEM "${CMAKE_SYSTEM_NAME}")

# config.h magic
SET(CONFIG_H_VALS_OLD "${CONFIG_H_VALS}")
SET(CONFIG_H_VALS "" CACHE INTERNAL "")
SET(CONFIG_H_FILE ${CMAKE_BINARY_DIR}/config.h)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})


MACRO(ADD_DEFINE ARG)
SET(CONFIG_H_CONTENT "${CONFIG_H_CONTENT}#define ${ARG}

")
#can't cache mulitline values
SET(CONFIG_H_VALS "${CONFIG_H_VALS}${ARG}" CACHE INTERNAL "")
ENDMACRO(ADD_DEFINE ARG)

MACRO(MACRO_TEST ARG VAR)
CHECK_C_SOURCE_COMPILES(
"#ifndef ${ARG}
#error ${ARG} macro not defined
#endif
int main(){}
" ${VAR})
ENDMACRO(MACRO_TEST ARG VAR)

ADD_DEFINE("PACKAGE_VERSION \"${PACKAGE_VERSION}\"")

# We need ISO C99 features for e.g. snprintf.
ADD_DEFINITIONS(-D_ISOC99_SOURCE)
ADD_DEFINE("SIZEOF_VOID_P ${CMAKE_SIZEOF_VOID_P}")

INCLUDE(${CMAKE_ROOT}/Modules/CheckCSourceCompiles.cmake)
INCLUDE(${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
INCLUDE(${CMAKE_ROOT}/Modules/CheckLibraryExists.cmake)
INCLUDE(${CMAKE_ROOT}/Modules/TestBigEndian.cmake)

INCLUDE_DIRECTORIES(${OpenAL_SOURCE_DIR}/../include)
INCLUDE_DIRECTORIES(${OpenAL_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${OpenAL_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${OpenAL_SOURCE_DIR}/src/arch)


SET(AL_OBJS              src/al_bpool.c
                         src/al_buffer.c
                         src/al_config.c
                         src/al_debug.c
                         src/al_distance.c
                         src/al_error.c
                         src/al_ext.c
                         src/al_filter.c
                         src/al_listen.c
                         src/al_main.c
                         src/al_mixer.c
                         src/al_mixfunc.c
                         src/al_mixmanager.c
                         src/al_mspool.c
                         src/al_mutexlib.c
                         src/al_queue.c
                         src/al_rctree.c
                         src/al_rcvar.c
                         src/al_source.c
                         src/al_spool.c
                         src/al_state.c
                         src/al_threadlib.c
                         src/al_vector.c
                         src/mixaudio16.c)

SET(ALC_OBJS             src/alc/alc_context.c
                         src/alc/alc_speaker.c
                         src/alc/alc_error.c
                         src/alc/alc_device.c)

SET(AC_OBJS              src/audioconvert/ac_misc.c
                         src/audioconvert/ac_freq.c
                         src/audioconvert/ac_bits.c
                         src/audioconvert/ac_channels.c
                         src/audioconvert/ac_helper.c
                         src/audioconvert/ac_adpcm.c
                         src/audioconvert/ac_endian.c)

SET(EXT_OBJS             src/extensions/al_ext_loki.c
                         src/extensions/al_ext_mp3.c
                         src/extensions/al_ext_vorbis.c
                         src/extensions/al_ext_capture.c)

SET(BSD_OBJS             src/arch/bsd/bsd_dsp.c)
SET(IRIS_OBJS            src/arch/irix/iris.c)
SET(LINUX_OBJS           src/arch/linux/lin_dsp.c)
SET(SOLARIS_OBJS         src/arch/solaris/solaris_native.c)
SET(DARWIN_OBJS          src/arch/darwin/darwin_native.c)
SET(MORPHOS_OBJS         src/arch/morphos/morphos_native.c
                         src/arch/morphos/morphos_dispatcher.c)
SET(WINDOWS_OBJS         src/arch/windows/windows_native.c)

SET(x86_OBJS             src/arch/i386/floatmul.c
                         src/arch/i386/x86_cpu_caps_prk.c)
SET(x86_MMX_OBJS         src/arch/i386/mixaudio16_mmx_prk.c)
SET(x86_NASM_OBJS        src/arch/i386/x86_cpu_caps_detect_prk.nasm)
SET(x86_MMX_NASM_OBJS    src/arch/i386/memcpy_mmx_prk.nasm)

SET(PORTABLE_OBJS        src/arch/portable/floatmul.c
                         src/arch/portable/cpu_caps.c)

SET(INTERFACE_OBJS       src/arch/interface/interface_sound.c)
SET(ARTS_OBJS            src/arch/arts/arts.c)
SET(ALSA_OBJS            src/arch/alsa/alsa.c)
SET(ESD_OBJS             src/arch/esd/esd.c)
SET(SDL_OBJS             src/arch/sdl/sdl.c)
SET(WAVEOUT_OBJS         src/arch/waveout/waveout.c)
SET(NULL_OBJS            src/arch/null/null.c)

SET(BSD_OBJS             src/arch/bsd/bsd_dsp.c)
SET(IRIS_OBJS            src/arch/irix/iris.c)
SET(LIN_OBJS             src/arch/linux/lin_dsp.c)
SET(SOLARIS_OBJS         src/arch/solaris/solaris_native.c)
SET(DARWIN_OBJS          src/arch/darwin/darwin_native.c)
SET(MORPHOS_OBJS         src/arch/morphos/morphos_native.c
                         src/arch/morphos/morphos_dispatcher.c)
SET(WINDOWS_OBJS         src/arch/windows/windows_native.c)

SET(REDUNDANT_DECLS_WARN ON)       # warnings about redudant decls
SET(SHADOW_WARN ON)                # warn about shadow variables
SET(BROKEN_PTHREAD_FLAG OFF)       # BSD uses -pthread, not -lpthread
SET(NO_PTHREAD_FLAG OFF)

SET(ADD_CFLAGS "")
SET(ADD_LIBS "")
SET(ADD_LDFLAGS "")
SET(WFLAGS "")

IF(CMAKE_UNAME)
  EXEC_PROGRAM(uname ARGS -m OUTPUT_VARIABLE CMAKE_SYSTEM_MACHINE)
ELSE(CMAKE_UNAME)
  MESSAGE("Could not detect machine type")
ENDIF(CMAKE_UNAME)

IF(CMAKE_SYSTEM MATCHES CYGWIN OR CMAKE_SYSTEM MATCHES MINGW)
  SET(CMAKE_SYSTEM Windows)
  IF(CMAKE_SYSTEM MATCHES CYGWIN)
    SET(CYGWIN ON)
  ELSE(CMAKE_SYSTEM MATCHES CYGWIN)
    SET(MINGW ON)
  ENDIF(CMAKE_SYSTEM MATCHES CYGWIN)
ENDIF(CMAKE_SYSTEM MATCHES CYGWIN OR CMAKE_SYSTEM MATCHES MINGW)


TEST_BIG_ENDIAN(CMAKE_WORDS_BIGENDIAN)

IF(CMAKE_WORDS_BIGENDIAN)
  ADD_DEFINE(WORDS_BIGENDIAN)
ENDIF(CMAKE_WORDS_BIGENDIAN)


OPTION(BUILD_STATIC "build static library too" ON)
OPTION(PROFILE "enable profile" OFF)
OPTION(OPTIMIZATION "enable optimization" OFF)
OPTION(DEBUG "enable debug" OFF)

# no mutex locking for sync. driver
OPTION(EMPTY_LOCKS "enable empty locks" OFF)

#sound backends
OPTION(ALSA "alsa backend" OFF)
OPTION(ALSA_DLOPEN "alsa dynamic loading" OFF)
OPTION(ARTS "arts backend" OFF)
OPTION(ARTS_DLOPEN "arts dynamic loading" OFF)
OPTION(ESD "esd backend" OFF)
OPTION(ESD_DLOPEN " esd dynamic loading" OFF)
OPTION(WAVEOUT "wave out backend" ON)
OPTION(NULLOUT "null out backend" ON)
OPTION(SDL "sdl backend" OFF)
OPTION(DSOUND "directsound backend" OFF)

#additional codecs
OPTION(VORBIS "vorbis support" OFF)
OPTION(SMPEG "smpeg support" OFF)

OPTION(CAPTURE "capture support" OFF)

#want test-suit?
OPTION(BUILD_TESTS "build the test-suite" OFF)

IF(OPTIMIZATION)
  SET(EMPTY_LOCKS OFF)
ENDIF(OPTIMIZATION)

IF(EMPTY_LOCKS)
  ADD_DEFINE(EMPTY_LOCKS)
ENDIF(EMPTY_LOCKS)

IF(DEBUG)
  ADD_DEFINE(DEBUG)
  OPTION(USE_WERROR      "enable fail on all warning"   OFF)
  OPTION(EFENCE          "enable efence support"        OFF)
  OPTION(DEBUG_MAXIMUS   "enable all debugging"         OFF)
  OPTION(DEBUG_STUB      "enable stub debugging"        OFF)
  OPTION(DEBUG_LOOP      "enable looping debugging"     OFF)
  OPTION(DEBUG_CONVERT   "enable convertion debugging"  OFF)
  OPTION(DEBUG_CONFIG    "enable config file debugging" OFF)
  OPTION(DEBUG_LOCK      "enable lock debugging"        OFF)
  OPTION(DEBUG_EXT       "enable extension debugging"   OFF)
  OPTION(DEBUG_BUFFER    "enable buffer debugging"      OFF)
  OPTION(DEBUG_SOURCE    "enable source debugging"      OFF)
  OPTION(DEBUG_MIXER     "enable mixer debugging"       OFF)
  OPTION(DEBUG_STREAMING "enable streaming debugging"   OFF)
  OPTION(DEBUG_MATH      "enable math debugging"        OFF)
  OPTION(DEBUG_MEM       "enable mem debugging"         OFF)
  OPTION(DEBUG_CONTEXT   "enable context debugging"     OFF)
  OPTION(DEBUG_QUEUE     "enable queue debugging"       OFF)
  OPTION(DEBUG_FILTER    "enable filter debugging"      OFF)

  IF(EFENCE)
    FIND_LIBRARY(EFENCE_LIB NAMES efence PATHS /usr/lib /usr/local/lib)
     IF(EFENCE_LIB MATCHES "NOTFOUND")
      MESSAGE("Electric Fence not installed - disabled.")
      SET(EFENCE OFF)
    ELSE(EFENCE_LIB MATCHES "NOTFOUND")
      SET(ADD_LIBS ${ADD_LIBS} ${EFENCE_LIB})
    ENDIF(EFENCE_LIB MATCHES "NOTFOUND")
  ENDIF(EFENCE)

  IF(DEBUG_MAXIMUS)
    ADD_DEFINE(DEBUG_MAXIMUS)
  ENDIF(DEBUG_MAXIMUS)

  IF(DEBUG_STUB)
    ADD_DEFINE(DEBUG_STUB)
  ENDIF(DEBUG_STUB)

  IF(DEBUG_LOOP)
    ADD_DEFINE(DEBUG_LOOP)
  ENDIF(DEBUG_LOOP)

  IF(DEBUG_CONVERT)
    ADD_DEFINE(DEBUG_CONVERT)
  ENDIF(DEBUG_CONVERT)

  IF(DEBUG_CONFIG)
    ADD_DEFINE(DEBUG_CONFIG)
  ENDIF(DEBUG_CONFIG)

  IF(DEBUG_LOCK)
    ADD_DEFINE(DEBUG_LOCK)
  ENDIF(DEBUG_LOCK)

  IF(DEBUG_EXT)
    ADD_DEFINE(DEBUG_EXT)
  ENDIF(DEBUG_EXT)

  IF(DEBUG_BUFFER)
    ADD_DEFINE(DEBUG_BUFFER)
  ENDIF(DEBUG_BUFFER)

  IF(DEBUG_SOURC)
    ADD_DEFINE(DEBUG_SOURC)
  ENDIF(DEBUG_SOURC)

  IF(DEBUG_MIXER)
    ADD_DEFINE(DEBUG_MIXER)
  ENDIF(DEBUG_MIXER)

  IF(DEBUG_STREAMING)
    ADD_DEFINE(DEBUG_STREAMING)
  ENDIF(DEBUG_STREAMING)

  IF(DEBUG_MATH)
    ADD_DEFINE(DEBUG_MATH)
  ENDIF(DEBUG_MATH)

  IF(DEBUG_MEM)
    ADD_DEFINE(DEBUG_MEM)
  ENDIF(DEBUG_MEM)

  IF(DEBUG_CONTEXT)
    ADD_DEFINE(DEBUG_CONTEXT)
  ENDIF(DEBUG_CONTEXT)

  IF(DEBUG_QUEUE)
    ADD_DEFINE(DEBUG_QUEUE)
  ENDIF(DEBUG_QUEUE)

  IF(DEBUG_FILTER)
    ADD_DEFINE(DEBUG_FILTER)
  ENDIF(DEBUG_FILTER)
ELSE(DEBUG)

  ADD_DEFINITIONS(-DNDEBUG)
ENDIF(DEBUG)


#library stuff
CHECK_LIBRARY_EXISTS("m" "sin" "" HAVE_LIB_M)

IF(NOT CMAKE_SYSTEM MATCHES "Darwin")
  IF(NOT HAVE_LIB_M)
    MESSAGE(FATAL_ERROR "libm not found.")
  ENDIF(NOT HAVE_LIB_M)
  CHECK_LIBRARY_EXISTS("dl" "dlopen" "" HAVE_LIB_DL)
ENDIF(NOT CMAKE_SYSTEM MATCHES "Darwin")

IF(HAVE_LIB_DL)
  SET(ADD_LIBS ${ADD_LIBS} dl)
ELSE(HAVE_LIB_DL)
  ADD_DEFINE(NODLOPEN)
ENDIF(HAVE_LIB_DL)

IF(HAVE_LIB_M)
  SET(ADD_LIBS ${ADD_LIBS} m)
ENDIF(HAVE_LIB_M)


# Special OS objs for target systems
IF(CMAKE_SYSTEM MATCHES "Linux")
  ADD_DEFINE("LINUX_TARGET 1")
  SET(OS_OBJS ${LINUX_OBJS})

  IF(CMAKE_SYSTEM_MACHINE MATCHES "mips")
    ADD_DEFINE(BROKEN_LIBIO)
  ENDIF(CMAKE_SYSTEM_MACHINE MATCHES "mips")
ELSE(CMAKE_SYSTEM MATCHES "Linux")

IF(CMAKE_SYSTEM MATCHES "BSD")
  ADD_DEFINE("BSD_TARGET 1")
  SET(OS_OBJS ${BSD_OBJS})
  SET(BROKEN_PTHREAD_FLAG ON)
ELSE(CMAKE_SYSTEM MATCHES "BSD")

IF(CMAKE_SYSTEM MATCHES "SunOS")
  ADD_DEFINE("SOLARIS_TARGET 1")
  SET(OS_OBJS ${SOLARIS_OBJS})
  SET(REDUNDANT_DECLS_WARN OFF)
ELSE(CMAKE_SYSTEM MATCHES "SunOS")

IF(CMAKE_SYSTEM MATCHES "IRIX")
  ADD_DEFINE("IRIX_TARGET 1")
  ADD_DEFINE(IRIS_SUPPORT)
  SET(OS_OBJS ${IRIX_OBJS})
  SET(REDUNDANT_DECLS_WARN OFF)
  SET(ADD_LIBS ${ADD_LIBS} audio)
ELSE(CMAKE_SYSTEM MATCHES "IRIX")

IF(CMAKE_SYSTEM MATCHES "Windows")
  ADD_DEFINE("WINDOWS_TARGET 1")
  ADD_DEFINE(AL_BUILD_LIBRARY)
  SET(OS_OBJS ${WINDOWS_OBJS})
  # need that mm library for waveOutOpen etc
  SET(ADD_LIBS ${ADD_LIBS} winmm)
  SET(SHADOW_WARN OFF)
  SET(REDUNDANT_DECLS_WARN OFF)

  CHECK_INCLUDE_FILE(windows.h WINTHREAD_AVAILABLE)
ELSE(CMAKE_SYSTEM MATCHES "Windows")

IF(CMAKE_SYSTEM MATCHES "Darwin")
  ADD_DEFINE("DARWIN_TARGET 1")
  SET(OS_OBJS ${DARWIN_OBJS})
  #FIXME
  SET(ADD_LDFLAGS "${ADD_LDFLAGS} -I/System/Library/Frameworks/CoreAudio.framework/Headers/ -framework CoreAudio")
  SET(BROKEN_PTHREAD_FLAG ON)
  SET(NO_PTHREAD_FLAG ON)
ELSE(CMAKE_SYSTEM MATCHES "Darwin")

IF(CMAKE_SYSTEM MATCHES "morphos")
  ADD_DEFINE("MORPHOS_TARGET 1")
  SET(OS_OBJS ${MORPHOS_OBJS})
  SET(REDUNDANT_DECLS_WARN OFF)
  CHECK_INCLUDE_FILE(dlfcn.h MOSTHREAD_AVAILABLE)
ELSE(CMAKE_SYSTEM MATCHES "morphos")

  MESSAGE("unknown System")
  SET(OS_OBJS ${LINUX_OBJS})
ENDIF(CMAKE_SYSTEM MATCHES "morphos")
ENDIF(CMAKE_SYSTEM MATCHES "Darwin")
ENDIF(CMAKE_SYSTEM MATCHES "Windows")
ENDIF(CMAKE_SYSTEM MATCHES "IRIX")
ENDIF(CMAKE_SYSTEM MATCHES "SunOS")
ENDIF(CMAKE_SYSTEM MATCHES "BSD")
ENDIF(CMAKE_SYSTEM MATCHES "Linux")

CHECK_INCLUDE_FILE(pthread.h PTHREAD_AVAILABLE)

#threadify
IF(PTHREAD_AVAILABLE)
  IF(NOT NO_PTHREAD_FLAG)
    IF(BROKEN_PTHREAD_FLAG)
      SET(ADD_LIBS "${ADD_LIBS} -pthread")
    ELSE(BROKEN_PTHREAD_FLAG)
      SET(ADD_LIBS "${ADD_LIBS} -lpthread")
    ENDIF(BROKEN_PTHREAD_FLAG)
  ENDIF(NOT NO_PTHREAD_FLAG)

  ADD_DEFINE(USE_POSIXMUTEX)
  ADD_DEFINE(USE_POSIXTHREADS)
ELSE(PTHREAD_AVAILABLE)

IF(WINTHREAD_AVAILABLE)
  ADD_DEFINE(USE_WINDOWSMUTEX)
  ADD_DEFINE(USE_WINDOWSTHREADS)
ELSE(WINTHREAD_AVAILABLE)

IF(MOSTHREAD_AVAILABLE)
  ADD_DEFINE(USE_MORPHOSMUTEX)
  ADD_DEFINE(USE_MORPHOSTHREADS)
ELSE(MOSTHREAD_AVAILABLE)

  ADD_DEFINE(NO_THREADS)
ENDIF(MOSTHREAD_AVAILABLE)
ENDIF(WINTHREAD_AVAILABLE)
ENDIF(PTHREAD_AVAILABLE)

#sound backends
IF(ALSA)
  CHECK_INCLUDE_FILE(alsa/asoundlib.h FOUND_ALSA)
  IF(FOUND_ALSA)
    SET(OS_OBJS ${OS_OBJS} ${ALSA_OBJS})
    SET(SHADOW_WARN OFF)
    SET(REDUNDANT_DECLS_WARN OFF)
    ADD_DEFINE(ALSA_SUPPORT)

    CHECK_INCLUDE_FILE(time.h HAVE_TIME_H)
    IF(HAVE_TIME_H)
      ADD_DEFINE("HAVE_TIME_H 1")
    ENDIF(HAVE_TIME_H)

    IF(ALSA_DLOPEN)
      ADD_DEFINE("OPENAL_DLOPEN_ALSA 1")
    ELSE(ALSA_DLOPEN)
      SET(ADD_LIBS ${ADD_LIBS} asound)
    ENDIF(ALSA_DLOPEN)
  ELSE(FOUND_ALSA)
    MESSAGE("Alsa not found - Alsa support disabled.")
    SET(ALSA OFF)
  ENDIF(FOUND_ALSA)
ENDIF(ALSA)

IF(ARTS)
  FIND_PROGRAM(ARTS_CONFIG NAMES arts-config)
  IF(NOT ${ARTS_CONFIG} MATCHES "NOTFOUND")
    EXEC_PROGRAM(${ARTS_CONFIG} ARGS "--cflags" OUTPUT_VARIABLE ARTS_CFLAGS)
    SET(ADD_CFLAGS "${ADD_CFLAGS} ${ARTS_CFLAGS}")
    SET(OS_OBJS ${OS_OBJS} ${ARTS_OBJS})
    ADD_DEFINE(ARTS_SUPPORT)

    IF(ARTS_DLOPEN)
      ADD_DEFINE("OPENAL_DLOPEN_ARTS 1")
    ELSE(ARTS_DLOPEN)
      EXEC_PROGRAM(${ARTS_CONFIG} ARGS "--libs" OUTPUT_VARIABLE ARTS_LIBS)
      SET(ADD_LIBS "${ADD_LIBS} ${ARTS_LIBS}")
    ENDIF(ARTS_DLOPEN)

  ELSE(NOT ${ARTS_CONFIG} MATCHES "NOTFOUND")
    MESSAGE("arts-config not found - aRts support disabled.")
    SET(ARTS OFF)
  ENDIF(NOT ${ARTS_CONFIG} MATCHES "NOTFOUND")
ENDIF(ARTS)

IF(ESD)
  FIND_PROGRAM(ESD_CONFIG NAMES esd-config)
  IF(NOT ${ESD_CONFIG} MATCHES "NOTFOUND")
    EXEC_PROGRAM(${ESD_CONFIG} ARGS "--cflags" OUTPUT_VARIABLE ESD_CFLAGS)
    SET(ADD_CFLAGS "${ADD_CFLAGS} ${ESD_CFLAGS}")
    SET(OS_OBJS ${OS_OBJS} ${ESD_OBJS})
    ADD_DEFINE(ESD_SUPPORT)

    # bad audiofile.h!  bad!
    SET(SHADOW_WARN OFF)

    IF(ESD_DLOPEN)
      ADD_DEFINE("OPENAL_DLOPEN_ESD 1")
    ELSE(ESD_DLOPEN)
      EXEC_PROGRAM(${ESD_CONFIG} ARGS "--libs" OUTPUT_VARIABLE ESD_LIBS)
      SET(ADD_LIBS "${ADD_LIBS} ${ESD_LIBS}")
    ENDIF(ESD_DLOPEN)

  ELSE(NOT ${ESD_CONFIG} MATCHES "NOTFOUND")
    MESSAGE("esd-config not found - esound support disabled.")
    SET(ESD OFF)
  ENDIF(NOT ${ESD_CONFIG} MATCHES "NOTFOUND")
ENDIF(ESD)

IF(WAVEOUT)
  SET(OS_OBJS ${OS_OBJS} ${WAVEOUT_OBJS})
  ADD_DEFINE(WAVEOUT_SUPPORT)
ENDIF(WAVEOUT)

IF(NULLOUT)
  SET(OS_OBJS ${OS_OBJS} ${NULL_OBJS})
  ADD_DEFINE(NULL_SUPPORT)
ENDIF(NULLOUT)

IF(SMPEG)
  SET(SDL ON)
ENDIF(SMPEG)

IF(SDL)
  IF(CYGWIN)
    # On cygwin targets, we don't rely on sdl-config.
    SET(SDL_CONFIG "1")
    SET(ADD_CFLAGS "${ADD_CFLAGS} -I/cygwin/usr/include/SDL")
    SET(OS_OBJS ${OS_OBJS} ${SDL_OBJS})
    ADD_DEFINE(SDL_SUPPORT)
  ELSE(CYGWIN)
  FIND_PROGRAM(SDL_CONFIG NAMES sdl-config)
  IF(NOT ${SDL_CONFIG} MATCHES "NOTFOUND")
    EXEC_PROGRAM(${SDL_CONFIG} ARGS "--cflags" OUTPUT_VARIABLE SDL_CFLAGS)
    EXEC_PROGRAM(${SDL_CONFIG} ARGS "--libs" OUTPUT_VARIABLE SDL_LIBS)
    SET(ADD_CFLAGS "${ADD_CFLAGS} ${SDL_CFLAGS} -I/usr/local/include/SDL -I/usr/local/include")
    SET(ADD_LIBS "${ADD_LIBS} ${SDL_LIBS}")
    SET(OS_OBJS ${OS_OBJS} ${SDL_OBJS})
    ADD_DEFINE(SDL_SUPPORT)
  ELSE(NOT ${SDL_CONFIG} MATCHES "NOTFOUND")
    MESSAGE("sdl-config not found - SDL support disabled.")
    SET(SDL OFF)
  ENDIF(NOT ${SDL_CONFIG} MATCHES "NOTFOUND")
  ENDIF(CYGWIN)
ENDIF(SDL)

IF(DSOUND)
  IF(CMAKE_SYSTEM MATCHES "Windows")
    CHECK_INCLUDE_FILE(dsound.h FOUND_DSOUND)
    IF(FOUND_DSOUND)
      SET(ADD_LIBS ${ADD_LIBS} dsound)
    ELSE(FOUND_DSOUND)
      MESSAGE("dsound.h not found - DirectSound support disabled.")
      SET(DSOUND OFF)
    ENDIF(FOUND_DSOUND)
  ENDIF(CMAKE_SYSTEM MATCHES "Windows")
ENDIF(DSOUND)

IF(VORBIS)
  CHECK_LIBRARY_EXISTS("vorbisfile" "ov_info" "" HAVE_VORBISFILE)
  IF(HAVE_VORBISFILE)
    SET(ADD_LIBS ${ADD_LIBS} vorbis vorbisfile)
  ELSE(HAVE_VORBISFILE)
    MESSAGE("Vorbisfile not found - Vorbis support disabled.")
    SET(VORBIS OFF)
  ENDIF(HAVE_VORBISFILE)
ENDIF(VORBIS)

IF(SMPEG)
  IF(SDL)
    FIND_PROGRAM(SMPEG_CONFIG NAMES smpeg-config)
    IF(NOT ${SMPEG_CONFIG} MATCHES "NOTFOUND")
      EXEC_PROGRAM(${SMPEG_CONFIG} ARGS "--cflags" OUTPUT_VARIABLE SMPEG_CFLAGS)
      EXEC_PROGRAM(${SMPEG_CONFIG} ARGS "--libs" OUTPUT_VARIABLE SMPEG_LIBS)
      SET(ADD_CFLAGS "${ADD_CFLAGS} ${SMPEG_CFLAGS}")
      SET(ADD_LIBS "${ADD_LIBS} ${SMPEG_LIBS}")
      ADD_DEFINE(SMPEG_SUPPORT)
    ELSE(NOT ${SMPEG_CONFIG} MATCHES "NOTFOUND")
      MESSAGE("smpeg-config not found - smpg support disabled.")
    ENDIF(NOT ${SMPEG_CONFIG} MATCHES "NOTFOUND")
  ELSE(SDL)
    MESSAGE("smpeg depends on SDL, but SDL wasn't found - smpg support disabled.")
    SET(SDL OFF)
  ENDIF(SDL)
ENDIF(SMPEG)


# enable capture support?
IF(CAPTURE)
  ADD_DEFINE(CAPTURE_SUPPORT)
ENDIF(CAPTURE)


# Special objs for architectures
IF(CMAKE_SYSTEM_MACHINE MATCHES "i.86" OR CMAKE_SYSTEM_MACHINE MATCHES "x86_64")
  INCLUDE_DIRECTORIES(${OpenAL_SOURCE_DIR}/src/arch/i386)
  SET(ARCH_OBJS ${x86_OBJS})

  MACRO_TEST(__MMX__ MMX_ENABLED)
  IF(MMX_ENABLED)
    SET(ARCH_OBJS ${ARCH_OBJS} ${x86_MMX_OBJS})
  ENDIF(MMX_ENABLED)

  IF(CMAKE_SYSTEM_MACHINE MATCHES "i.86")
    #nasm stuff to do!
    FIND_PROGRAM(HAVE_NASM NAMES nasm)

    IF(NOT HAVE_NASM MATCHES "NOTFOUND")
      PROJECT(OpenAL ASM)
      MESSAGE(STATUS "Found NASM")
      ADD_DEFINE(HAVE_CPU_CAPS_DETECTION)
      SET(ARCH_OBJS ${ARCH_OBJS} ${x86_NASM_OBJS})

      IF(MMX_ENABLED)
        SET(ARCH_OBJS ${ARCH_OBJS} ${x86_MMX_NASM_OBJS})
      ENDIF(MMX_ENABLED)
    ENDIF(NOT HAVE_NASM MATCHES "NOTFOUND")
  ENDIF(CMAKE_SYSTEM_MACHINE MATCHES "i.86")

  IF(CMAKE_SYSTEM_MACHINE MATCHES "i686" OR CMAKE_SYSTEM_MACHINE MATCHES "x86_64")
    OPTION(OPTIM_GENERIC "enable full generic mixing" OFF)
  ELSE(CMAKE_SYSTEM_MACHINE MATCHES "i686" OR CMAKE_SYSTEM_MACHINE MATCHES "x86_64")
    OPTION(OPTIM_GENERIC "enable full generic mixing" ON)
  ENDIF(CMAKE_SYSTEM_MACHINE MATCHES "i686" OR CMAKE_SYSTEM_MACHINE MATCHES "x86_64")

  IF(NOT OPTIM_GENERIC)
    ADD_DEFINE(USE_LIGHT_GEN_MIXING)
  ENDIF(NOT OPTIM_GENERIC)

ELSE(CMAKE_SYSTEM_MACHINE MATCHES "i.86" OR CMAKE_SYSTEM_MACHINE MATCHES "x86_64")
  SET(ARCH_OBJS ${PORTABLE_OBJS})
ENDIF(CMAKE_SYSTEM_MACHINE MATCHES "i.86" OR CMAKE_SYSTEM_MACHINE MATCHES "x86_64")


#compiler specific settings
IF(CMAKE_COMPILER_IS_GNUCC)
  # warns
  SET(WFLAGS "${WFLAGS} -Wall -W -Wbad-function-cast -Wcast-qual -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wimplicit-function-declaration -Waggregate-return -Winline -Wpointer-arith -fno-common")

  # Should we use turn warnings into errors?
  IF(USE_WERROR)
    SET(WFLAGS "${WFLAGS} -Werror -pedantic-errors")
  ENDIF(USE_WERROR)

  # if we are forced to live with redundant decls, turn off those errors
  IF(REDUNDANT_DECLS_WARN)
    SET(WFLAGS "${WFLAGS} -Wredundant-decls")
  ELSE(REDUNDANT_DECLS_WARN)
    SET(WFLAGS "${WFLAGS} -Wno-redundant-decls")
  ENDIF(REDUNDANT_DECLS_WARN)

  IF(SHADOW_WARN)
    SET(WFLAGS "${WFLAGS} -Wshadow")
  ENDIF(SHADOW_WARN)

  IF(CMAKE_SYSTEM MATCHES "Linux")
    SET(WFLAGS "${WFLAGS} -ansi")
  ENDIF(CMAKE_SYSTEM MATCHES "Linux")

  IF(PROFILE)
    SET(ADD_CFLAGS "${ADD_CFLAGS} -O2 -fexpensive-optimizations -ffast-math -funroll-loops -funroll-all-loops -g -pg")
    SET(ADD_LDFLAGS "-g -pg")
  ENDIF(PROFILE)

  IF(OPTIMIZATION)
    SET(ADD_CFLAGS "${ADD_CFLAGS} -O6 -fexpensive-optimizations -funroll-all-loops -funroll-loops -fomit-frame-pointer -finline-functions -ffast-math")

    IF(CMAKE_SYSTEM_MACHINE MATCHES "i.86")
      SET(ADD_CFLAGS "${ADD_CFLAGS} -march=${CMAKE_SYSTEM_MACHINE}")
    ENDIF(CMAKE_SYSTEM_MACHINE MATCHES "i.86")

    IF(CMAKE_SYSTEM_MACHINE MATCHES "mips")
      SET(ADD_CFLAGS "${ADD_CFLAGS} -march=mips")
    ENDIF(CMAKE_SYSTEM_MACHINE MATCHES "mips")
  ENDIF(OPTIMIZATION)

  IF(${CMAKE_C_FLAGS} MATCHES "^ $")
    IF(NOT ${ADD_CFLAGS} MATCHES "O")
      SET(ADD_CFLAGS "-g -O2 ${ADD_CFLAGS}")
      MESSAGE("No CFLAGS found; setting default ones.")
    ENDIF(NOT ${ADD_CFLAGS} MATCHES "O")
  ENDIF(${CMAKE_C_FLAGS} MATCHES "^ $")

ENDIF(CMAKE_COMPILER_IS_GNUCC)

ADD_DEFINE(LINUX_AL)

#Generate config.h only if values differ
IF(NOT CONFIG_H_VALS_OLD STREQUAL CONFIG_H_VALS OR NOT EXISTS ${CONFIG_H_FILE})
  MESSAGE(STATUS "Writing config.h")
  FILE(WRITE ${CONFIG_H_FILE} "${CONFIG_H_CONTENT}")
ENDIF(NOT CONFIG_H_VALS_OLD STREQUAL CONFIG_H_VALS OR NOT EXISTS ${CONFIG_H_FILE})


SET(CMAKE_C_FLAGS "${WFLAGS} ${ADD_CFLAGS} ${CMAKE_C_FLAGS}")
SET(CMAKE_CXX_FLAGS "${ADD_CFLAGS} ${CMAKE_CXX_FLAGS}")
SET(CMAKE_SHARED_LINKER_FLAGS "${ADD_LDFLAGS} ${CMAKE_SHARED_LINKER_FLAGS}")
SET(CMAKE_MODULE_LINKER_FLAGS "${ADD_LDFLAGS} ${CMAKE_MODULE_LINKER_FLAGS}")

SET(OPENAL_OBJS ${AL_OBJS}
                ${INTERFACE_OBJS}
                ${OS_OBJS}
                ${ARCH_OBJS}
                ${ALC_OBJS}
                ${AC_OBJS}
                ${EXT_OBJS})

IF(BUILD_STATIC)
  #we can't create a static library with the same name
  #as the shared one, so we copy it over after creation
  ADD_LIBRARY(openal-static STATIC ${OPENAL_OBJS})
  SET_TARGET_PROPERTIES(openal-static PROPERTIES OUTPUT_NAME openal)
  SET_TARGET_PROPERTIES(openal-static PROPERTIES LINKER_LANGUAGE C)
  TARGET_LINK_LIBRARIES(openal-static ${ADD_LIBS})
  ADD_CUSTOM_COMMAND(
    TARGET openal-static
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy
      ${CMAKE_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}openal-static${CMAKE_STATIC_LIBRARY_SUFFIX}
      ${CMAKE_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}openal${CMAKE_STATIC_LIBRARY_SUFFIX})
  INSTALL_FILES(/lib FILES ${CMAKE_STATIC_LIBRARY_PREFIX}openal${CMAKE_STATIC_LIBRARY_SUFFIX})
ENDIF(BUILD_STATIC)

ADD_LIBRARY(openal SHARED ${OPENAL_OBJS})
SET_TARGET_PROPERTIES(openal PROPERTIES LINKER_LANGUAGE C)
SET_TARGET_PROPERTIES(openal PROPERTIES VERSION ${PACKAGE_VERSION}
                                        SOVERSION ${PACKAGE_MAJOR_VERSION})
TARGET_LINK_LIBRARIES(openal ${ADD_LIBS})

#test-suite
IF(BUILD_TESTS)
  # need CXX for testcpp only
  PROJECT(OpenAL CXX)
  FIND_LIBRARY(ALUT_LIB NAMES alut PATHS /usr/lib /usr/local/lib)
  IF(ALUT_LIB MATCHES "NOTFOUND")
    MESSAGE("alut not installed, cannot build tests")
  ELSE(ALUT_LIB MATCHES "NOTFOUND")
    ADD_LIBRARY(testlib test/testlib.c)
    TARGET_LINK_LIBRARIES(testlib ${ADD_LIBS} openal ${ALUT_LIB})

    SET(TESTS reverb
              da
              multi
              alut
              time
              multialut
              alinitexit
              single
              nothread
              pitch
              pitch2
              pitch3
              pitch4
              doppler
              sourcestop
              ext
              stream
              callback
              orient
              relative
              channel
              channel2
              pause
              pitchnothread
              pitchnothread2
              gain
              loop
              listener
              listener2
              listener3
              listener4
              bufferuniq
              sourceuniq
              gendelbuffer_error
              gendelsource_error
              multichannel
              mp3
              mp32
              vorbis
              gainlinear
              multimp3
              queue
              queue2
              queue3
              capture
              capture2
              leak
              multicontext
              strings
              attributes)

    FOREACH(TEST ${TESTS})
      SET(TEST_SRC test/test${TEST}.c)
      #gets added at the beginning thus having no effect
      SET_SOURCE_FILES_PROPERTIES(${TEST_SRC} PROPERTIES COMPILE_FLAGS "-Wno-unused")
      ADD_EXECUTABLE(test${TEST} ${TEST_SRC})
      TARGET_LINK_LIBRARIES(test${TEST} testlib ${ADD_LIBS} openal ${ALUT_LIB})
    ENDFOREACH(TEST)

    SET_SOURCE_FILES_PROPERTIES(test/testcpp.cc PROPERTIES COMPILE_FLAGS "-Wno-unused")
    ADD_EXECUTABLE(testcpp test/testcpp.cc)
    TARGET_LINK_LIBRARIES(testcpp testlib ${ADD_LIBS} openal ${ALUT_LIB})

    #copy over testdata, so test-suite can be used in binary dir
    SET(TESTDATA boom.mp3
                 boom.ogg
                 boom.wav
                 fire2.wav
                 multiboom.wav
                 multisample.wav
                 sample.wav)

    FOREACH(TESTDATUM ${TESTDATA})
      CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/test/${TESTDATUM} ${CMAKE_BINARY_DIR}/${TESTDATUM} COPYONLY)
    ENDFOREACH(TESTDATUM)
  ENDIF(ALUT_LIB MATCHES "NOTFOUND")
ENDIF(BUILD_TESTS)


#needed for openal.pc.in and openal-config.in
SET(prefix ${CMAKE_INSTALL_PREFIX})
SET(exec_prefix "\${prefix}")
SET(libdir "\${exec_prefix}/lib")
SET(includedir "\${prefix}/include")
SET(requirements "")

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/admin/openal.pc.in ${CMAKE_BINARY_DIR}/admin/openal.pc @ONLY)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/admin/openal-config.in ${CMAKE_BINARY_DIR}/admin/openal-config @ONLY)

#info file
IF(UNIX)
  FIND_PROGRAM(MAKEINFO_PRG NAMES makeinfo)
  IF(NOT MAKEINFO_PRG MATCHES "NOTFOUND")
    IF(NOT EXISTS ${CMAKE_BINARY_DIR}/doc)
      MAKE_DIRECTORY(${CMAKE_BINARY_DIR}/doc)
    ENDIF(NOT EXISTS ${CMAKE_BINARY_DIR}/doc)

    ADD_CUSTOM_COMMAND(
      OUTPUT    ${CMAKE_BINARY_DIR}/doc/openal.info
      DEPENDS   ${CMAKE_SOURCE_DIR}/doc/openal.texi
      COMMAND   cd ${CMAKE_BINARY_DIR}/doc && ${MAKEINFO_PRG}
      ARGS      ${CMAKE_SOURCE_DIR}/doc/openal.texi)

    ADD_CUSTOM_TARGET(InfoFile ALL echo DEPENDS ${CMAKE_BINARY_DIR}/doc/openal.info)
    INSTALL_FILES(/share/info FILES doc/openal.info)
  ENDIF(NOT MAKEINFO_PRG MATCHES "NOTFOUND")
ENDIF(UNIX)

INSTALL_TARGETS(/lib openal)
INSTALL_FILES(/include/AL FILES ${CMAKE_SOURCE_DIR}/../include/AL/al.h
                                ${CMAKE_SOURCE_DIR}/../include/AL/alc.h
                                include/AL/alext.h)
INSTALL_PROGRAMS(/bin FILES admin/openal-config)
INSTALL_FILES(/lib/pkgconfig FILES admin/openal.pc)
