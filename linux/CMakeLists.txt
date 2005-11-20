# cmake project file by Prakash Punnoor
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules")

PROJECT(OpenAL ASM C)
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
SET(SHOULD_ENFORCE_ANSI OFF)       # whether we should use -ansi
SET(SHADOW_WARN ON)                # warn about shadow variables
SET(BROKEN_PTHREAD_FLAG OFF)       # BSD uses -pthread, not -lpthread
SET(NO_PTHREAD_FLAG OFF)
SET(SHOULD_USE_FPIC ON)            # use -fPIC to compile position independent objs
SET(SHOULD_USE_WERROR OFF)         # turn warnings into errors
SET(SHOULD_USE_RDYNAMIC ON)        # doesn't work on irix
SET(ENABLE_EMPTY_LOCKS OFF)

SET(ADD_CFLAGS "")
SET(ADD_LDFLAGS "")
SET(WFLAGS "")

IF(CMAKE_UNAME)
  EXEC_PROGRAM(uname ARGS -m OUTPUT_VARIABLE CMAKE_SYSTEM_MACHINE)
ELSE(CMAKE_UNAME)
MESSAGE("Could not detect machine type")
ENDIF(CMAKE_UNAME)

IF (CMAKE_SYSTEM MATCHES CYGWIN OR CMAKE_SYSTEM MATCHES MINGW)
  SET(CMAKE_SYSTEM Windows)
ENDIF (CMAKE_SYSTEM MATCHES CYGWIN OR CMAKE_SYSTEM MATCHES MINGW)


TEST_BIG_ENDIAN(CMAKE_WORDS_BIGENDIAN)

IF(CMAKE_WORDS_BIGENDIAN)
  ADD_DEFINE(WORDS_BIGENDIAN)
ENDIF(CMAKE_WORDS_BIGENDIAN)

# werror stuff
OPTION(SHOULD_USE_WERROR "enable fail on all warning" OFF)

# profile stuff
OPTION(ENABLE_PROFILE "enable profile" OFF)

# optimization stuff
OPTION(ENABLE_OPTIMIZATION "enable optimization" OFF)

# no mutex locking for sync. driver
OPTION(ENABLE_EMPTY_LOCKS "enable empty locks" OFF)

# debug stuff
OPTION(ENABLE_DEBUG "enable debug" OFF)

OPTION(ENABLE_ALSA "alsa backend" OFF)
OPTION(ENABLE_ALSA_DLOPEN "alsa dynamic loading" OFF)
OPTION(ENABLE_ARTS "arts backend" OFF)
OPTION(ENABLE_ESD "esd backend" OFF)
OPTION(ENABLE_ESD_DLOPEN " esd dynamic loading" OFF)
OPTION(ENABLE_WAVEOUT "wave out backend" ON)
OPTION(ENABLE_NULLOUT "null out backend" ON)
OPTION(ENABLE_SDL "sdl backend" OFF)
OPTION(ENABLE_DSOUND "directsound backend" OFF)
OPTION(ENABLE_VORBIS "vorbis support" OFF)
OPTION(ENABLE_SMPEG "smpeg support" OFF)
OPTION(ENABLE_CAPTURE "capture support" OFF)


IF(ENABLE_OPTIMIZATION)
  SET(ENABLE_EMPTY_LOCKS OFF)
ENDIF(ENABLE_OPTIMIZATION)


IF(ENABLE_EMPTY_LOCKS)
  ADD_DEFINE(EMPTY_LOCKS)
ENDIF(ENABLE_EMPTY_LOCKS)

IF(ENABLE_DEBUG)
  ADD_DEFINE(DEBUG)
ENDIF(ENABLE_DEBUG)


#library stuff
CHECK_LIBRARY_EXISTS("m" "sin" "" HAVE_LIB_M)

IF(NOT CMAKE_SYSTEM MATCHES "Darwin")
  IF(NOT HAVE_LIB_M)
    MESSAGE(FATAL_ERROR "libm not found.")
  ENDIF(NOT HAVE_LIB_M)
  CHECK_LIBRARY_EXISTS("dl" "dlopen" "" HAVE_LIB_DL)
