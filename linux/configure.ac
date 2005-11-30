################################################################################
# Process this file with autoconf to produce a configure script.
################################################################################

AC_INIT([OpenAL library], [0.0.8], [openal-devel@opensource.creative.com], [openal])
AC_CONFIG_AUX_DIR([admin/autotools])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE
AC_PREREQ([2.56])
AC_CONFIG_SRCDIR([AUTHORS])
AC_CONFIG_HEADERS([config.h])

# Compatibility hack for older autoconf versions
m4_ifdef([AS_HELP_STRING], [], [AC_DEFUN([AS_HELP_STRING], [AC_HELP_STRING($][@)])])

################################################################################
# Checks for programs.
################################################################################

# We need ISO C99 features for e.g. snprintf.
FEATURECFLAGS="-D_ISOC99_SOURCE"

AC_PROG_CC
AC_C_CONST
AC_EXEEXT
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_SUBST([LIBTOOL_DEPS])
AC_PROG_CXX

BROKEN_PTHREAD_FLAG=no         # BSD use -pthread, not -lpthread
NO_PTHREAD_FLAG=no

################################################################################
# Checks for library functions.
################################################################################

AC_CHECK_LIBM
AC_SUBST([LIBM])

################################################################################
# handling of warning-related options
################################################################################

AC_ARG_ENABLE([warnings],
[AS_HELP_STRING([--enable-warnings],
                [enable pedantic compiler warnings @<:@default=yes@:>@])])

if test "x$enable_warnings" = xno; then
  WARNINGCFLAGS=""
else
  AX_CFLAGS_WARN_ALL_ANSI([WARNINGCFLAGS])
fi

AC_ARG_ENABLE([gcc-warnings],
[AS_HELP_STRING([--enable-gcc-warnings],
                [enable additional GCC-specific warnings @<:@default=no@:>@])])

if test "x$enable_gcc_warnings" = xyes; then
  if test "x$enable_warnings" = xno; then
    AC_MSG_WARN([--enable-gcc-warnings ignored because of --disable-warnings])
  elif test "x$GCC" != xyes; then
    AC_MSG_WARN([--enable-gcc-warnings ignored because no GCC was detected])
  else
    # There are some warnings which are note enabled (yet):
    #    -Wpadded: Perhaps good for optimizing out data layout, but not in general
    #    -Wfloat-equal: Often we should use an epsilon, but there are some correct uses, too
    WARNINGCFLAGS="$WARNINGCFLAGS -W -Waggregate-return -Wbad-function-cast -Wcast-align -Wcast-qual -Wconversion -Wdisabled-optimization -Wendif-labels -Winline -Wlong-long -Wmissing-declarations -Wmissing-noreturn -Wmissing-prototypes -Wnested-externs -Wpacked -Wpointer-arith -Wredundant-decls -Wshadow -Wsign-compare -Wstrict-prototypes -Wwrite-strings"
  fi
fi

AC_ARG_ENABLE([werror],
[AS_HELP_STRING([--enable-werror],
                [enable failure on all warnings @<:@default=no@:>@])])

if test "x$enable_werror" = xyes; then
  if test "x$enable_warnings" = xno; then
    AC_MSG_WARN([--enable-werror ignored because of --disable-warnings])
  elif test "x$GCC" != xyes; then
    AC_MSG_WARN([--enable-werror ignored because no GCC was detected])
  else
    WARNINGCFLAGS="$WARNINGCFLAGS -Werror"
  fi
fi

################################################################################
# handling of optimization-related options
################################################################################

AC_ARG_ENABLE([optimization],
[AS_HELP_STRING([--enable-optimization],
                [enable optimization @<:@default=no@:>@])])

