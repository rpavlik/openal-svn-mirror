# Process this file with autoconf to produce a configure script.

AC_INIT([OpenAL library], [0.0.8], [openal-devel@opensource.creative.com], [openal])
AC_PREREQ([2.50])
AC_CONFIG_SRCDIR([doc/openal.texi])
AC_CONFIG_HEADER([config.h])
AC_SUBST([PACKAGE_VERSION_MAJOR], [`echo $PACKAGE_VERSION | sed 's/^\(@<:@^.@:>@*\)\(\.\{0,1\}\(.*\)\)$/\1'/`])

# Compatibility hack for older autoconf versions
m4_ifdef([AS_HELP_STRING], [], [AC_DEFUN([AS_HELP_STRING], [AC_HELP_STRING($][@)])])

# Checks for programs.

# We need ISO C99 features for e.g. snprintf.
CFLAGS="-D_ISOC99_SOURCE $CFLAGS"

AC_PROG_CC
AC_C_CONST
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S

REDUNDANT_DECLS_WARN=yes       # warnings about redudant decls
SHOULD_ENFORCE_ANSI=no         # whether we should use -ansi
SHADOW_WARN=yes                # warn about shadow variables
BROKEN_PTHREAD_FLAG=no         # BSD use -pthread, not -lpthread
NO_PTHREAD_FLAG=no
SHOULD_USE_FPIC=yes            # use -fPIC to compile position independent objs
SHOULD_USE_WERROR=no           # turn warnings into errors
SHARED_LIBRARY_EXT=so	       # shared library extension
SHOULD_USE_RDYNAMIC=yes        # doesn't work on irix
DO_MINOR_LIBS=yes              # wheether to copy foo.0.0

# warns
WFLAGS="-Wall -W -Wbad-function-cast -Wcast-qual -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wimplicit-function-declaration -Waggregate-return -Winline -Wpointer-arith -fno-common"

# We start off with a blank requirement.
requirements=""

# werror stuff
AC_ARG_ENABLE([werror],
[AS_HELP_STRING([--enable-werror],
                [enable fail on all warnings @<:@default=no@:>@])])

if test "x$enable_werror" = xyes; then
  SHOULD_USE_WERROR=yes
fi

# profile stuff
AC_ARG_ENABLE([profile],
[AS_HELP_STRING([--enable-profile],
                [enable profile @<:@default=no@:>@])])

if test "x$enable_profile" = xyes; then
  CFLAGS="-O2 -fexpensive-optimizations -ffast-math -funroll-loops -funroll-all-loops -g -pg $CFLAGS"
  LDFLAGS="-g -pg"
fi

AC_CANONICAL_TARGET

# optimization stuff
AC_ARG_ENABLE([optimization],
[AS_HELP_STRING([--enable-optimization],
                [enable optimization @<:@default=no@:>@])])

if test "x$enable_optimization" = xyes; then
  enable_paranoid_locks=no
  enable_empty_locks=no
  CFLAGS="-O6 -fexpensive-optimizations -funroll-all-loops -funroll-loops -fomit-frame-pointer -finline-functions -ffast-math $CFLAGS"

  # add -march
  case "$target" in
    *i386*) CFLAGS="$CFLAGS -march=i386" ;;
    *i486*) CFLAGS="$CFLAGS -march=i486" ;;
    *i586*) CFLAGS="$CFLAGS -march=i586" ;;
    *i686*) CFLAGS="$CFLAGS -march=i686" ;;
    mips*)  CFLAGS="$CFLAGS -march=mips" ;;
  esac
fi

# keep CFLAGS variable if set, otherwise set it to just debug
if test "x$CFLAGS" = x; then
  CFLAGS="-g -O2"
fi

# optimization stuff
AC_ARG_ENABLE([paranoid-locks],
[AS_HELP_STRING([--enable-paranoid-locks],
                [enable paranoid locks @<:@default=no@:>@])])