ENDIF(NOT CMAKE_SYSTEM MATCHES "Darwin")

IF(HAVE_LIB_DL)
  SET(ADD_LIBS "${ADD_LIBS} -ldl")
ELSE(HAVE_LIB_DL)
  ADD_DEFINE(NODLOPEN)
ENDIF(HAVE_LIB_DL)

IF(HAVE_LIB_M)
  SET(ADD_LIBS "${ADD_LIBS} -lm")
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
  SET(SHOULD_USE_RDYNAMIC OFF)
  SET(REDUNDANT_DECLS_WARN OFF)
  SET(ADD_LIBS "${ADD_LIBS} -laudio")
ELSE(CMAKE_SYSTEM MATCHES "IRIX")

IF(CMAKE_SYSTEM MATCHES "Windows")
  ADD_DEFINE("WINDOWS_TARGET 1")
  ADD_DEFINE(AL_BUILD_LIBRARY)
  SET(OS_OBJS ${WINDOWS_OBJS})
  # need that mm library for waveOutOpen etc
  SET(ADD_LIBS "${ADD_LIBS} -lwinmm")
  # all code is position independant, causes error
  SET(SHOULD_USE_FPIC OFF)
  SET(SHADOW_WARN OFF)
  SET(REDUNDANT_DECLS_WARN OFF)

  CHECK_INCLUDE_FILE(windows.h WINTHREAD_AVAILABLE)
ELSE(CMAKE_SYSTEM MATCHES "Windows")

IF(CMAKE_SYSTEM MATCHES "Darwin")
  ADD_DEFINE("DARWIN_TARGET 1")
  SET(OS_OBJS ${DARWIN_OBJS})
  SET(ADD_LIBS "${ADD_LIBS} -I/System/Library/Frameworks/CoreAudio.framework/Headers/ -framework CoreAudio")
  SET(BROKEN_PTHREAD_FLAG ON)
  SET(NO_PTHREAD_FLAG ON)
  SET(SHOULD_USE_WERROR OFF)
ELSE(CMAKE_SYSTEM MATCHES "Darwin")

IF(CMAKE_SYSTEM MATCHES "morphos")
  ADD_DEFINE("MORPHOS_TARGET 1")
  SET(OS_OBJS ${MORPHOS_OBJS})
  SET(REDUNDANT_DECLS_WARN OFF)
  SET(SHOULD_USE_FPIC OFF)

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
IF(ENABLE_ALSA)
  CHECK_INCLUDE_FILE(alsa/asoundlib.h FOUND_ALSA)
  IF(FOUND_ALSA)
    SET(OS_OBJS ${OS_OBJS} ${ALSA_OBJS})
    SET(SHADOW_WARN OFF)
    SET(REDUNDANT_DECLS_WARN OFF)

    CHECK_INCLUDE_FILE(time.h HAVE_TIME_H)
    IF(HAVE_TIME_H)
      ADD_DEFINE(HAVE_TIME_H)
    ENDIF(HAVE_TIME_H)

    IF(ENABLE_ALSA_DLOPEN)
      ADD_DEFINE(OPENAL_DLOPEN_ALSA)
    ELSE(ENABLE_ALSA_DLOPEN)
      SET(ADD_LIBS "${ADD_LIBS} -lasound")
    ENDIF(ENABLE_ALSA_DLOPEN)
  ENDIF(FOUND_ALSA)
ENDIF(ENABLE_ALSA)

IF(ENABLE_WAVEOUT)
  SET(OS_OBJS ${OS_OBJS} ${WAVEOUT_OBJS})
  ADD_DEFINE(WAVEOUT_SUPPORT)
ENDIF(ENABLE_WAVEOUT)

IF(ENABLE_NULLOUT)
  SET(OS_OBJS ${OS_OBJS} ${NULL_OBJS})
  ADD_DEFINE(NULL_SUPPORT)