if test "x$enable_optimization" = xyes; then
  enable_empty_locks=no
  OPTIMIZATIONCFLAGS="-fexpensive-optimizations -funroll-all-loops -funroll-loops -finline-functions -ffast-math"

  # -pg and -fomit-frame-pointer are incompatible
  if test "x$enable_profile" != xyes; then
    OPTIMIZATIONCFLAGS="$OPTIMIZATIONCFLAGS -fomit-frame-pointer"
  fi

  # add architecture-specific optimization flags
  case "$target" in
    *i386*) OPTIMIZATIONCFLAGS="-march=i386 $OPTIMIZATIONCFLAGS" ;;
    *i486*) OPTIMIZATIONCFLAGS="-march=i486 $OPTIMIZATIONCFLAGS" ;;
    *i586*) OPTIMIZATIONCFLAGS="-march=i586 $OPTIMIZATIONCFLAGS" ;;
    *i686*) OPTIMIZATIONCFLAGS="-march=i686 $OPTIMIZATIONCFLAGS" ;;
    mips*)  OPTIMIZATIONCFLAGS="-march=mips $OPTIMIZATIONCFLAGS" ;;
  esac
fi

# profiling stuff
AC_ARG_ENABLE([profile],
[AS_HELP_STRING([--enable-profile],
                [enable profile @<:@default=no@:>@])])

if test "x$enable_profile" = xyes; then
  OPTIMIZATIONCFLAGS="$OPTIMIZATIONCFLAGS -pg"
  PROFILINGLDFLAGS="$LDFLAGS -pg"
fi

# debugging stuff
AC_ARG_ENABLE([debug],
[AS_HELP_STRING([--enable-debug],
                [enable debug code and assertions @<:@default=no@:>@])])

if test "x$enable_debug" = xyes; then
  AC_DEFINE([DEBUG], [1], [Define to 1 if you want to include debugging code.])
else
  AC_DEFINE([NDEBUG], [1], [Define to 1 if you want to disable the assert() macro.])
fi

# no mutex locking for sync. driver
AC_ARG_ENABLE([empty-locks],
[AS_HELP_STRING([--enable-empty-locks],
                [enable empty locks @<:@default=no@:>@])])

if test "x$enable_empty_locks" = xyes; then
  AC_DEFINE([EMPTY_LOCKS], [1], [undocumented])
fi

AC_C_BIGENDIAN

dnl library stuff
case "$target" in
    *darwin*)
    RANLIB=ranlib
    ;;
    *)
    RANLIB=echo
    dnl Should be allow dlopen of extensions?
    AC_CHECK_LIB([dl], [dlopen], [LIBS="$LIBS -ldl"], [AC_DEFINE([NODLOPEN], [1], [undocumented]) echo "Can't dlopen extensions."])
    ;;
esac

dnl Special OS objs for target systems
case "$target" in
    *linux*)
    dnl subtest for mips/linux systems with broken libio headers
    case "$target" in
    	*mips* )
	AC_DEFINE([BROKEN_LIBIO], [1], [undocumented])
        ;;
	*)
        ;;
    esac
    AC_DEFINE([LINUX_TARGET], [1], [undocumented])
    OS_OBJS="\$(LIN_OBJS)"
    ;;
    *bsd*)
    BROKEN_PTHREAD_FLAG=yes
    AC_DEFINE([BSD_TARGET], [1], [undocumented])
    OS_OBJS="\$(BSD_OBJS)"
    ;;
    *solaris*)
    AC_DEFINE([SOLARIS_TARGET], [1], [undocumented])
    OS_OBJS="\$(SOLARIS_OBJS)"
    ;;
    *irix*)
    AC_DEFINE([IRIX_TARGET], [1], [undocumented])
    OS_OBJS="\$(IRIS_OBJS)"
    ;;
    *cygwin*|*mingw*)
    AC_DEFINE([WINDOWS_TARGET], [1], [undocumented])
    FEATURECFLAGS="-DAL_BUILD_LIBRARY $FEATURECFLAGS"
    OS_OBJS="\$(WINDOWS_OBJS)"
    LIBS="$LIBS -lwinmm"         # need that mm library for waveOutOpen etc
    ;;
    *darwin*)
    # I think darwin can use dlopen with fink, but
    # I don't want to require fink
    AC_DEFINE([NODLOPEN], [1], [undocumented])
    BROKEN_PTHREAD_FLAG=yes
    NO_PTHREAD_FLAG=yes
    LIBS="$LIBS -I/System/Library/Frameworks/CoreAudio.framework/Headers/ -framework CoreAudio"
    AC_DEFINE([DARWIN_TARGET], [1], [undocumented])
    OS_OBJS="\$(DARWIN_OBJS)" ;;
    *morphos*)
    AC_DEFINE([MORPHOS_TARGET], [1], [undocumented])
    OS_OBJS="\$(MORPHOS_OBJS)"
    ;;
    *)
    OS_OBJS="\$(LIN_OBJS)"
    ;;
