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
## libtool shared library version.
################################################################################

# Some information from the libtool info pages and the autobook regarding how to
# handle the library version info:
#
# 1. Start with version information of `0:0:0' for each libtool library.
#
# 2. Update the version information only immediately before a public release of
#    your software. More frequent updates are unnecessary, and only guarantee
#    that the current interface number gets larger faster.
#
# 3. If the library source code has changed at all since the last update, then
#    increment REVISION (`C:R:A' becomes `C:r+1:A'). This is a new revision of
#    the current interface.
#
# 4. If any interfaces have been added, removed, or changed since the last
#    update, increment CURRENT, and set REVISION to 0. This is the first
#    revision of a new interface.
#
# 5. If any interfaces have been added since the last public release, then
#    increment AGE. This release is backwards compatible with the previous
#    release.
#
# 6. If any interfaces have been removed since the last public release, then set
#    AGE to 0. This release has a new, but backwards incompatible interface.
#
# NEVER try to set the interface numbers so that they correspond to the release
# number of your package. This is an abuse that only fosters misunderstanding of
# the purpose of library versions. Furthermore, do not confuse those versions
# with the version of the specification which is implemented.

CURRENT=1
REVISION=0
AGE=0

AC_SUBST([VERSIONINFO], ["$CURRENT:$REVISION:$AGE"])

################################################################################
# Checks for programs.
################################################################################

# We need ISO C99 features (e.g. snprintf) and POSIX IEEE Std 1003.1b-1993
# features (e.g. struct timespec, used in <alsa/asoundlib.h>, and nanosleep)
AC_DEFINE([_ISOC99_SOURCE], [1], [Define to 1 if extensions to ISO C89 from ISO C99 are needed.])
AC_DEFINE([_POSIX_C_SOURCE], [199309], [Define to the POSIX version that should be used.])

# Without __NO_CTYPE tolower and friends are macros which introduce a GLIBC 2.3
# dependency. By defining this identifier we are currently backwards compatible
# to GLIBC 2.1.3, which is a good thing. In addition, the macros lead to code
# which triggers warnings with -Wunreachable-code.
AC_DEFINE([__NO_CTYPE], [1], [Define to 1 if tolower and friends should not be macros.])

AC_DEFINE([AL_BUILD_LIBRARY], [1], [Define to 1 if we are building the OpenAL library.])

AC_PROG_CC
AC_C_CONST
ALUT_C__ATTRIBUTE__
AC_EXEEXT
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_SUBST([LIBTOOL_DEPS])
AC_PROG_CXX
AC_ARG_VAR([PKG_CONFIG], [package configuration utility])
AC_CHECK_PROGS([PKG_CONFIG], [pkg-config], [none])

################################################################################
# Checks for library functions.
################################################################################

AC_CHECK_HEADERS([sys/time.h time.h windows.h])

# Checks for library functions.
ALUT_CHECK_FUNC([[@%:@include <time.h>]],
                [nanosleep], [[((struct timespec*)0, (struct timespec*)0)]])

ALUT_CHECK_FUNC([[@%:@include <unistd.h>]],
                [usleep], [[(0)]])

ALUT_CHECK_FUNC([[@%:@include <windows.h>]],
                [Sleep], [[(0)]])

AC_CHECK_LIBM
AC_SUBST([LIBM])

ACX_PTHREAD(
  [LIBS="$PTHREAD_LIBS $LIBS"
   FEATURECFLAGS="$FEATURECFLAGS $PTHREAD_CFLAGS"
   CC=$PTHREAD_CC
   AC_DEFINE([USE_POSIXTHREADING], [1], [Define to 1 if POSIX threading should be used.])],
  [AC_CHECK_HEADER([windows.h],
    [AC_DEFINE([USE_WINDOWSTHREADING], [1], [Define to 1 if Windows threading should be used.])],
    [AC_CHECK_HEADER([dos/dosextens.h],
      [AC_DEFINE([USE_MORPHOSTHREADING], [1], [Define to 1 if MorphOS threading should be used.])],
      [AC_DEFINE([NO_THREADING], [1], [undocumented])],
      [AC_INCLUDES_DEFAULT([])])],
    [AC_INCLUDES_DEFAULT([])])])

AC_FUNC_SELECT_ARGTYPES

################################################################################
# Handle warning-related options.
################################################################################

AC_ARG_ENABLE([warnings],
[AS_HELP_STRING([--enable-warnings],
                [enable pedantic compiler warnings @<:@default=yes@:>@])])

if test "x$enable_warnings" = xno; then
  WARNINGCFLAGS=""
else
  # Doing it in two steps gives a nicer message...
  AX_CFLAGS_WARN_ALL_ANSI([flags])
  WARNINGCFLAGS=$flags
fi

AC_ARG_ENABLE([more-warnings],
[AS_HELP_STRING([--enable-more-warnings],
                [enable even more compiler warnings @<:@default=no@:>@])])

if test "x$enable_more_warnings" = xyes; then
  if test "x$enable_warnings" = xno; then
    AC_MSG_WARN([--enable-more-warnings ignored because of --disable-warnings])
  elif test "x$GCC" != xyes; then
    AC_MSG_WARN([--enable-more-warnings ignored because no GCC was detected])
  else
    # The long list of warning options below contains every GCC warning option
    # which is not automatically enabled with -Wall. The only exceptions to this
    # rule are:
    #
    #    -Wpadded:
    #      Perhaps good for optimizing out data layout, but not in general.
    #
    #    -Wconversion:
    #      Passing e.g. float as an argument is fine, we always have prototypes.
    #
    #    -Wfloat-equal:
    #       Often we should use an epsilon, but there are some correct uses,
    #       too, so we leave it out for now. ToDo: Fix those warnings, anyway!
    #
    #    -Wunreachable-code:
    #       Some older GCC versions give false positives about unreachable code.
    #       Furthermore, using GNU's <string.h> header normally results in lots
    #       of string functions like strcmp being macros which result in such
    #       false positives, too. To disable those, one can use
    #       -D__NO_STRING_INLINES for testing. And another source for false
    #       positives is inlining, so leaving out -finline-functions or using
    #       -fno-inline-functions explicitly might help, too. All in all, using
    #       this warning option is too troublesome to really help.
    WARNINGCFLAGS="$WARNINGCFLAGS -W -Waggregate-return -Wbad-function-cast -Wcast-align -Wcast-qual -Wdisabled-optimization -Wendif-labels -Winline -Wlong-long -Wmissing-declarations -Wmissing-format-attribute -Wmissing-noreturn -Wmissing-prototypes -Wnested-externs -Wpacked -Wpointer-arith -Wredundant-decls -Wshadow -Wsign-compare -Wstrict-prototypes -Wundef -Wwrite-strings"

    # Check for GCC 4.x-only warning options.
    ALUT_CHECK_FLAG([-Wdeclaration-after-statement],
                    [WARNINGCFLAGS="$WARNINGCFLAGS -Wdeclaration-after-statement"])
    ALUT_CHECK_FLAG([-Winvalid-pch],
                    [WARNINGCFLAGS="$WARNINGCFLAGS -Winvalid-pch"])
    ALUT_CHECK_FLAG([-Wmissing-field-initializers],
                    [WARNINGCFLAGS="$WARNINGCFLAGS -Wmissing-field-initializers"])
    # We cheat here a bit: The code generated by AC_LANG_PROGRAM triggers a
    # warning with -Wold-style-definition, so we assume that this flag is
    # supported whenever -Wvariadic-macros is.
    ALUT_CHECK_FLAG([-Wvariadic-macros],
                    [WARNINGCFLAGS="$WARNINGCFLAGS -Wvariadic-macros -Wold-style-definition"])
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
# Handle debugging-related options.
################################################################################

