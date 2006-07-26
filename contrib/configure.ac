################################################################################
# Process this file with autoconf to produce a configure script.
################################################################################

AC_INIT([Contributed SW for OpenAL], [1.0], [openal-devel@opensource.creative.com], [openal-contrib])
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

AC_PROG_CC
AC_C_CONST
AC_EXEEXT
ALUT_C__ATTRIBUTE__

AC_ARG_VAR([PKG_CONFIG], [package configuration utility])
AC_CHECK_PROGS([PKG_CONFIG], [pkg-config], [none])

################################################################################
# Checks for library functions.
################################################################################

AC_CHECK_LIBM
AC_SUBST([LIBM])

if test "x$PKG_CONFIG" != xnone && "$PKG_CONFIG" --exists openal; then
  AM_CFLAGS="`$PKG_CONFIG --cflags openal` $AM_CFLAGS"
  LIBS="`$PKG_CONFIG --libs openal` $LIBS"
else
  AC_SEARCH_LIBS([alGetError], [openal32 openal])
fi

if test "x$PKG_CONFIG" != xnone && "$PKG_CONFIG" --exists alut; then
  AM_CFLAGS="`$PKG_CONFIG --cflags alut` $AM_CFLAGS"
  LIBS="`$PKG_CONFIG --libs alut` $LIBS"
else
  AC_SEARCH_LIBS([alutExit], [alut])
fi


# TODO: This is an evil hack, OpenGL support under Cygwin seems to work without
# X only currently.
if test "x$target_os" = xcygwin; then
  no_x="yes"
fi

enable_opengl="yes"
FP_CHECK_GLUT

################################################################################
# Checks for header files.
################################################################################

# We could possibly need struct timespec, which is not ANSI.
AC_DEFINE([_XOPEN_SOURCE], [500], [Define to 500 if Single Unix conformance is wanted, 600 for sixth revision.])

# We might need nanosleep, which is a POSIX IEEE Std 1003.1b-1993 feature.
AC_DEFINE([_POSIX_C_SOURCE], [199309], [Define to the POSIX version that should be used.])

# Without __NO_CTYPE tolower and friends are macros which introduce a GLIBC 2.3
# dependency. By defining this identifier we are currently backwards compatible
# to GLIBC 2.1.3, which is a good thing. In addition, the macros lead to code
# which triggers warnings with -Wunreachable-code.
AC_DEFINE([__NO_CTYPE], [1], [Define to 1 if tolower and friends should not be macros.])

AC_HEADER_STDC
AC_CHECK_HEADERS([conio.h time.h windows.h])

# Checks for library functions.
ALUT_CHECK_FUNC([[@%:@include <time.h>]],
                [nanosleep], [[((struct timespec*)0, (struct timespec*)0)]])

ALUT_CHECK_FUNC([[@%:@include <unistd.h>]],
                [usleep], [[(0)]])

ALUT_CHECK_FUNC([[@%:@include <windows.h>]],
                [Sleep], [[(0)]])

ALUT_CHECK_FUNC([[@%:@include <sys/types.h>
                  @%:@include <sys/stat.h>
                  @%:@include <unistd.h>]],
                [stat], [[("", (struct stat*)0)]])

ALUT_CHECK_FUNC([[@%:@include <sys/types.h>
                  @%:@include <sys/stat.h>]],
                [_stat], [[("", (struct _stat*)0)]])

################################################################################
# Handle warning-related options.
################################################################################

AC_ARG_ENABLE([warnings],
[AS_HELP_STRING([--enable-warnings],
                [enable pedantic compiler warnings @<:@default=yes@:>@])])

if test "x$enable_warnings" != xno; then
  # Doing it in two steps gives a nicer message...
  AX_CFLAGS_WARN_ALL_ANSI([flags])
  AM_CFLAGS="$AM_CFLAGS $flags"
fi