esac

dnl Special objs for architectures
case "$target" in
    *i386* | *i486* | *i586* | *i686* | *x86_64*)
    ARCH_INCLUDES="$ARCH_INCLUDES -Iarch/i386"
    ARCH_OBJS="\$(x86_OBJS)"

    AC_MSG_CHECKING([for MMX support])    
    AC_PREPROC_IFELSE(
        [AC_LANG_PROGRAM([[
#ifndef __MMX__
#error no MMX support
#endif]], [])],
        [have_mmx=yes], [have_mmx=no])
    AC_MSG_RESULT([$have_mmx])
    if test "x$have_mmx" = xyes; then
        ARCH_OBJS="$ARCH_OBJS \$(x86_MMX_OBJS)"
    fi
    ;;
esac

case "$target" in
    *i386* | *i486* | *i586* | *i686*)
        AC_PATH_PROG([NASM], [nasm], [no])
        if test "x$NASM" != xno; then
            AC_DEFINE([HAVE_CPU_CAPS_DETECTION], [1], [detect cpu capabilities])
            ARCH_OBJS="$ARCH_OBJS \$(x86_NASM_OBJS)"

            if test "x$have_mmx" = xyes; then
                AC_DEFINE([HAVE_MMX_MEMCPY], [1], [mmx memcpy])
                ARCH_OBJS="$ARCH_OBJS \$(x86_MMX_NASM_OBJS)"
            fi

            case "$target" in
            *linux*)
                NASM_FORMAT="-f elf"
                ;;
            *cygwin* | *mingw*)
                NASM_FORMAT="-f win32 -DWIN32"
                ;;
            esac
        fi
        ;;
    *x86_64*)
        ;;
    *)
        ARCH_OBJS="\$(PORTABLE_OBJS)"
        ;;
esac


AC_ARG_ENABLE([optim-generic],
[AS_HELP_STRING([--enable-optim-generic],
                [x86(_64): even if mmx enabled, build optimized instead of light generic mixing routines  @<:@i686/x86_64:default=no@:>@])])

if test "x$have_mmx" = xyes; then
    case "$target" in
    *i686* | *x86_64*)
        if test "x$enable_optim_generic" != xyes; then
            enable_optim_generic=no
        fi
        ;;
    *)
        if test "x$enable_optim_generic" != xno; then
            enable_optim_generic=yes
        fi
        ;;
    esac

    if test "x$enable_optim_generic" = xno; then
        AC_DEFINE([USE_LIGHT_GEN_MIXING], [1], [build light generic mixing routines])
    fi
fi


dnl Check for headers/libs required for native backends
dnl Special OS objs for target systems
case "$target" in
    *irix*)
    # need dmedia
    AC_DEFINE([IRIS_SUPPORT], [1], [undocumented])
    LIBS="$LIBS -laudio"
    ;;
    *)
    # No extra libs needed
    ;;
esac

dnl check for sizeof
AC_CHECK_SIZEOF([void *])

# efence stuff
AC_ARG_ENABLE([efence],
[AS_HELP_STRING([--enable-efence],
                [enable EFENCE support @<:@default=no@:>@])])

if test "x$enable_efence" = xyes; then
  LIBS="$LIBS -lefence"
fi

# stubifying
AC_ARG_ENABLE([stub-notify],
[AS_HELP_STRING([--enable-debug-stub],
                [enable stub debugging @<:@default=no@:>@])])

if test "x$enable_debug_stub" = xyes; then
  AC_DEFINE([DEBUG_STUB], [1], [undocumented])
fi

dnl looping debug notification

AC_ARG_ENABLE([debug-loop],
[AS_HELP_STRING([--enable-debug-loop],
                [enable looping debugging @<:@default=no@:>@])])

if test "x$enable_debug_loop" = xyes; then
  AC_DEFINE([DEBUG_LOOP], [1], [undocumented])
fi

# conversion debug notification
AC_ARG_ENABLE([debug-convert],
[AS_HELP_STRING([--enable-debug-convert],
                [enable convertion debugging @<:@default=no@:>@])])