need_debug=no

AC_ARG_ENABLE([debug],
[AS_HELP_STRING([--enable-debug],
                [enable debug code and assertions @<:@default=no@:>@])])

if test "x$enable_debug" = xyes; then
  AC_DEFINE([DEBUG], [1], [Define to 1 if you want to include debugging code.])
  need_debug=yes
else
  AC_DEFINE([NDEBUG], [1], [Define to 1 if you want to disable the assert() macro.])
fi

AC_ARG_ENABLE([efence],
[AS_HELP_STRING([--enable-efence],
                [enable Electric Fence support @<:@default=no@:>@])])

if test "x$enable_efence" = xyes; then
  openal_saved_LIBS=$LIBS
  LIBS="-lefence $LIBS"
  AC_LINK_IFELSE([AC_LANG_PROGRAM([extern int EF_ALIGNMENT;], [EF_ALIGNMENT = 8;])],
    [:],
    [AC_MSG_WARN([--enable-efence ignored because the Electric Fence library was not found.])
     LIBS=$openal_saved_LIBS])
fi

# stubifying
AC_ARG_ENABLE([debug-stub],
[AS_HELP_STRING([--enable-debug-stub],
                [enable stub debugging @<:@default=no@:>@])])

if test "x$enable_debug_stub" = xyes; then
  AC_DEFINE([DEBUG_STUB], [1], [Define to 1 to enable stub debugging.])
  need_debug=yes
fi

# looping debug notification
AC_ARG_ENABLE([debug-loop],
[AS_HELP_STRING([--enable-debug-loop],
                [enable looping debugging @<:@default=no@:>@])])

if test "x$enable_debug_loop" = xyes; then
  AC_DEFINE([DEBUG_LOOP], [1], [Define to 1 to enable looping debugging.])
  need_debug=yes
fi

# conversion debug notification
AC_ARG_ENABLE([debug-convert],
[AS_HELP_STRING([--enable-debug-convert],
                [enable conversion debugging @<:@default=no@:>@])])

if test "x$enable_debug_convert" = xyes; then
  AC_DEFINE([DEBUG_CONVERT], [1], [Define to 1 to enable conversion debugging.])
  need_debug=yes
fi

# config file debug notification
AC_ARG_ENABLE([debug-config],
[AS_HELP_STRING([--enable-debug-config],
                [enable config file debugging @<:@default=no@:>@])])

if test "x$enable_debug_config" = xyes; then
  AC_DEFINE([DEBUG_CONFIG], [1], [Define to 1 to enable config file debugging.])
  need_debug=yes
fi

# lock debug notification
AC_ARG_ENABLE([debug-lock],
[AS_HELP_STRING([--enable-debug-lock],
                [enable lock debugging @<:@default=no@:>@])])

if test "x$enable_debug_lock" = xyes; then
  AC_DEFINE([DEBUG_LOCK], [1], [Define to 1 to enable lock debugging.])
  need_debug=yes
fi

# extension debug notification
AC_ARG_ENABLE([debug-ext],
[AS_HELP_STRING([--enable-debug-ext],
                [enable extension debugging @<:@default=no@:>@])])

if test "x$enable_debug_ext" = xyes; then
  AC_DEFINE([DEBUG_EXT], [1], [Define to 1 to enable extension debugging.])
  need_debug=yes
fi

# buffer debug notification
AC_ARG_ENABLE([debug-buffer],
[AS_HELP_STRING([--enable-debug-buffer],
                [enable buffer debugging @<:@default=no@:>@])])

if test "x$enable_debug_buffer" = xyes; then
  AC_DEFINE([DEBUG_BUFFER], [1], [Define to 1 to enable buffer debugging.])
  need_debug=yes
fi

# source debug notification
AC_ARG_ENABLE([debug-source],
[AS_HELP_STRING([--enable-debug-source],
                [enable source debugging @<:@default=no@:>@])])

if test "x$enable_debug_source" = xyes; then
  AC_DEFINE([DEBUG_SOURCE], [1], [Define to 1 to enable source debugging.])
  need_debug=yes
fi

# mixer debug notification
AC_ARG_ENABLE([debug-mixer],
[AS_HELP_STRING([--enable-debug-mixer],
                [enable mixer debugging @<:@default=no@:>@])])

if test "x$enable_debug_mixer" = xyes; then
  AC_DEFINE([DEBUG_MIXER], [1], [Define to 1 to enable mixer debugging.])
  need_debug=yes
fi

# streaming debug notification
AC_ARG_ENABLE([debug-streaming],
[AS_HELP_STRING([--enable-debug-streaming],
                [enable streaming debugging @<:@default=no@:>@])])

if test "x$enable_debug_streaming" = xyes; then
  AC_DEFINE([DEBUG_STREAMING], [1], [Define to 1 to enable streaming debugging.])
  need_debug=yes
fi

# math debug notification
AC_ARG_ENABLE([debug-math],
[AS_HELP_STRING([--enable-debug-math],
                [enable math debugging @<:@default=no@:>@])])

if test "x$enable_debug_math" = xyes; then
  AC_DEFINE([DEBUG_MATH], [1], [Define to 1 to enable math debugging.])
  need_debug=yes
fi

# mem debug notification
AC_ARG_ENABLE([debug-mem],
[AS_HELP_STRING([--enable-debug-mem],
                [enable memory debugging @<:@default=no@:>@])])

if test "x$enable_debug_mem" = xyes; then
  AC_DEFINE([DEBUG_MEM], [1], [Define to 1 to enable memory debugging.])
  need_debug=yes
fi

# context debug notification
AC_ARG_ENABLE([debug-context],
[AS_HELP_STRING([--enable-debug-context],
                [enable context debugging @<:@default=no@:>@])])

if test "x$enable_debug_context" = xyes; then
  AC_DEFINE([DEBUG_CONTEXT], [1], [Define to 1 to enable context debugging.])
  need_debug=yes