if test "x$enable_paranoid_locks" = xyes; then
  AC_DEFINE(PARANOID_LOCKS, 1, [undocumented])
fi

# no mutex locking for sync. driver
AC_ARG_ENABLE([empty-locks],
[AS_HELP_STRING([--enable-empty-locks],
                [enable empty locks @<:@default=no@:>@])])

if test "x$enable_empty_locks" = xyes; then
  AC_DEFINE(EMPTY_LOCKS, 1, [undocumented])
fi

# debug stuff
AC_ARG_ENABLE([debug],
[AS_HELP_STRING([--enable-debug],
                [enable debug @<:@default=no@:>@])])

if test "x$enable_debug" = xyes; then
  AC_DEFINE(DEBUG, 1, [undocumented])
fi

AC_C_BIGENDIAN

dnl library stuff
dnl I'm afraid we need libm
case "$target" in
    *darwin*)
    RANLIB=ranlib
    AC_CHECK_LIB(m, sin, LIBS="$LIBS -lm", echo "don't need libm...") ;;
    *)
    RANLIB=echo
    dnl Should be allow dlopen of extensions?
    AC_CHECK_LIB(dl, dlopen, LIBS="$LIBS -ldl", AC_DEFINE(NODLOPEN, 1, [undocumented]) echo "Can't dlopen extensions." )
    AC_CHECK_LIB(m, sin, LIBS="$LIBS -lm", echo "I'm afraid you'll need libm."; exit 1) ;;
esac

dnl Special OS objs for target systems
case "$target" in
    *linux*)
    dnl subtest for mips/linux systems with broken libio headers
    case "$target" in
    	*mips* )
	AC_DEFINE(BROKEN_LIBIO, 1, [undocumented]) ;;
	*) ;;
    esac
    WFLAGS="$WFLAGS -ansi"
    AC_DEFINE(LINUX_TARGET, 1, [undocumented])
    OS_OBJS="\$(LIN_OBJS)" ;;
    *bsd*)
    BROKEN_PTHREAD_FLAG=yes
    AC_DEFINE(BSD_TARGET, 1, [undocumented])
    OS_OBJS="\$(BSD_OBJS)" ;;
    *solaris*)
    AC_DEFINE(SOLARIS_TARGET, 1, [undocumented])
    OS_OBJS="\$(SOLARIS_OBJS)"
    REDUNDANT_DECLS_WARN=no
    SHOULD_ENFORCE_ANSI=no ;;
    *irix*)
    AC_DEFINE(IRIX_TARGET, 1, [undocumented])
    OS_OBJS="\$(IRIS_OBJS)"
    SHOULD_USE_RDYNAMIC=no
    REDUNDANT_DECLS_WARN=no
    SHOULD_ENFORCE_ANSI=no ;;
    *cygwin*|*mingw*)
    AC_DEFINE(WINDOWS_TARGET, 1, [undocumented])
    CFLAGS="-DAL_BUILD_LIBRARY $CFLAGS"
    OS_OBJS="\$(WINDOWS_OBJS)"
    LIBS="$LIBS -lwinmm"         # need that mm library for waveOutOpen etc
    SHARED_LIBRARY_EXT=dll       # shared library extension
    SHOULD_USE_FPIC=no      # all code is position independant, causes error
    SHADOW_WARN=no
    DO_MINOR_LIBS=no
    REDUNDANT_DECLS_WARN=no
    SHOULD_ENFORCE_ANSI=no ;;
    *darwin*)
    # I think darwin can use dlopen with fink, but
    # I don't want to require fink
    AC_DEFINE(NODLOPEN, 1, [undocumented])
    SHARED_LIBRARY_EXT=dylib
    BROKEN_PTHREAD_FLAG=yes
    NO_PTHREAD_FLAG=yes
    DO_MINOR_LIBS=no
    LIBS="$LIBS -I/System/Library/Frameworks/CoreAudio.framework/Headers/ -framework CoreAudio"
    AC_DEFINE(DARWIN_TARGET, 1, [undocumented])
    SHOULD_ENFORCE_ANSI=no
    SHOULD_USE_WERROR=no
    OS_OBJS="\$(DARWIN_OBJS)" ;;
	 *morphos*)
	 AC_DEFINE(MORPHOS_TARGET, 1, [undocumented])
	 OS_OBJS="\$(MORPHOS_OBJS)"
    REDUNDANT_DECLS_WARN=no
	 SHOULD_USE_FPIC=no
    SHOULD_ENFORCE_ANSI=no ;;
    *)
    OS_OBJS="\$(LIN_OBJS)" ;;