if test "x$enable_debug_convert" = xyes; then
  AC_DEFINE([DEBUG_CONVERT], [1], [undocumented])
fi

# config file debug notification
AC_ARG_ENABLE([debug-config],
[AS_HELP_STRING([--enable-debug-config],
                [enable config file debugging @<:@default=no@:>@])])

if test "x$enable_debug_config" = xyes; then
  AC_DEFINE([DEBUG_CONFIG], [1], [undocumented])
fi

# lock debug notification
AC_ARG_ENABLE([debug-lock],
[AS_HELP_STRING([--enable-debug-lock],
                [enable lock debugging @<:@default=no@:>@])])

if test "x$enable_debug_lock" = xyes; then
  AC_DEFINE([DEBUG_LOCK], [1], [undocumented])
fi

# extension debug notification
AC_ARG_ENABLE([debug-ext],
[AS_HELP_STRING([--enable-debug-ext],
                [enable extension debugging @<:@default=no@:>@])])

if test "x$enable_debug_ext" = xyes; then
  AC_DEFINE([DEBUG_EXT], [1], [undocumented])
fi

# buffer debug notification
AC_ARG_ENABLE([debug-buffer],
[AS_HELP_STRING([--enable-debug-buffer],
                [enable buffer debugging @<:@default=no@:>@])])

if test "x$enable_debug_buffer" = xyes; then
  AC_DEFINE([DEBUG_BUFFER], [1], [undocumented])
fi

# source debug notification
AC_ARG_ENABLE([debug-source],
[AS_HELP_STRING([--enable-debug-source],
                [enable source debugging @<:@default=no@:>@])])

if test "x$enable_debug_source" = xyes; then
  AC_DEFINE([DEBUG_SOURCE], [1], [undocumented])
fi

# mixer debug notification
AC_ARG_ENABLE([debug-mixer],
[AS_HELP_STRING([--enable-debug-mixer],
                [enable mixer debugging @<:@default=no@:>@])])

if test "x$enable_debug_mixer" = xyes; then
  AC_DEFINE([DEBUG_MIXER], [1], [undocumented])
fi

# streaming debug notification
AC_ARG_ENABLE([debug-streaming],
[AS_HELP_STRING([--enable-debug-streaming],
                [enable streaming debugging @<:@default=no@:>@])])

if test "x$enable_debug_streaming" = xyes; then
  AC_DEFINE([DEBUG_STREAMING], [1], [undocumented])
fi

# math debug notification
AC_ARG_ENABLE([debug-math],
[AS_HELP_STRING([--enable-debug-math],
                [enable math debugging @<:@default=no@:>@])])

if test "x$enable_debug_math" = xyes; then
  AC_DEFINE([DEBUG_MATH], [1], [undocumented])
fi

# mem debug notification
AC_ARG_ENABLE([debug-mem],
[AS_HELP_STRING([--enable-debug-mem],
                [enable mem debugging @<:@default=no@:>@])])

if test "x$enable_debug_mem" = xyes; then
  AC_DEFINE([DEBUG_MEM], [1], [undocumented])
fi

# context debug notification
AC_ARG_ENABLE([debug-context],
[AS_HELP_STRING([--enable-debug-context],
                [enable context debugging @<:@default=no@:>@])])

if test "x$enable_debug_context" = xyes; then
  AC_DEFINE([DEBUG_CONTEXT], [1], [undocumented])
fi

# debug queue structures/functions/etc
AC_ARG_ENABLE([debug-queue],
[AS_HELP_STRING([--enable-debug-queue],
                [enable queue debugging @<:@default=no@:>@])])

if test "x$enable_debug_queue" = xyes; then
  AC_DEFINE([DEBUG_QUEUE], [1], [undocumented])
fi

# debug filter
AC_ARG_ENABLE([debug-filter],
[AS_HELP_STRING([--enable-debug-filter],
                [enable filter debugging @<:@default=no@:>@])])

if test "x$enable_debug_filter" = xyes; then
  AC_DEFINE([DEBUG_FILTER], [1], [undocumented])
fi

# debug maximus
AC_ARG_ENABLE([debug-maximus],
[AS_HELP_STRING([--enable-debug-maximus],
                [enable all debugging @<:@default=no@:>@])])