fi

# debug queue structures/functions/etc
AC_ARG_ENABLE([debug-queue],
[AS_HELP_STRING([--enable-debug-queue],
                [enable queue debugging @<:@default=no@:>@])])

if test "x$enable_debug_queue" = xyes; then
  AC_DEFINE([DEBUG_QUEUE], [1], [Define to 1 to enable queue debugging.])
  need_debug=yes
fi

# debug filter
AC_ARG_ENABLE([debug-filter],
[AS_HELP_STRING([--enable-debug-filter],
                [enable filter debugging @<:@default=no@:>@])])

if test "x$enable_debug_filter" = xyes; then
  AC_DEFINE([DEBUG_FILTER], [1], [Define to 1 to enable filter debugging.])
  need_debug=yes
fi

# debug maximus
AC_ARG_ENABLE([debug-maximus],
[AS_HELP_STRING([--enable-debug-maximus],
                [enable all debugging @<:@default=no@:>@])])

if test "x$enable_debug_maximus" = xyes; then
  AC_DEFINE([DEBUG_MAXIMUS], [1], [Define to 1 to enable all debugging.])
  need_debug=yes
fi

AM_CONDITIONAL([NEEDDEBUG], [test "x$need_debug" = xyes])

################################################################################
# Handle optimization-related options.
################################################################################

AC_ARG_ENABLE([optimization],
[AS_HELP_STRING([--enable-optimization],
                [enable optimization @<:@default=yes@:>@])])

if test "x$enable_optimization" != xno; then
  ALUT_CHECK_FLAG([-finline-functions],
                  [OPTIMIZATIONCFLAGS="$OPTIMIZATIONCFLAGS -finline-functions"])
  ALUT_CHECK_FLAG([-ffast-math],
                  [OPTIMIZATIONCFLAGS="$OPTIMIZATIONCFLAGS -ffast-math"])
  # -pg and -fomit-frame-pointer are incompatible. ToDo: Test for this!
  if test "x$enable_profile" != xyes; then
    ALUT_CHECK_FLAG([-fomit-frame-pointer],
                    [OPTIMIZATIONCFLAGS="$OPTIMIZATIONCFLAGS -fomit-frame-pointer"])
  fi
fi

# profiling stuff
AC_ARG_ENABLE([profile],
[AS_HELP_STRING([--enable-profile],
                [enable profile @<:@default=no@:>@])])

if test "x$enable_profile" = xyes; then
  ALUT_CHECK_FLAG([-pg],
                  [OPTIMIZATIONCFLAGS="$OPTIMIZATIONCFLAGS -pg"
                   PROFILINGLDFLAGS="$LDFLAGS -pg"],
                  [AC_MSG_WARN([--enable-profile ignored because $CC does not support -pg.])])
fi

AC_C_BIGENDIAN

# A few notes about dynamic loading of libraries: We should really use libtool's
# mechanisms for this, but for now we have to get our hands dirty with dlopen
# and friends and hope for the best. These calls are available on a lot of
# *nices and Cygwin and according to Apple's developer pages, they are available
# on Mac OS X 10.3 and later, too. In former Mac OS X versions one has to use
# the dlcompat library to emulate this facility. Furthermore, adding "-ldl"
# might be required, but not always (e.g. Cygwin).

AC_CHECK_HEADERS([dlfcn.h],
  [AC_CHECK_LIB([dl], [dlopen], [LIBS="-ldl $LIBS"])],
  [AC_MSG_WARN([Dynamic loading of libraries is not available.])
   AC_DEFINE([NODLOPEN], [1], [Define to 1 if dynamic loading of libraries is not available.])])

# ranlib stuff
case "$target" in
    *darwin*) RANLIB=ranlib ;;
    *) RANLIB=echo ;;
esac

dnl Special OS objs for target systems
case "$target" in
  *darwin*) LIBS="$LIBS -I/System/Library/Frameworks/CoreAudio.framework/Headers/ -framework CoreAudio"
            AC_DEFINE([USE_BACKEND_NATIVE_DARWIN], [1], [undocumented]) ;;
esac

dnl Special objs for architectures
case "$target" in
    *i386* | *i486* | *i586* | *i686* | *x86_64*)
    openal_x86_support_yn=yes

    AC_MSG_CHECKING([for MMX support])
    AC_TRY_COMPILE([
#ifndef __MMX__
#error no MMX support
#endif
      ],, [openal_x86_mmx_support_yn=yes], [openal_x86_mmx_support_yn=no])
    AC_MSG_RESULT([$openal_x86_mmx_support_yn])
    ;;
esac

if test "x$openal_x86_mmx_support_yn" = xyes; then
case "$target" in
    *i386* | *i486* | *i586* | *i686*)
        AC_PATH_PROG([NASM], [nasm], [no])

        if test "x$NASM" != xno; then
            AC_DEFINE([HAVE_CPU_CAPS_DETECTION], [1], [detect cpu capabilities])
            openal_x86_nasm_support_yn=yes

            if test "x$openal_x86_mmx_support_yn" = xyes; then
                AC_DEFINE([HAVE_MMX_MEMCPY], [1], [mmx memcpy])
                openal_x86_nasm_mmx_support_yn=yes
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
        ;;
esac


AC_ARG_ENABLE([optim-generic],
[AS_HELP_STRING([--enable-optim-generic],
                [x86(_64): even if mmx enabled, build optimized instead of light generic mixing routines  @<:@i686/x86_64:default=no@:>@])])

if test "x$openal_x86_mmx_support_yn" = xyes; then
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
fi

AM_CONDITIONAL([X86ENABLED], [test "x$openal_x86_support_yn" = xyes])
AM_CONDITIONAL([X86MMXENABLED], [test "x$openal_x86_mmx_support_yn" = xyes])
AM_CONDITIONAL([X86NASMENABLED], [test "x$openal_x86_nasm_support_yn" = xyes])
AM_CONDITIONAL([X86NASMMMXENABLED], [test "x$openal_x86_nasm_mmx_support_yn" = xyes])

# Note that -fvisibility=... support implies __attribute__((visibility(...))) 
# support.
ALUT_CHECK_FLAG([-fvisibility=hidden],
                [OPTIMIZATIONCFLAGS="$OPTIMIZATIONCFLAGS -fvisibility=hidden"
                 AC_DEFINE([HAVE_GCC_VISIBILITY], [1],
                           [Define to 1 if we are using a GCC with symbol visibility support.])])