esac

dnl Special objs for architectures
case "$target" in
    *i386* | *i486* | *i586* | *i686* )
    ARCH_OBJS="\$(i386_OBJS)" ;;
    *)
    ARCH_OBJS="\$(PORTABLE_OBJS)" ;;
esac

dnl Check for headers/libs required for native backends
dnl Special OS objs for target systems
case "$target" in
    *linux*)
    # No extra libs needed
    ;;
    *bsd*)
    # No extra libs needed
    ;;
    *solaris*)
    # No extra libs needed
    ;;
    *irix*)
    # need dmedia
    AC_DEFINE(IRIS_SUPPORT, 1, [undocumented])
    LIBS="$LIBS -laudio"
    ;;
    *cygwin*|*mingw*)
    # No extra libs needed
    ;;
    *)
    # No extra libs needed
    ;;
esac

dnl check for sizeof
AC_CHECK_SIZEOF(void *)

# dmalloc stuff
AC_ARG_ENABLE([dmalloc],
[AS_HELP_STRING([--enable-dmalloc],
                [enable DMALLOC support @<:@default=no@:>@])])

if test "x$enable_dmalloc" = xyes; then
    AC_CHECK_HEADER(dmalloc.h,
    	dnl header found
	AC_DEFINE(DMALLOC, 1, [undocumented])
	AC_DEFINE(DMALLOC_FUNC_CHECK, 1, [undocumented])
    	INCLUDES="$INCLUDES -include \"/usr/include/dmalloc.h\""
    	LIBS="$LIBS -ldmallocth"
	SHOULD_USE_WERROR=no
    	REDUNDANT_DECLS_WARN=no,
	dnl header not found, check local/include
	dnl invalid cache
	[AC_CHECK_HEADER(/usr/local/include/dmalloc.h,
		dnl header found in local/include
		AC_DEFINE(DMALLOC, 1, [undocumented])
		AC_DEFINE(DMALLOC_FUNC_CHECK, 1, [undocumented])
		INCLUDES="$CFLAGS -include \"/usr/local/include/dmalloc.h\""
		LIBS="$LIBS -ldmallocth"
		SHOULD_USE_WERROR=no
		REDUNDANT_DECLS_WARN=no)])
fi

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
  AC_DEFINE(DEBUG_STUB, 1, [undocumented])
fi

dnl looping debug notification

AC_ARG_ENABLE([debug-loop],
[AS_HELP_STRING([--enable-debug-loop],
                [enable looping debugging @<:@default=no@:>@])])

if test "x$enable_debug_loop" = xyes; then
  AC_DEFINE(DEBUG_LOOP, 1, [undocumented])
fi

# conversion debug notification
AC_ARG_ENABLE([debug-convert],
[AS_HELP_STRING([--enable-debug-convert],
                [enable convertion debugging @<:@default=no@:>@])])

if test "x$enable_debug_convert" = xyes; then
  AC_DEFINE(DEBUG_CONVERT, 1, [undocumented])
fi

# config file debug notification
AC_ARG_ENABLE([debug-config],
[AS_HELP_STRING([--enable-debug-config],
                [enable config file debugging @<:@default=no@:>@])])