if test "x$enable_debug_maximus" = xyes; then
  AC_DEFINE([DEBUG_MAXIMUS], [1], [undocumented])
fi

# Check available thread packages
AC_CHECK_HEADER([pthread.h], [PTHREAD_AVAILABLE=yes], [PTHREAD_AVAILABLE=no])

dnl do some platform specific threead mojo
case "$target" in
	*cygwin*|*mingw*)
		AC_CHECK_HEADER([windows.h], [WINTHREAD_AVAILABLE=yes]) ;;
	*morphos*)
		AC_CHECK_HEADER([dlfcn.h], [MOSTHREAD_AVAILABLE=yes]) ;;
	*) ;;
esac

dnl threadify.
if test "x$PTHREAD_AVAILABLE" = xyes; then

    if test "x$NO_PTHREAD_FLAG" = xyes; then
        LIBS="$LIBS"
    else
    if test "x$BROKEN_PTHREAD_FLAG" = xyes; then
	LIBS="$LIBS -pthread"
    else
	LIBS="$LIBS -lpthread"
    fi
    fi

    AC_DEFINE([USE_POSIXMUTEX], [1], [undocumented])
    AC_DEFINE([USE_POSIXTHREADS], [1], [undocumented])
else
	dnl no pthread, check others

	if test "x$WINTHREAD_AVAILABLE" = xyes; then
    		AC_DEFINE([USE_WINDOWSMUTEX], [1], [undocumented])
    		AC_DEFINE([USE_WINDOWSTHREADS], [1], [undocumented])
	else

		if test "x$MOSTHREAD_AVAILABLE" = xyes; then
				AC_DEFINE([USE_MORPHOSMUTEX], [1], [undocumented])
				AC_DEFINE([USE_MORPHOSTHREADS], [1], [undocumented])
		else

    		dnl no thread available

    		AC_DEFINE([NO_THREADS], [1], [undocumented])
		fi
	fi
fi

dnl check for ALSA headers
AC_ARG_ENABLE([alsa],
[AS_HELP_STRING([--enable-alsa],
                [enable the alsa backend @<:@default=no@:>@])])