################################################################################
# Checks for backends.
################################################################################
#
# Detecting backends in the various configurations and on the various platforms
# is a nightmare, but let's at least describe the general ideas:
#
#  * Every non-trivial backend has two switches: One for enabling/disabling the
#    backend and one for enabling/disabling dlopening the relevant libraries at
#    runtime.
#
#  * If the backend has been disabled explicitly, there is nothing to do,
#    otherwise we need at least the relevant header.
#
#  * To find any needed flags for the C compiler, use pkg-config or the outdated
#    foo-config. If neither is available, assume that no special flags are
#    needed (and guess a library later if necessary).
#
#  * If static linking to the backend has been requested, we have to find out
#    any needed linker flags, too.
#
# ToDo: There is far too much cut-n-paste below, we should really refactor this
# using some nice macros...


# OPENAL_ENABLE(FEATURE-NAME, SUPPORT-VAR, SUPPORT-DEFINE, SUPPORT-MAKEVAR)
#--------------------------------------------------------------------------
AC_DEFUN([OPENAL_ENABLE],
[AC_MSG_CHECKING([for $1 support])
 AC_MSG_RESULT([[$]$2])

 case [$]$2 in
   dynamic|static) openal_support_yn=yes ;;
   *) openal_support_yn=no ;;
 esac

 if test "x$openal_support_yn" = xyes; then
   AC_DEFINE([$3], [1], [Define to 1 if the $1 should be used.])
 fi

 AM_CONDITIONAL([$4], [test "x$openal_support_yn" = xyes])
]) # OPENAL_ENABLE


# check for ALSA backend #######################################################

AC_ARG_ENABLE([alsa],
[AS_HELP_STRING([--enable-alsa],
                [enable ALSA backend @<:@default=auto@:>@])])

AC_ARG_ENABLE([alsa-dlopen],
[AS_HELP_STRING([--enable-alsa-dlopen],
                [dynamically load ALSA libs @<:@default=yes@:>@])])

if test "x$enable_alsa" = xno; then
  openal_backend_alsa_support=disabled
else
  AC_CACHE_CHECK([for ALSA compiler flags], [openal_cv_cflags_alsa],
    [openal_cv_cflags_alsa=unknown
     test "x$PKG_CONFIG" != xnone && "$PKG_CONFIG" --exists alsa && openal_cv_cflags_alsa="`$PKG_CONFIG --cflags alsa`"
     test -z "$openal_cv_cflags_alsa" && openal_cv_cflags_alsa=none])
  case $openal_cv_cflags_alsa in
    unknown|none) openal_cflags_alsa="" ;;
    *) openal_cflags_alsa=$openal_cv_cflags_alsa ;;
  esac
  openal_saved_CFLAGS=$CFLAGS
  CFLAGS="$CFLAGS $openal_cflags_alsa"
  AC_CHECK_HEADER([alsa/asoundlib.h],
    [if test "x$enable_alsa_dlopen" != xno; then
       openal_backend_alsa_support=dynamic
       FEATURECFLAGS="$FEATURECFLAGS $openal_cflags_alsa"
       AC_DEFINE([OPENAL_DLOPEN_ALSA], [1], [dynamically load ALSA libs])
     else
       AC_CACHE_CHECK([for ALSA linker flags], [openal_cv_libs_alsa],
         [if test "x$PKG_CONFIG" != xnone && "$PKG_CONFIG" --exists alsa; then
            openal_cv_libs_alsa="`$PKG_CONFIG --libs alsa`"
            if test -z "$openal_cv_libs_alsa"; then
              openal_cv_libs_alsa=none
            fi
          else
            openal_cv_libs_alsa=unknown
          fi])
       if test "x$openal_cv_libs_alsa" = xunknown; then
         openal_libs_alsa="-lasound"
       else
         openal_libs_alsa=$openal_cv_libs_alsa
       fi
       openal_saved_LIBS=$LIBS
       LIBS="$openal_libs_alsa $LIBS"
       AC_CACHE_CHECK([for snd_pcm_info_sizeof], [openal_cv_func_snd_pcm_info_sizeof],
         [AC_LINK_IFELSE([AC_LANG_CALL([], [snd_pcm_info_sizeof])],
            [openal_cv_func_snd_pcm_info_sizeof=yes],
            [openal_cv_func_snd_pcm_info_sizeof=no])])
       if test "x$openal_cv_func_snd_pcm_info_sizeof" = xyes; then
         openal_backend_alsa_support=static
         FEATURECFLAGS="$FEATURECFLAGS $openal_cflags_alsa"
         LIBS="$openal_libs_alsa $LIBS"
       else
         openal_backend_alsa_support=none
         LIBS=$openal_saved_LIBS
         if test "x$enable_alsa" = xyes; then
           AC_MSG_WARN([--enable-alsa ignored because the ALSA library could not be found.])
         fi
       fi
     fi],
    [openal_backend_alsa_support=none
     if test "x$enable_alsa" = xyes; then
       AC_MSG_WARN([--enable-alsa ignored because the ALSA header could not be found.])
     fi],
    [AC_INCLUDES_DEFAULT([])])
  CFLAGS=$openal_saved_CFLAGS
fi

OPENAL_ENABLE([ALSA backend], [openal_backend_alsa_support],
              [USE_BACKEND_ALSA], [USEBACKENDALSA])

# check for aRts backend #######################################################

AC_ARG_ENABLE([arts],
[AS_HELP_STRING([--enable-arts],
                [enable aRts backend @<:@default=auto@:>@])])

AC_ARG_ENABLE([arts-dlopen],
[AS_HELP_STRING([--enable-arts-dlopen],
                [dynamically load aRts libs @<:@default=yes@:>@])])

if test "x$enable_arts" = xno; then
  openal_backend_arts_support=disabled