# ToDo: Handle C++ here, too.
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
    # Note that some older GCC versions give false positives about unreachable
    # code. Furthermore, using GNU's <string.h> header normally results in lots
    # of string functions like strcmp being macros which result in such false
    # positives, too. To disable those, one can use -D__NO_STRING_INLINES for
    # testing. And another source for false positives is inlining, so leaving
    # out -finline-functions or using -fno-inline-functions explicitly might
    # help, too.
    AM_CFLAGS="$AM_CFLAGS -W -Waggregate-return -Wbad-function-cast -Wcast-align -Wcast-qual -Wdisabled-optimization -Wendif-labels -Winline -Wlong-long -Wmissing-declarations -Wmissing-format-attribute -Wmissing-noreturn -Wmissing-prototypes -Wnested-externs -Wpacked -Wpointer-arith -Wredundant-decls -Wshadow -Wsign-compare -Wstrict-prototypes -Wundef -Wunreachable-code -Wwrite-strings"

    # Check for GCC 4.x-only warning options.
    ALUT_CHECK_FLAG([-Wdeclaration-after-statement],
                    [AM_CFLAGS="$AM_CFLAGS -Wdeclaration-after-statement"])
    ALUT_CHECK_FLAG([-Winvalid-pch],
                    [AM_CFLAGS="$AM_CFLAGS -Winvalid-pch"])
    ALUT_CHECK_FLAG([-Wmissing-field-initializers],
                    [AM_CFLAGS="$AM_CFLAGS -Wmissing-field-initializers"])
    # We cheat here a bit: The code generated by AC_LANG_PROGRAM triggers a
    # warning with -Wold-style-definition, so we assume that this flag is
    # supported whenever -Wvariadic-macros is.
    ALUT_CHECK_FLAG([-Wvariadic-macros],
                    [AM_CFLAGS="$AM_CFLAGS -Wvariadic-macros -Wold-style-definition"])
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
    AM_CFLAGS="$AM_CFLAGS -Werror"
  fi
fi

################################################################################
# Handle debugging-related options.
################################################################################

AC_ARG_ENABLE([debug],
[AS_HELP_STRING([--enable-debug],
                [enable debug code and assertions @<:@default=no@:>@])])

if test "x$enable_debug" = xyes; then
  AC_DEFINE([DEBUG], [1], [Define to 1 if you want to include debugging code.])
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

################################################################################
# Handle optimization-related options.
################################################################################

AC_ARG_ENABLE([optimization],
[AS_HELP_STRING([--enable-optimization],
                [enable optimization @<:@default=yes@:>@])])

if test "x$enable_optimization" = xyes; then
  ALUT_CHECK_FLAG([-finline-functions],
                  [AM_CFLAGS="$AM_CFLAGS -finline-functions"])
  ALUT_CHECK_FLAG([-ffast-math],
                  [AM_CFLAGS="$AM_CFLAGS -ffast-math"])
  # -pg and -fomit-frame-pointer are incompatible. ToDo: Test for this!
  if test "x$enable_profile" != xyes; then
    ALUT_CHECK_FLAG([-fomit-frame-pointer],
                    [AM_CFLAGS="$AM_CFLAGS -fomit-frame-pointer"])
  fi
fi

# profiling stuff
AC_ARG_ENABLE([profile],
[AS_HELP_STRING([--enable-profile],
                [enable profile @<:@default=no@:>@])])

if test "x$enable_profile" = xyes; then
  ALUT_CHECK_FLAG([-pg],
                  [AM_CFLAGS="$AM_CFLAGS -pg"
                   AM_LDFLAGS="$AM_LDFLAGS -pg"],
                  [AC_MSG_WARN([--enable-profile ignored because $CC does not support -pg.])])
fi

################################################################################
# Misc stuff
################################################################################

AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_LDFLAGS])
AC_SUBST([LIBS])

# Generate output.
# NOTE: Do not break the following line, otherwise we are into CR/LF vs. LF
# trouble! This is a buglet in autoconf IMHO, but easy to work around.
AC_CONFIG_FILES([Makefile demos/Makefile demos/panning2d/Makefile examples/Makefile examples/openal-info/Makefile tests/Makefile tests/altest/Makefile])

AC_OUTPUT