if test "x$enable_alsa" = xyes; then
    AC_CHECK_HEADERS([time.h])
    AC_CHECK_HEADER([alsa/asoundlib.h],
      [AC_DEFINE([ALSA_SUPPORT], [1], [undocumented])
       OS_OBJS="$OS_OBJS \$(ALSA_OBJS)"],
      [enable_alsa=no],
      [#if HAVE_TIME_H
#include <time.h>
#endif])
fi

AC_ARG_ENABLE([alsa-dlopen],
[AS_HELP_STRING([--enable-alsa-dlopen],
                [dlopen alsa libs at runtime @<:@default=no@:>@])])

if test "x$enable_alsa" = xyes; then
  if test "x$enable_alsa_dlopen" = xyes; then
    AC_DEFINE([OPENAL_DLOPEN_ALSA], [1], [undocumented])
  else
    AC_DEFINE([OPENAL_DLOPEN_ALSA], [0], [undocumented])
    LIBS="$LIBS -lasound"
  fi
fi

# enable arts sound?
AC_ARG_ENABLE([arts],
[AS_HELP_STRING([--enable-arts],
                [enable aRts backend @<:@default=no@:>@])])

AC_ARG_ENABLE([arts-dlopen],
[AS_HELP_STRING([--enable-arts-dlopen],
                [dlopen aRts lib at runtime @<:@default=no@:>@])])

if test "x$enable_arts" = xyes; then
  AC_PATH_PROG([HAVEARTS], [artsc-config], [], [$PATH])
  if test -n "$HAVEARTS"; then
    ARTS_FEATURECFLAGS=`$HAVEARTS --cflags`
    ARTS_LIBS=`$HAVEARTS --libs`
    AC_DEFINE(ARTS_SUPPORT, [1], [undocumented])
    FEATURECFLAGS="$FEATURECFLAGS $ARTS_FEATURECFLAGS"
    OS_OBJS="$OS_OBJS \$(ARTS_OBJS)"
    if test "x$enable_arts_dlopen" = xyes; then
      AC_DEFINE([OPENAL_DLOPEN_ARTS], [1], [undocumented])
    else
      AC_DEFINE([OPENAL_DLOPEN_ARTS], [0], [undocumented])
      LIBS="$LIBS $ARTS_LIBS"
    fi
  else
    AC_MSG_WARN([artsc-config not found. No aRts support compiled in.])
  fi
fi

# enable esd sound?
AC_ARG_ENABLE([esd],
[AS_HELP_STRING([--enable-esd],
                [enable esd backend @<:@default=no@:>@])])

AC_ARG_ENABLE([esd-dlopen],
[AS_HELP_STRING([--enable-esd-dlopen],
                [dlopen esd lib at runtime @<:@default=no@:>@])])

if test "x$enable_esd" = xyes; then
  AC_PATH_PROG([HAVEESD], [esd-config], [], [$PATH])
  if test -n "$HAVEESD"; then
    ESD_FEATURECFLAGS=`$HAVEESD --cflags`
    ESD_LIBS=`$HAVEESD --libs`
    AC_DEFINE(ESD_SUPPORT, [1], [undocumented])
    FEATURECFLAGS="$FEATURECFLAGS $ESD_FEATURECFLAGS"
    OS_OBJS="$OS_OBJS \$(ESD_OBJS)"
    if test "x$enable_esd_dlopen" = xyes; then
      AC_DEFINE([OPENAL_DLOPEN_ESD], [1], [undocumented])
    else
      AC_DEFINE([OPENAL_DLOPEN_ESD], [0], [undocumented])
      LIBS="$LIBS $ESD_LIBS"
    fi
  else
    AC_MSG_WARN([esd-config not found. No esd support compiled in.])
  fi
fi

# enable wave output?
AC_ARG_ENABLE([waveout],
[AS_HELP_STRING([--enable-waveout],
                [enable waveout backend @<:@default=yes@:>@])])

if test "x$enable_waveout" != xno; then
  AC_DEFINE([WAVEOUT_SUPPORT], [1], [undocumented])
  OS_OBJS="$OS_OBJS \$(WAVEOUT_OBJS)"
fi

# enable null output?
AC_ARG_ENABLE([null],
[AS_HELP_STRING([--enable-null],
                [enable null backend @<:@default=yes@:>@])])

if test "x$enable_null" != xno; then
  AC_DEFINE([NULL_SUPPORT], [1], [undocumented])
  OS_OBJS="$OS_OBJS \$(NULL_OBJS)"
fi

# enable SDL sound?
AC_ARG_ENABLE([sdl],
[AS_HELP_STRING([--enable-sdl],
                [enable SDL backend @<:@default=no@:>@])])

if test "x$enable_sdl" = xyes; then
  # On cygwin targets, we don't rely on sdl-config.
  case "$target" in
    *cygwin*)
      HAVESDL=1
      SDL_FEATURECFLAGS="-I/cygwin/usr/include/SDL"
      SDL_LIBS=""
      FEATURECFLAGS="$FEATURECFLAGS $SDL_FEATURECFLAGS"
      LIBS="$LIBS $SDL_LIBS"
      OS_OBJS="$OS_OBJS \$(SDL_OBJS)"
      AC_DEFINE([SDL_SUPPORT], [1], [undocumented])
      ;;

    *)
      AC_PATH_PROG([HAVESDL], [sdl-config], [], [$PATH])
      if test -n "$HAVESDL"; then
        SDL_FEATURECFLAGS=`$HAVESDL --cflags`
        SDL_LIBS=`$HAVESDL --libs`
        AC_DEFINE([SDL_SUPPORT], [1], [undocumented])
        FEATURECFLAGS="$FEATURECFLAGS $SDL_FEATURECFLAGS"
        LIBS="$LIBS $SDL_LIBS"
        OLDCPPFLAGS="$CPPFLAGS"
        CPPFLAGS="-I/usr/local/include/SDL -I/usr/local/include"
        OS_OBJS="$OS_OBJS \$(SDL_OBJS)"
      else
        AC_MSG_WARN([SDL not found. No SDL support compiled in.])
      fi
      ;;
  esac
fi

# enable directsound backend?
AC_ARG_ENABLE([dsound],
[AS_HELP_STRING([--enable-dsound],
                [enable directsound backend @<:@default=no@:>@])])