ENDIF(ENABLE_NULLOUT)


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
      MESSAGE(STATUS "Found NASM")
      ADD_DEFINE(HAVE_CPU_CAPS_DETECTION)
      SET(ARCH_OBJS ${ARCH_OBJS} ${x86_NASM_OBJS})

      IF(MMX_ENABLED)
        SET(ARCH_OBJS ${ARCH_OBJS} ${x86_MMX_NASM_OBJS})
      ENDIF(MMX_ENABLED)
    ENDIF(NOT HAVE_NASM MATCHES "NOTFOUND")
  ENDIF(CMAKE_SYSTEM_MACHINE MATCHES "i.86")

  IF(CMAKE_SYSTEM_MACHINE MATCHES "i686" OR CMAKE_SYSTEM_MACHINE MATCHES "x86_64")
    OPTION(ENABLE_OPTIM_GENERIC "enable full generic mixing" OFF)
  ELSE(CMAKE_SYSTEM_MACHINE MATCHES "i686" OR CMAKE_SYSTEM_MACHINE MATCHES "x86_64")
    OPTION(ENABLE_OPTIM_GENERIC "enable full generic mixing" ON)
  ENDIF(CMAKE_SYSTEM_MACHINE MATCHES "i686" OR CMAKE_SYSTEM_MACHINE MATCHES "x86_64")

  IF(NOT ENABLE_OPTIM_GENERIC)
    ADD_DEFINE(USE_LIGHT_GEN_MIXING)
  ENDIF(NOT ENABLE_OPTIM_GENERIC)

ELSE(CMAKE_SYSTEM_MACHINE MATCHES "i.86" OR CMAKE_SYSTEM_MACHINE MATCHES "x86_64")
  SET(ARCH_OBJS ${PORTABLE_OBJS})
ENDIF(CMAKE_SYSTEM_MACHINE MATCHES "i.86" OR CMAKE_SYSTEM_MACHINE MATCHES "x86_64")



IF(CMAKE_COMPILER_IS_GNUCC)
  # warns
  SET(WFLAGS "${WFLAGS} -Wall -W -Wbad-function-cast -Wcast-qual -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wimplicit-function-declaration -Waggregate-return -Winline -Wpointer-arith -fno-common")

  # Should we use turn warnings into errors?
  IF(SHOULD_USE_WERROR)
    SET(WFLAGS "${WFLAGS} -Werror -pedantic-errors")
  ENDIF(SHOULD_USE_WERROR)

  # if we are forced to live with redundant decls, turn off those errors
  IF(REDUNDANT_DECLS_WARN)
    SET(WFLAGS "${WFLAGS} -Wredundant-decls")
  ELSE(REDUNDANT_DECLS_WARN)
    SET(WFLAGS "${WFLAGS} -Wno-redundant-decls")
  ENDIF(REDUNDANT_DECLS_WARN)

  IF(SHADOW_WARN)
    SET(WFLAGS "${WFLAGS} -Wshadow")
  ENDIF(SHADOW_WARN)

  IF(SHOULD_USE_FPIC)
    ADD_DEFINE(PIC)
    SET(ADD_CFLAGS "${ADD_CFLAGS} -fPIC")
  ENDIF(SHOULD_USE_FPIC)

  IF(SHOULD_USE_RDYNAMIC)
    ADD_DEFINE(RDYNAMIC)
    SET(ADD_CFLAGS "${ADD_CFLAGS} -rdynamic")
  ENDIF(SHOULD_USE_RDYNAMIC)

  IF(CMAKE_SYSTEM MATCHES "Linux")
    SET(WFLAGS "${WFLAGS} -ansi")
  ENDIF(CMAKE_SYSTEM MATCHES "Linux")

  IF(ENABLE_PROFILE)
    SET(ADD_CFLAGS "${ADD_CFLAGS} -O2 -fexpensive-optimizations -ffast-math -funroll-loops -funroll-all-loops -g -pg")
    SET(ADD_LDFLAGS "-g -pg")
  ENDIF(ENABLE_PROFILE)

  IF(ENABLE_OPTIMIZATION)
    SET(ADD_CFLAGS "${ADD_CFLAGS} -O6 -fexpensive-optimizations -funroll-all-loops -funroll-loops -fomit-frame-pointer -finline-functions -ffast-math")

    IF(CMAKE_SYSTEM_MACHINE MATCHES "i.86")
      SET(ADD_CFLAGS "${ADD_CFLAGS} -march=${CMAKE_SYSTEM_MACHINE}")
    ENDIF(CMAKE_SYSTEM_MACHINE MATCHES "i.86")

    IF(CMAKE_SYSTEM_MACHINE MATCHES "mips")
      SET(ADD_CFLAGS "${ADD_CFLAGS} -march=mips")
    ENDIF(CMAKE_SYSTEM_MACHINE MATCHES "mips")
  ENDIF(ENABLE_OPTIMIZATION)

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
SET(CMAKE_SHARED_LINKER_FLAGS "${ADD_LDFLAGS} ${CMAKE_SHARED_LINKER_FLAGS}")
SET(CMAKE_MODULE_LINKER_FLAGS "${ADD_LDFLAGS} ${CMAKE_MODULE_LINKER_FLAGS}")