else
  AC_ARG_VAR([ARTSC_CONFIG], [aRts configuration utility])
  AC_CHECK_PROGS([ARTSC_CONFIG], [artsc-config], [none])
  AC_CACHE_CHECK([for aRts compiler flags], [openal_cv_cflags_arts],
    [openal_cv_cflags_arts=unknown
     test "x$ARTSC_CONFIG" != xnone && openal_cv_cflags_arts="`$ARTSC_CONFIG --cflags`"
     test -z "$openal_cv_cflags_arts" && openal_cv_cflags_arts=none])
  case $openal_cv_cflags_arts in
    unknown|none) openal_cflags_arts="" ;;
    *) openal_cflags_arts=$openal_cv_cflags_arts ;;
  esac
  openal_saved_CFLAGS=$CFLAGS
  CFLAGS="$CFLAGS $openal_cflags_arts"
  AC_CHECK_HEADER([artsc.h],
    [if test "x$enable_arts_dlopen" != xno; then
       openal_backend_arts_support=dynamic
       FEATURECFLAGS="$FEATURECFLAGS $openal_cflags_arts"
       AC_DEFINE([OPENAL_DLOPEN_ARTS], [1], [dynamically load aRts libs])
     else
       AC_CACHE_CHECK([for aRts linker flags], [openal_cv_libs_arts],
         [if test "x$ARTSC_CONFIG" != xnone; then
            openal_cv_libs_arts="`$ARTSC_CONFIG --libs`"
            if test -z "$openal_cv_libs_arts"; then
              openal_cv_libs_arts=none
            fi
          else
            openal_cv_libs_arts=unknown
          fi])
       if test "x$openal_cv_libs_arts" = xunknown; then
         openal_libs_arts="-lartsc"
       else
         openal_libs_arts=$openal_cv_libs_arts
       fi
       openal_saved_LIBS=$LIBS
       LIBS="$openal_libs_arts $LIBS"
       AC_CACHE_CHECK([for arts_free], [openal_cv_func_arts_free],
         [AC_LINK_IFELSE([AC_LANG_CALL([], [arts_free])],
            [openal_cv_func_arts_free=yes],
            [openal_cv_func_arts_free=no])])
       if test "x$openal_cv_func_arts_free" = xyes; then
         openal_backend_arts_support=static
         FEATURECFLAGS="$FEATURECFLAGS $openal_cflags_arts"
         LIBS="$openal_libs_arts $LIBS"
       else
         openal_backend_arts_support=none
         LIBS=$openal_saved_LIBS
         if test "x$enable_arts" = xyes; then
           AC_MSG_WARN([--enable-arts ignored because the aRts library could not be found.])
         fi
       fi
     fi],
    [openal_backend_arts_support=none
     if test "x$enable_arts" = xyes; then
       AC_MSG_WARN([--enable-arts ignored because the aRts header could not be found.])
     fi],
    [AC_INCLUDES_DEFAULT([])])
  CFLAGS=$openal_saved_CFLAGS
fi

OPENAL_ENABLE([aRts backend], [openal_backend_arts_support],
              [USE_BACKEND_ARTS], [USEBACKENDARTS])

# check for native BSD/OSS backend #############################################

# ToDo: Hmmm, OpenBSD/FreeBSD are using OSS, too, just like Linux. Perhaps some
# re-use is possible?

AC_ARG_ENABLE([bsd],
[AS_HELP_STRING([--enable-bsd],
                [enable native BSD/OSS backend @<:@default=auto@:>@])])

if test "x$enable_bsd" = xno; then
  openal_backend_bsd_support=disabled
else
  case "$target" in
    *bsd*) openal_backend_bsd_support=static ;;
    *) openal_backend_bsd_support=none
       if test "x$enable_bsd" = xyes; then
         AC_MSG_WARN([--enable-bsd ignored because this is no BSD build.])
       fi ;;
  esac
fi

OPENAL_ENABLE([native BSD/OSS backend], [openal_backend_bsd_support],
              [USE_BACKEND_NATIVE_BSD], [USEBACKENDNATIVEBSD])

# check for native Darwin backend ##############################################

AC_ARG_ENABLE([darwin],
[AS_HELP_STRING([--enable-darwin],
                [enable native Darwin backend @<:@default=auto@:>@])])

if test "x$enable_darwin" = xno; then
  openal_backend_darwin_support=disabled
else
  case "$target" in
    *darwin*) openal_backend_darwin_support=static ;;
    *) openal_backend_darwin_support=none
       if test "x$enable_darwin" = xyes; then
         AC_MSG_WARN([--enable-darwin ignored because this is no Darwin build.])
       fi ;;
  esac
fi

OPENAL_ENABLE([native Darwin backend], [openal_backend_darwin_support],
              [USE_BACKEND_NATIVE_DARWIN], [USEBACKENDNATIVEDARWIN])

# check for ESD backend ########################################################

AC_ARG_ENABLE([esd],
[AS_HELP_STRING([--enable-esd],
                [enable ESD backend @<:@default=auto@:>@])])

AC_ARG_ENABLE([esd-dlopen],
[AS_HELP_STRING([--enable-esd-dlopen],
                [dynamically load ESD libs @<:@default=yes@:>@])])

if test "x$enable_esd" = xno; then
  openal_backend_esd_support=disabled
else
  AC_ARG_VAR([ESD_CONFIG], [esd configuration utility])
  AC_CHECK_PROGS([ESD_CONFIG], [esd-config], [none])
  AC_CACHE_CHECK([for ESD compiler flags], [openal_cv_cflags_esd],
    [openal_cv_cflags_esd=unknown
     test "x$ESD_CONFIG" != xnone && openal_cv_cflags_esd="`$ESD_CONFIG --cflags`"
     test -z "$openal_cv_cflags_esd" && openal_cv_cflags_esd=none])
  case $openal_cv_cflags_esd in
    unknown|none) openal_cv_cflags_esd="" ;;
    *) openal_cflags_esd=$openal_cv_cflags_esd ;;
  esac
  openal_saved_CFLAGS=$CFLAGS
  CFLAGS="$CFLAGS $openal_cflags_esd"
  AC_CHECK_HEADER([esd.h],
    [if test "x$enable_esd_dlopen" != xno; then
       openal_backend_esd_support=dynamic
       FEATURECFLAGS="$FEATURECFLAGS $openal_cflags_esd"
       AC_DEFINE([OPENAL_DLOPEN_ESD], [1], [dynamically load ESD libs])
     else
       AC_CACHE_CHECK([for ESD linker flags], [openal_cv_libs_esd],
         [if test "x$ESD_CONFIG" != xnone; then
            openal_cv_libs_esd="`$ESD_CONFIG --libs`"
            if test -z "$openal_cv_libs_esd"; then
              openal_cv_libs_esd=none
            fi
          else
            openal_cv_libs_esd=unknown
          fi])
       if test "x$openal_cv_libs_esd" = xunknown; then
         openal_libs_esd="-lesd"
       else
         openal_libs_esd=$openal_cv_libs_esd
       fi
       openal_saved_LIBS=$LIBS
       LIBS="$openal_libs_esd $LIBS"
       AC_CACHE_CHECK([for esd_audio_open], [openal_cv_func_esd_audio_open],
         [AC_LINK_IFELSE([AC_LANG_CALL([], [esd_audio_open])],
            [openal_cv_func_esd_audio_open=yes],
            [openal_cv_func_esd_audio_open=no])])
       if test "x$openal_cv_func_esd_audio_open" = xyes; then
         openal_backend_esd_support=static
         FEATURECFLAGS="$FEATURECFLAGS $openal_cflags_esd"
         LIBS="$openal_libs_esd $LIBS"
       else
         openal_backend_esd_support=none
         LIBS=$openal_saved_LIBS
         if test "x$enable_esd" = xyes; then
           AC_MSG_WARN([--enable-esd ignored because the ESD library could not be found.])
         fi
       fi
     fi],
    [openal_backend_esd_support=none
     if test "x$enable_esd" = xyes; then
       AC_MSG_WARN([--enable-esd ignored because the ESD header could not be found.])
     fi],
    [AC_INCLUDES_DEFAULT([])])
  CFLAGS=$openal_saved_CFLAGS