if test "x$enable_dsound" = xyes; then
  # On cygwin targets, we don't rely on sdl-config.
  case "$target" in
    *cygwin*|*mingw*)
      AC_CHECK_HEADER([dsound.h],
        [AC_DEFINE([DSOUND_SUPPORT], [1], [undocumented])
        LIBS="$LIBS -ldsound"])
      AC_DEFINE([DSOUND_SUPPORT], [1], [undocumented])
      ;;
    *)
      echo "No dsound on nonwindows"
      ;;
  esac
fi

# enable vorbis support?
AC_ARG_ENABLE([vorbis],
[AS_HELP_STRING([--enable-vorbis],
                [enable vorbis support @<:@default=no@:>@])])

if test "x$enable_vorbis" = xyes; then
    AC_CHECK_LIB([vorbisfile], [ov_info],
      [AC_DEFINE([VORBIS_SUPPORT], [1], [undocumented])
      LIBS="$LIBS -lvorbis -lvorbisfile"], [], [-lvorbis])
fi

# enable SMPEG for mp3 stuff?
AC_ARG_ENABLE([smpeg],
[AS_HELP_STRING([--enable-smpeg],
                [enable smpeg support @<:@default=no@:>@])])

if test "x$enable_smpeg" = xyes; then
  AC_PATH_PROG([HAVESMPEG], [smpeg-config], [], [$PATH])
  if test -n "$HAVESMPEG"; then
    # smpeg requires sdl
    # Only test for SDL if we haven't already tested for it
    if test -z "$HAVESDL"; then
      AC_PATH_PROG([HAVESDL], [sdl-config], [], [$PATH])
      if test -n "$HAVESDL"; then
              SDL_FEATURECFLAGS=`$HAVESDL --cflags`
              SDL_LIBS=`$HAVESDL --libs`
              AC_DEFINE([SDL_SUPPORT], [1], [undocumented])
              FEATURECFLAGS="$FEATURECFLAGS $SDL_FEATURECFLAGS"
              LIBS="$LIBS $SDL_LIBS"
              OLDCPPFLAGS="$CPPFLAGS"
              CPPFLAGS="-I/usr/local/include/SDL -I/usr/local/include"
              OS_OBJS="$OS_OBJS \$(SDL_OBJS)"
      fi
    fi
    if test -n "$HAVESDL"; then
      SMPEG_FEATURECFLAGS=`$HAVESMPEG --cflags`
      SMPEG_LIBS=`$HAVESMPEG --libs`
      AC_DEFINE([SMPEG_SUPPORT], [1], [undocumented])
      FEATURECFLAGS="$FEATURECFLAGS $SMPEG_FEATURECFLAGS"
      LIBS="$LIBS $SMPEG_LIBS"
    else
      AC_MSG_WARN([smpeg requires SDL, and none found.])
    fi
  else
    AC_MSG_WARN([smpeg not found.  No support compiled in.])
  fi
fi

# enable capture support?
AC_ARG_ENABLE([capture],
[AS_HELP_STRING([--enable-capture],
                [enable capture support @<:@default=no@:>@])])

if test "x$enable_capture" = xyes; then
  AC_DEFINE([CAPTURE_SUPPORT], [1], [undocumented])
fi

AC_DEFINE([LINUX_AL], [1], [undocumented])

AC_SUBST([ARCH_INCLUDES])
AC_SUBST([ARCH_OBJS])
AC_SUBST([FEATURECFLAGS])
AC_SUBST([LIBS])
AC_SUBST([NASM])
AC_SUBST([NASM_FORMAT])
AC_SUBST([OPTIMIZATIONCFLAGS])
AC_SUBST([OS_OBJS])
AC_SUBST([PROFILINGLDFLAGS])
AC_SUBST([RANLIB])
AC_SUBST([SIZEOF_VOID_P])
AC_SUBST([WARNINGCFLAGS])

# Generate output.
AC_CONFIG_FILES([Makefile
                 admin/Makefile
                 admin/pkgconfig/Makefile
                 admin/pkgconfig/openal-config
                 admin/pkgconfig/openal.pc
                 common/Makefile
                 common/include/Makefile
                 include/Makefile
                 src/Makefile])

AC_CONFIG_COMMANDS([default], [chmod +x admin/pkgconfig/openal-config])
AC_OUTPUT