SET(OPENAL_OBJS ${AL_OBJS}
                ${INTERFACE_OBJS}
                ${OS_OBJS}
                ${ARCH_OBJS}
                ${ALC_OBJS}
                ${AC_OBJS}
                ${EXT_OBJS})

#ADD_LIBRARY(openal-static STATIC ${OPENAL_OBJS})
#SET_TARGET_PROPERTIES(openal-static PROPERTIES OUTPUT_NAME openal)
#SET_TARGET_PROPERTIES(openal-static PROPERTIES LINKER_LANGUAGE C)
#TARGET_LINK_LIBRARIES(openal-static ${ADD_LIBS})

ADD_LIBRARY(openal SHARED ${OPENAL_OBJS})
SET_TARGET_PROPERTIES(openal PROPERTIES LINKER_LANGUAGE C)
SET_TARGET_PROPERTIES(openal PROPERTIES VERSION ${PACKAGE_VERSION}
                                        SOVERSION ${PACKAGE_MAJOR_VERSION})
TARGET_LINK_LIBRARIES(openal ${ADD_LIBS})

#needed fo openal.pc.in and openal-config.in
SET(prefix ${CMAKE_INSTALL_PREFIX})
SET(exec_prefix "\${prefix}")
SET(libdir "\${exec_prefix}/lib")
SET(includedir "\${prefix}/include")
SET(requirements "")

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/openal.pc.in ${CMAKE_BINARY_DIR}/openal.pc @ONLY)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/openal-config.in ${CMAKE_BINARY_DIR}/openal-config @ONLY)

#info file
IF(UNIX)
  FIND_PROGRAM(MAKEINFO NAMES makeinfo)
  IF(NOT MAKEINFO MATCHES "NOTFOUND")
    IF(NOT EXISTS ${CMAKE_BINARY_DIR}/doc)
      MAKE_DIRECTORY(${CMAKE_BINARY_DIR}/doc)
    ENDIF(NOT EXISTS ${CMAKE_BINARY_DIR}/doc)

    ADD_CUSTOM_COMMAND(
      OUTPUT    ${CMAKE_BINARY_DIR}/doc/openal.info
      DEPENDS   ${CMAKE_SOURCE_DIR}/doc/openal.texi
      COMMAND   cd ${CMAKE_BINARY_DIR}/doc && ${MAKEINFO}
      ARGS      ${CMAKE_SOURCE_DIR}/doc/openal.texi)

    ADD_CUSTOM_TARGET(InfoFile ALL echo DEPENDS ${CMAKE_BINARY_DIR}/doc/openal.info)
    INSTALL_FILES(/share/info FILES doc/openal.info)
  ENDIF(NOT MAKEINFO MATCHES "NOTFOUND")
ENDIF(UNIX)

INSTALL_TARGETS(/lib openal)
INSTALL_FILES(/include/AL FILES ${CMAKE_SOURCE_DIR}/../include/AL/al.h
                                ${CMAKE_SOURCE_DIR}/../include/AL/alc.h
                                include/AL/alext.h)
INSTALL_PROGRAMS(/bin FILES openal-config)
INSTALL_FILES(/lib/pkgconfig FILES openal.pc)