if test "x$enable_debug_config" = xyes; then
  AC_DEFINE(DEBUG_CONFIG, 1, [undocumented])
fi

# lock debug notification
AC_ARG_ENABLE([debug-lock],
[AS_HELP_STRING([--enable-debug-lock],
                [enable lock debugging @<:@default=no@:>@])])

if test "x$enable_debug_lock" = xyes; then
  AC_DEFINE(DEBUG_LOCK, 1, [undocumented])
fi

# extension debug notification
AC_ARG_ENABLE([debug-ext],
[AS_HELP_STRING([--enable-debug-ext],
                [enable extension debugging @<:@default=no@:>@])])

if test "x$enable_debug_ext" = xyes; then
  AC_DEFINE(DEBUG_EXT, 1, [undocumented])
fi

# buffer debug notification
AC_ARG_ENABLE([debug-buffer],
[AS_HELP_STRING([--enable-debug-buffer],
                [enable buffer debugging @<:@default=no@:>@])])

if test "x$enable_debug_buffer" = xyes; then
  AC_DEFINE(DEBUG_BUFFER, 1, [undocumented])
fi

# source debug notification
AC_ARG_ENABLE([debug-source],
[AS_HELP_STRING([--enable-debug-source],
                [enable source debugging @<:@default=no@:>@])])

if test "x$enable_debug_source" = xyes; then
  AC_DEFINE(DEBUG_SOURCE, 1, [undocumented])
fi

# mixer debug notification
AC_ARG_ENABLE([debug-mixer],
[AS_HELP_STRING([--enable-debug-mixer],
                [enable mixer debugging @<:@default=no@:>@])])

if test "x$enable_debug_mixer" = xyes; then
  AC_DEFINE(DEBUG_MIXER, 1, [undocumented])
fi

# streaming debug notification
AC_ARG_ENABLE([debug-streaming],
[AS_HELP_STRING([--enable-debug-streaming],
                [enable streaming debugging @<:@default=no@:>@])])

if test "x$enable_debug_streaming" = xyes; then
  AC_DEFINE(DEBUG_STREAMING, 1, [undocumented])
fi

# math debug notification
AC_ARG_ENABLE([debug-math],
[AS_HELP_STRING([--enable-debug-math],
                [enable math debugging @<:@default=no@:>@])])

if test "x$enable_debug_math" = xyes; then
  AC_DEFINE(DEBUG_MATH, 1, [undocumented])
fi

# mem debug notification
AC_ARG_ENABLE([debug-mem],
[AS_HELP_STRING([--enable-debug-mem],
                [enable mem debugging @<:@default=no@:>@])])

if test "x$enable_debug_mem" = xyes; then
  AC_DEFINE(DEBUG_MEM, 1, [undocumented])
fi

# context debug notification
AC_ARG_ENABLE([debug-context],
[AS_HELP_STRING([--enable-debug-context],
                [enable context debugging @<:@default=no@:>@])])

if test "x$enable_debug_context" = xyes; then
  AC_DEFINE(DEBUG_CONTEXT, 1, [undocumented])
fi

# debug queue structures/functions/etc
AC_ARG_ENABLE([debug-queue],
[AS_HELP_STRING([--enable-debug-queue],
                [enable queue debugging @<:@default=no@:>@])])

if test "x$enable_debug_queue" = xyes; then
  AC_DEFINE(DEBUG_QUEUE, 1, [undocumented])
fi

# debug filter
AC_ARG_ENABLE([debug-filter],
[AS_HELP_STRING([--enable-debug-filter],
                [enable filter debugging @<:@default=no@:>@])])

if test "x$enable_debug_filter" = xyes; then
  AC_DEFINE(DEBUG_FILTER, 1, [undocumented])
fi

# debug maximus
AC_ARG_ENABLE([debug-maximus],
[AS_HELP_STRING([--enable-debug-maximus],
                [enable all debugging @<:@default=no@:>@])])