fi

OPENAL_ENABLE([ESD backend], [openal_backend_esd_support],
              [USE_BACKEND_ESD], [USEBACKENDESD])

# check for native IRIX backend ################################################

AC_ARG_ENABLE([irix],
[AS_HELP_STRING([--enable-irix],
                [enable native IRIX backend @<:@default=auto@:>@])])

if test "x$enable_irix" = xno; then
  openal_backend_irix_support=disabled
else
  AC_CHECK_HEADER([dmedia/audio.h],
    [openal_libs_irix="-laudio"
     openal_saved_LIBS=$LIBS
     LIBS="$openal_libs_irix $LIBS"
     AC_CACHE_CHECK([for alGetResourceByName], [openal_cv_func_algetresourcebyname],
       [AC_LINK_IFELSE([AC_LANG_CALL([], [alGetResourceByName])],
          [openal_cv_func_algetresourcebyname=yes],
          [openal_cv_func_algetresourcebyname=no])])
     if test "x$openal_cv_func_algetresourcebyname" = xyes; then
       openal_backend_irix_support=static
       LIBS="$openal_libs_irix $LIBS"
     else
       openal_backend_irix_support=none
       LIBS=$openal_saved_LIBS
       if test "x$enable_irix" = xyes; then
         AC_MSG_WARN([--enable-irix ignored because the Irix Multimedia library could not be found.])
       fi
     fi],
    [openal_backend_irix_support=none
     if test "x$enable_irix" = xyes; then
       AC_MSG_WARN([--enable-irix ignored because the Irix header could not be found.])
     fi],
    [AC_INCLUDES_DEFAULT([])])
fi

OPENAL_ENABLE([native IRIX backend], [openal_backend_irix_support],
              [USE_BACKEND_DMEDIA], [USEBACKENDDMEDIA])

# check for native Linux backend ###############################################

AC_ARG_ENABLE([linux],
[AS_HELP_STRING([--enable-linux],
                [enable native Linux/OSS backend @<:@default=auto@:>@])])

if test "x$enable_linux" = xno; then
  openal_backend_linux_support=disabled
else
  AC_CHECK_HEADER([linux/soundcard.h],
    [openal_backend_linux_support=static],
    [openal_backend_linux_support=none
     if test "x$enable_linux" = xyes; then
       AC_MSG_WARN([--enable-linux ignored because the Linux OSS header could not be found.])
     fi],
    [AC_INCLUDES_DEFAULT([])])
fi

OPENAL_ENABLE([native Linux backend], [openal_backend_linux_support],
              [USE_BACKEND_NATIVE_LINUX], [USEBACKENDNATIVELINUX])

# check for native MorphOS backend #############################################

AC_ARG_ENABLE([morphos],
[AS_HELP_STRING([--enable-morphos],
                [enable native MorphOS backend @<:@default=auto@:>@])])

if test "x$enable_morphos" = xno; then
  openal_backend_morphos_support=disabled
else
  case "$target" in
    *morphos*) openal_backend_morphos_support=static ;;
    *) openal_backend_morphos_support=none
       if test "x$enable_morphos" = xyes; then
         AC_MSG_WARN([--enable-morphos ignored because this is no MorphOS build.])
       fi ;;
  esac
fi

OPENAL_ENABLE([native MorphOS backend], [openal_backend_morphos_support],
              [USE_BACKEND_NATIVE_MORPHOS], [USEBACKENDNATIVEMORPHOS])

# check for null backend #######################################################

AC_ARG_ENABLE([null],
[AS_HELP_STRING([--enable-null],
                [enable null backend @<:@default=yes@:>@])])

case $enable_null in
  no) openal_backend_null_support=disabled ;;
  *) openal_backend_null_support=static ;;
esac

OPENAL_ENABLE([null backend], [openal_backend_null_support],
              [USE_BACKEND_NULL], [USEBACKENDNULL])

# check for SDL backend ########################################################

AC_ARG_ENABLE([sdl],
[AS_HELP_STRING([--enable-sdl],
                [enable SDL backend @<:@default=auto@:>@])])

AC_ARG_ENABLE([sdl-dlopen],
[AS_HELP_STRING([--enable-sdl-dlopen],
                [dynamically load SDL libs @<:@default=yes@:>@])])

if test "x$enable_sdl" = xno; then
  openal_backend_sdl_support=disabled
else
  AC_ARG_VAR([SDL_CONFIG], [sdl configuration utility])
  AC_CHECK_PROGS([SDL_CONFIG], [sdl-config], [none])
  AC_CACHE_CHECK([for SDL compiler flags], [openal_cv_cflags_sdl],
    [openal_cv_cflags_sdl=unknown
     test "x$SDL_CONFIG" != xnone && openal_cv_cflags_sdl="`$SDL_CONFIG --cflags`"
     test -z "$openal_cv_cflags_sdl" && openal_cv_cflags_sdl=none])
  case $openal_cv_cflags_sdl in
    unknown|none) openal_cv_cflags_sdl="" ;;
    *) openal_cflags_sdl=$openal_cv_cflags_sdl ;;
  esac
  openal_saved_CFLAGS=$CFLAGS
  CFLAGS="$CFLAGS $openal_cflags_sdl"
  AC_CHECK_HEADER([SDL.h],
    [if test "x$enable_sdl_dlopen" != xno; then
       openal_backend_sdl_support=dynamic
       FEATURECFLAGS="$FEATURECFLAGS $openal_cflags_sdl"
       AC_DEFINE([OPENAL_DLOPEN_SDL], [1], [dynamically load SDL libs])
     else
       AC_CACHE_CHECK([for SDL linker flags], [openal_cv_libs_sdl],
         [if test "x$SDL_CONFIG" != xnone; then
            openal_cv_libs_sdl="`$SDL_CONFIG --libs`"
            if test -z "$openal_cv_libs_sdl"; then
              openal_cv_libs_sdl=none
            fi
          else
            openal_cv_libs_sdl=unknown
          fi])
       if test "x$openal_cv_libs_sdl" = xunknown; then
         openal_libs_sdl="-lsdl"
       else
         openal_libs_sdl=$openal_cv_libs_sdl
       fi
       openal_saved_LIBS=$LIBS
       LIBS="$openal_libs_sdl $LIBS"
       AC_CACHE_CHECK([for SDL_LockAudio], [openal_cv_func_sdl_lockaudio],
         [AC_LINK_IFELSE([AC_LANG_CALL([], [SDL_LockAudio])],
            [openal_cv_func_sdl_lockaudio=yes],
            [openal_cv_func_sdl_lockaudio=no])])
       if test "x$openal_cv_func_sdl_lockaudio" = xyes; then
         openal_backend_sdl_support=static
         FEATURECFLAGS="$FEATURECFLAGS $openal_cflags_sdl"
         LIBS="$openal_libs_sdl $LIBS"
       else
         openal_backend_sdl_support=none
         LIBS=$openal_saved_LIBS
         if test "x$enable_sdl" = xyes; then
           AC_MSG_WARN([--enable-sdl ignored because the SDL library could not be found.])
         fi
       fi
     fi],
    [openal_backend_sdl_support=none
     if test "x$enable_sdl" = xyes; then
       AC_MSG_WARN([--enable-sdl ignored because the SDL header could not be found.])
     fi],
    [AC_INCLUDES_DEFAULT([])])
  CFLAGS=$openal_saved_CFLAGS