if test "x$enable_debug_maximus" = xyes; then
  AC_DEFINE(DEBUG_MAXIMUS, 1, [undocumented])
fi

# Check available thread packages
AC_CHECK_HEADER(pthread.h, PTHREAD_AVAILABLE=yes, PTHREAD_AVAILABLE=no)

dnl do some platform specific threead mojo
case "$target" in
	*cygwin*|*mingw*)
		AC_CHECK_HEADER(windows.h, WINTHREAD_AVAILABLE=yes) ;;
	*morphos*)
		AC_CHECK_HEADER(dlfcn.h, MOSTHREAD_AVAILABLE=yes) ;;
	*) ;;
esac

dnl threadify.
if test x$PTHREAD_AVAILABLE = xyes; then

    if test $NO_PTHREAD_FLAG = yes; then
        LIBS="$LIBS"
    else
    if test $BROKEN_PTHREAD_FLAG = yes; then
	LIBS="$LIBS -pthread"
    else
	LIBS="$LIBS -lpthread"
    fi
    fi

    AC_DEFINE(USE_POSIXMUTEX, 1, [undocumented])
    AC_DEFINE(USE_POSIXTHREADS, 1, [undocumented])
else
	dnl no pthread, check others

	if test x$WINTHREAD_AVAILABLE = xyes; then
    		AC_DEFINE(USE_WINDOWSMUTEX, 1, [undocumented])
    		AC_DEFINE(USE_WINDOWSTHREADS, 1, [undocumented])
	else

		if test x$MOSTHREAD_AVAILABLE = xyes; then
				AC_DEFINE(USE_MORPHOSMUTEX, 1, [undocumented])
				AC_DEFINE(USE_MORPHOSTHREADS, 1, [undocumented])
		else

    		dnl no thread available

    		AC_DEFINE(NO_THREADS, 1, [undocumented])
		fi
	fi
fi


dnl arch specific ASM files
ARCH_OBJS="\$(PORTABLE_OBJS)"

dnl check for ALSA headers
AC_ARG_ENABLE([alsa],
[AS_HELP_STRING([--enable-alsa],
                [enable the alsa backend @<:@default=no@:>@])])

if test "x$enable_alsa" = xyes; then
    AC_CHECK_HEADERS([time.h])
    AC_CHECK_HEADER([alsa/asoundlib.h],
      [SHADOW_WARN=no
       REDUNDANT_DECLS_WARN=no
       AC_DEFINE(ALSA_SUPPORT, 1, [undocumented])
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
    AC_DEFINE(OPENAL_DLOPEN_ALSA, 1, [undocumented])
  else
    AC_DEFINE(OPENAL_DLOPEN_ALSA, 0, [undocumented])
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
  AC_PATH_PROG(HAVEARTS, artsc-config, , $PATH)
  if test ! -z "$HAVEARTS"; then
    ARTS_CFLAGS=`$HAVEARTS --cflags`
    ARTS_LIBS=`$HAVEARTS --libs`
    AC_DEFINE(ARTS_SUPPORT, 1, [undocumented])
    CFLAGS="$CFLAGS $ARTS_CFLAGS"
    OS_OBJS="$OS_OBJS \$(ARTS_OBJS)"
    if test x$enable_arts_dlopen = xyes; then
      AC_DEFINE(OPENAL_DLOPEN_ARTS, 1, [undocumented])
    else
      AC_DEFINE(OPENAL_DLOPEN_ARTS, 0, [undocumented])
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
  AC_PATH_PROG(HAVEESD, esd-config, , $PATH)
  if test ! -z "$HAVEESD"; then
    ESD_CFLAGS=`$HAVEESD --cflags`
    ESD_LIBS=`$HAVEESD --libs`
    AC_DEFINE(ESD_SUPPORT, 1, [undocumented])
    CFLAGS="$CFLAGS $ESD_CFLAGS"
    OS_OBJS="$OS_OBJS \$(ESD_OBJS)"
    # bad audiofile.h!  bad!
    SHADOW_WARN=no
    if test x$enable_esd_dlopen = xyes; then
      AC_DEFINE(OPENAL_DLOPEN_ESD, 1, [undocumented])
    else
      AC_DEFINE(OPENAL_DLOPEN_ESD, 0, [undocumented])
      LIBS="$LIBS $ESD_LIBS"
    fi
  else
    AC_MSG_WARN([esd-config not found. No esd support compiled in.])
  fi
fi

dnl check for IRIS headers
dnl AC_ARG_ENABLE([iris],
dnl [AS_HELP_STRING([--enable-iris],
dnl                 [enable the irix backend @<:@default=no@:>@])])
dnl
dnl if test "x$enable_iris" = xyes; then
dnl   CPPFLAGS=" "
dnl   OS_OBJS="$OS_OBJS \$(IRIS_OBJS)"
dnl   DEFINES="$DEFINES -DIRIS_SUPPORT"
dnl   LIBS="$LIBS -laudio"
dnl fi

# enable wave output?
AC_ARG_ENABLE([waveout],
[AS_HELP_STRING([--enable-waveout],
                [enable waveout backend @<:@default=yes@:>@])])

if test "x$enable_waveout" != xno; then
  AC_DEFINE(WAVEOUT_SUPPORT, 1, [undocumented])
  OS_OBJS="$OS_OBJS \$(WAVEOUT_OBJS)"
fi

# enable null output?
AC_ARG_ENABLE([null],
[AS_HELP_STRING([--enable-null],
                [enable null backend @<:@default=yes@:>@])])

if test "x$enable_null" != xno; then
  AC_DEFINE(NULL_SUPPORT, 1, [undocumented])
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
      SDL_CFLAGS="-I/cygwin/usr/include/SDL"
      SDL_LIBS=""
      CFLAGS="$CFLAGS $SDL_CFLAGS"
      LIBS="$LIBS $SDL_LIBS"
      OS_OBJS="$OS_OBJS \$(SDL_OBJS)"
      AC_DEFINE(SDL_SUPPORT, 1, [undocumented])
      ;;

    *)
      AC_PATH_PROG(HAVESDL, sdl-config, , $PATH)
      if test ! -z "$HAVESDL"; then
        SDL_CFLAGS=`$HAVESDL --cflags`
        SDL_LIBS=`$HAVESDL --libs`
        AC_DEFINE(SDL_SUPPORT, 1, [undocumented])
        CFLAGS="$CFLAGS $SDL_CFLAGS"
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
      AC_CHECK_HEADER(dsound.h,
        AC_DEFINE(DSOUND_SUPPORT, 1, [undocumented])
        LIBS="$LIBS -ldsound")
      AC_DEFINE(DSOUND_SUPPORT, 1, [undocumented])
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
    AC_CHECK_LIB(vorbisfile, ov_info,
      AC_DEFINE( VORBIS_SUPPORT, 1, [undocumented] )
      LIBS="$LIBS -lvorbis -lvorbisfile",, -lvorbis )
fi

# enable SMPEG for mp3 stuff?
AC_ARG_ENABLE([smpeg],
[AS_HELP_STRING([--enable-smpeg],
                [enable smpeg support @<:@default=no@:>@])])

if test "x$enable_smpeg" = xyes; then
  AC_PATH_PROG(HAVESMPEG, smpeg-config, , $PATH)
  if test ! -z "$HAVESMPEG"; then
    # smpeg requires sdl
    # Only test for SDL if we haven't already tested for it
    if test -z "$HAVESDL"; then
      AC_PATH_PROG(HAVESDL, sdl-config, , $PATH)
      if test ! -z "$HAVESDL"; then
              SDL_CFLAGS=`$HAVESDL --cflags`
              SDL_LIBS=`$HAVESDL --libs`
              AC_DEFINE(SDL_SUPPORT, 1, [undocumented])
              CFLAGS="$CFLAGS $SDL_CFLAGS"
              LIBS="$LIBS $SDL_LIBS"
              OLDCPPFLAGS="$CPPFLAGS"
              CPPFLAGS="-I/usr/local/include/SDL -I/usr/local/include"
              OS_OBJS="$OS_OBJS \$(SDL_OBJS)"
      fi
    fi
    if test ! -z "$HAVESDL"; then
      SMPEG_CFLAGS=`$HAVESMPEG --cflags`
      SMPEG_LIBS=`$HAVESMPEG --libs`
      AC_DEFINE(SMPEG_SUPPORT, 1, [undocumented])
      CFLAGS="$CFLAGS $SMPEG_CFLAGS"
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
  AC_DEFINE(CAPTURE_SUPPORT, 1, [undocumented])
fi

# makeinfo stuff
AC_CHECK_PROG(MAKEINFO, makeinfo, yes, no)
if test "x$MAKEINFO" = xyes; then
  MAKEINFO='makeinfo'
else
  # for silly people without texinfo
  MAKEINFO='@echo'
fi

# Warning flags
#   This is where we prune down warnings based on the limitations of
#   the platform or included headers.

# warning pruning based on CPU type
case "$target" in
  *alpha*)
    SHOULD_ENFORCE_ANSI=no ;; # f***ed up math headers