fi

OPENAL_ENABLE([SDL backend], [openal_backend_sdl_support],
              [USE_BACKEND_SDL], [USEBACKENDSDL])

# check for native Solaris backend #############################################

AC_ARG_ENABLE([solaris],
[AS_HELP_STRING([--enable-solaris],
                [enable native Solaris backend @<:@default=auto@:>@])])

if test "x$enable_solaris" = xno; then
  openal_backend_solaris_support=disabled
else
  AC_CHECK_HEADER([sys/audioio.h],
    [openal_backend_solaris_support=static],
    [openal_backend_solaris_support=none
     if test "x$enable_solaris" = xyes; then
       AC_MSG_WARN([--enable-solaris ignored because the Solaris audio header could not be found.])
     fi],
    [AC_INCLUDES_DEFAULT([])])
fi

OPENAL_ENABLE([native Solaris backend], [openal_backend_solaris_support],
              [USE_BACKEND_NATIVE_SOLARIS], [USEBACKENDNATIVESOLARIS])

# check for WAVE backend #######################################################

AC_ARG_ENABLE([waveout],
[AS_HELP_STRING([--enable-waveout],
                [enable WAVE backend @<:@default=yes@:>@])])

case $enable_waveout in
  no) openal_backend_waveout_support=disabled ;;
  *) openal_backend_waveout_support=static ;;
esac

OPENAL_ENABLE([WAVE backend], [openal_backend_waveout_support],
              [USE_BACKEND_WAVEOUT], [USEBACKENDWAVEOUT])

# check for native Windows backend #############################################

AC_ARG_ENABLE([windows],
[AS_HELP_STRING([--enable-windows],
                [enable native Windows backend @<:@default=auto@:>@])])

if test "x$enable_windows" = xno; then
  openal_backend_windows_support=disabled
else
  AC_CHECK_HEADER([windows.h],
    [openal_libs_windows="-lwinmm"
     openal_saved_LIBS=$LIBS
     LIBS="$openal_libs_windows $LIBS"
     AC_CACHE_CHECK([for waveOutOpen], [openal_cv_func_waveoutopen],
       [AC_LINK_IFELSE(
         [AC_LANG_PROGRAM([[@%:@include <windows.h>]],
                          [[waveOutOpen(NULL, WAVE_MAPPER, NULL, 0, 0, CALLBACK_FUNCTION);]])],
          [openal_cv_func_waveoutopen=yes],
          [openal_cv_func_waveoutopen=no])])
     if test "x$openal_cv_func_waveoutopen" = xyes; then
       openal_backend_windows_support=static
       LIBS="$openal_libs_windows $LIBS"
     else
       openal_backend_windows_support=none
       LIBS=$openal_saved_LIBS
       if test "x$enable_windows" = xyes; then
         AC_MSG_WARN([--enable-windows ignored because the Windows Multimedia library could not be found.])
       fi
     fi],
    [openal_backend_windows_support=none
     if test "x$enable_windows" = xyes; then
       AC_MSG_WARN([--enable-windows ignored because the Windows header could not be found.])
     fi],
    [AC_INCLUDES_DEFAULT([])])
fi

OPENAL_ENABLE([native Windows backend], [openal_backend_windows_support],
              [USE_BACKEND_NATIVE_WINDOWS], [USEBACKENDNATIVEWINDOWS])

################################################################################
# Checks for extensions.
################################################################################

# check for AL_EXT_vorbis extension ############################################

AC_ARG_ENABLE([vorbis],
[AS_HELP_STRING([--enable-vorbis],
                [enable AL_EXT_vorbis extension @<:@default=auto@:>@])])

AC_ARG_ENABLE([vorbis-dlopen],
[AS_HELP_STRING([--enable-vorbis-dlopen],
                [dynamically load Vorbis libs @<:@default=yes@:>@])])

if test "x$enable_vorbis" = xno; then
  openal_extension_vorbis_support=disabled
else
  AC_CACHE_CHECK([for Vorbis compiler flags], [openal_cv_cflags_vorbis],
    [openal_cv_cflags_vorbis=unknown
     test "x$PKG_CONFIG" != xnone && "$PKG_CONFIG" --exists vorbis && openal_cv_cflags_vorbis="`$PKG_CONFIG --cflags vorbisfile`"
     test -z "$openal_cv_cflags_vorbis" && openal_cv_cflags_vorbis=none])
  case $openal_cv_cflags_vorbis in
    unknown|none) openal_cflags_vorbis="" ;;
    *) openal_cflags_vorbis=$openal_cv_cflags_vorbis ;;
  esac
  openal_saved_CFLAGS=$CFLAGS
  CFLAGS="$CFLAGS $openal_cflags_vorbis"
  AC_CHECK_HEADER([vorbis/vorbisfile.h],
    [if test "x$enable_vorbis_dlopen" != xno; then
       openal_extension_vorbis_support=dynamic
       FEATURECFLAGS="$FEATURECFLAGS $openal_cflags_vorbis"
       AC_DEFINE([OPENAL_DLOPEN_VORBIS], [1], [dynamically load Vorbis libs])
     else
       AC_CACHE_CHECK([for Vorbis linker flags], [openal_cv_libs_vorbis],
         [if test "x$PKG_CONFIG" != xnone && "$PKG_CONFIG" --exists vorbis; then
            openal_cv_libs_vorbis="`$PKG_CONFIG --libs vorbisfile`"
            if test -z "$openal_cv_libs_vorbis"; then
              openal_cv_libs_vorbis=none
            fi
          else
            openal_cv_libs_vorbis=unknown
          fi])
       if test "x$openal_cv_libs_vorbis" = xunknown; then
         openal_libs_vorbis="-lvorbisfile -lvorbis -logg"
       else
         openal_libs_vorbis=$openal_cv_libs_vorbis
       fi
       openal_saved_LIBS=$LIBS
       LIBS="$openal_libs_vorbis $LIBS"
       AC_CACHE_CHECK([for ov_clear], [openal_cv_func_ov_clear],
         [AC_LINK_IFELSE([AC_LANG_CALL([], [ov_clear])],
            [openal_cv_func_ov_clear=yes],
            [openal_cv_func_ov_clear=no])])
       if test "x$openal_cv_func_ov_clear" = xyes; then
         openal_extension_vorbis_support=static
         FEATURECFLAGS="$FEATURECFLAGS $openal_cflags_vorbis"
         LIBS="$openal_libs_vorbis $LIBS"
       else
         openal_extension_vorbis_support=none
         LIBS=$openal_saved_LIBS
         if test "x$enable_vorbis" = xyes; then
           AC_MSG_WARN([--enable-vorbis ignored because the Vorbis library could not be found.])
         fi
       fi
     fi],
    [openal_extension_vorbis_support=none
     if test "x$enable_vorbis" = xyes; then
       AC_MSG_WARN([--enable-vorbis ignored because the Vorbis header could not be found.])
     fi],
    [AC_INCLUDES_DEFAULT([])])
  CFLAGS=$openal_saved_CFLAGS
fi

OPENAL_ENABLE([AL_EXT_vorbis supprt], [openal_extension_vorbis_support],
              [ENABLE_EXTENSION_AL_EXT_VORBIS], [ENABLEEXTENSIONALEXTVORBIS])

# check for AL_EXT_mp3 extension ###############################################

AC_ARG_ENABLE([mp3],
[AS_HELP_STRING([--enable-mp3],
                [enable AL_EXT_mp3 support @<:@default=auto@:>@])])

AC_ARG_ENABLE([mp3-dlopen],
[AS_HELP_STRING([--enable-mp3-dlopen],
                [dynamically load MP3 libs @<:@default=yes@:>@])])

# For legacy reasons only, this misnomer will be nuked some day.
AC_ARG_ENABLE([smpeg],
[AS_HELP_STRING([--enable-smpeg],
                [deprecated, use --enable-mp3 instead])],
[AC_MSG_WARN([DO NOT USE --enable-smpeg ANYMORE, IT WILL BE REMOVED SOON. USE --enable-mp3 INSTEAD.])
 enable_mp3=$enable_smpeg])

if test "x$enable_mp3" = xno; then
  openal_extension_smpeg_support=disabled
else
  AC_ARG_VAR([SMPEG_CONFIG], [smpeg configuration utility])
  AC_CHECK_PROGS([SMPEG_CONFIG], [smpeg-config], [none])
  AC_CACHE_CHECK([for SMPEG compiler flags], [openal_cv_cflags_smpeg],
    [openal_cv_cflags_smpeg=unknown
     test "x$SMPEG_CONFIG" != xnone && openal_cv_cflags_smpeg="`$SMPEG_CONFIG --cflags`"
     test -z "$openal_cv_cflags_smpeg" && openal_cv_cflags_smpeg=none])
  case $openal_cv_cflags_smpeg in
    unknown|none) openal_cv_cflags_smpeg="" ;;
    *) openal_cflags_smpeg=$openal_cv_cflags_smpeg ;;
  esac
  openal_saved_CFLAGS=$CFLAGS
  CFLAGS="$CFLAGS $openal_cflags_smpeg"
  AC_CHECK_HEADER([smpeg.h],
    [if test "x$enable_mp3_dlopen" != xno; then
       openal_extension_smpeg_support=dynamic
       FEATURECFLAGS="$FEATURECFLAGS $openal_cflags_smpeg"
       AC_DEFINE([OPENAL_DLOPEN_MP3], [1], [dynamically load SMPEG libs])
     else
       AC_CACHE_CHECK([for SMPEG linker flags], [openal_cv_libs_smpeg],
         [if test "x$SMPEG_CONFIG" != xnone; then
            openal_cv_libs_smpeg="`$SMPEG_CONFIG --libs`"
            if test -z "$openal_cv_libs_smpeg"; then
              openal_cv_libs_smpeg=none
            fi
          else
            openal_cv_libs_smpeg=unknown
          fi])
       if test "x$openal_cv_libs_smpeg" = xunknown; then
         openal_libs_smpeg="-lsmpeg"
       else
         openal_libs_smpeg=$openal_cv_libs_smpeg
       fi
       openal_saved_LIBS=$LIBS
       LIBS="$openal_libs_smpeg $LIBS"
       AC_CACHE_CHECK([for SMPEG_play], [openal_cv_func_SMPEG_play],
         [AC_LINK_IFELSE([AC_LANG_CALL([], [SMPEG_play])],
            [openal_cv_func_SMPEG_play=yes],
            [openal_cv_func_SMPEG_play=no])])
       if test "x$openal_cv_func_SMPEG_play" = xyes; then
         openal_extension_smpeg_support=static
         FEATURECFLAGS="$FEATURECFLAGS $openal_cflags_smpeg"
         LIBS="$openal_libs_smpeg $LIBS"
       else
         openal_extension_smpeg_support=none
         LIBS=$openal_saved_LIBS
         if test "x$enable_mp3" = xyes; then
           AC_MSG_WARN([--enable-smpeg ignored because the SMPEG library could not be found.])
         fi
       fi
     fi],
    [openal_extension_smpeg_support=none
     if test "x$enable_mp3" = xyes; then
       AC_MSG_WARN([--enable-smpeg ignored because the SMPEG header could not be found.])
     fi],
    [AC_INCLUDES_DEFAULT([])])
  CFLAGS=$openal_saved_CFLAGS
fi

OPENAL_ENABLE([AL_EXT_mp3 extension], [openal_extension_smpeg_support],
              [ENABLE_EXTENSION_AL_EXT_MP3], [ENABLEEXTENSIONALEXTMP3])

################################################################################
# Misc stuff
################################################################################

AC_CHECK_SIZEOF([void *])

AC_DEFINE([AL_SUPPORT_BYTE_LOKI_SOURCE_ATTR_], [1], [Define to 1 if the AL_BYTE_LOKI source attribute shall be supported.])

AC_SUBST([FEATURECFLAGS])
AC_SUBST([LIBS])
AC_SUBST([NASM])
AC_SUBST([NASM_FORMAT])
AC_SUBST([OPTIMIZATIONCFLAGS])
AC_SUBST([PROFILINGLDFLAGS])
AC_SUBST([RANLIB])
AC_SUBST([SIZEOF_VOID_P])
AC_SUBST([WARNINGCFLAGS])

# Generate output.
# NOTE: Do not break the following line, otherwise we are into CR/LF vs. LF
# trouble! This is a buglet in autoconf IMHO, but easy to work around.
AC_CONFIG_FILES([Makefile admin/Makefile admin/pkgconfig/Makefile admin/pkgconfig/openal-config admin/pkgconfig/openal.pc common/Makefile common/include/Makefile include/Makefile src/Makefile src/arch/Makefile src/arch/i386/Makefile])

AC_CONFIG_COMMANDS([default], [chmod +x admin/pkgconfig/openal-config])
AC_OUTPUT