esac

if test ! -z "$HAVESDL"; then
  # If we don't have SDL, we can use -ansi for gcc.  If we do, SDL
  # uses long long which is bad mojo
  SHOULD_USE_WERROR=no
fi

# Should we use turn warnings into errors?
if test "x$SHOULD_USE_WERROR" = xyes; then
  WFLAGS="$WFLAGS -Werror -pedantic-errors"
fi

# if we are forced to live with redundant decls, turn off those errors
if test $REDUNDANT_DECLS_WARN = no; then
  WFLAGS="$WFLAGS -Wno-redundant-decls"
else
  WFLAGS="$WFLAGS -Wredundant-decls"
fi

# Typical UNIX doesn't like -ansi
if test "x$SHOULD_ENFORCE_ANSI" = xyes; then
  WFLAGS="-ansi $WFLAGS"
fi

# Stupid audiofile headers
if test "x$SHADOW_WARN" = xyes; then
  WFLAGS="-Wshadow $WFLAGS"
fi

# Stupid audiofile headers
if test "x$SHOULD_USE_FPIC" = xyes; then
  AC_DEFINE(PIC, 1, [undocumented])
  CFLAGS="$CFLAGS -fPIC"
fi

# add warnings to CFALGS
CFLAGS="$CFLAGS $WFLAGS"

# okay, rdynamic okay?
if test "x$SHOULD_USE_RDYNAMIC" = xyes; then
  AC_DEFINE_UNQUOTED(RDYNAMIC, "-rdynamic", [undocumented])
fi

# Da domb!
AC_DEFINE(BOMB, 1, [undocumented])

AC_DEFINE(LINUX_AL, 1, [undocumented])

AC_SUBST([DO_MINOR_LIBS])
AC_SUBST([NASM])
AC_SUBST([RANLIB])
AC_SUBST([CC])
AC_SUBST([CFLAGS])
AC_SUBST([DEFINES])
AC_SUBST([LDFLAGS])
AC_SUBST([LIBS])
AC_SUBST([INCLUDES])
AC_SUBST([MAKEINFO])
AC_SUBST([OS_OBJS])
AC_SUBST([ARCH_OBJS])
AC_SUBST([SHARED_LIBRARY_EXT])
AC_SUBST([SIZEOF_VOID_P])
AC_SUBST([RDYNAMIC])
AC_SUBST([requirements])

# Generate output.
AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 openal-config
                 openal.pc
                 src/Makefile
                 test/Makefile])

AC_CONFIG_COMMANDS([default], [chmod +x openal-config])
AC_OUTPUT

