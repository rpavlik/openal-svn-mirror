# FP_CHECK_GL
# -----------
AC_DEFUN([FP_CHECK_GL],
[AC_REQUIRE([FP_PATH_XTRA])
AC_REQUIRE([FP_CHECK_QUARTZ_OPENGL])
AC_REQUIRE([FP_CHECK_WIN32])

if test x"$use_quartz_opengl" = xno; then
  AC_CHECK_FUNC(atan,[fp_libm_not_needed=yes],[fp_libm_not_needed=dunno])
  if test x"$fp_libm_not_needed" = xdunno; then
     AC_CHECK_LIB([m], [atan], [GL_LIBS="-lm $GL_LIBS"])
  fi

  if test x"$no_x" != xyes; then
    test -n "$x_includes" && GL_CFLAGS="-I$x_includes $GL_CFLAGS"
    test -n "$x_libraries" && GL_LIBS="-L$x_libraries -lX11 $GL_LIBS"
  fi

  FP_CHECK_GL_HELPER([GL], [-lGL -lopengl32], [@%:@include <GL/gl.h>], [glEnd()])

  if test x"$fp_is_win32" = xyes; then
    # Ugly: To get wglGetProcAddress on Windows, we have to link with
    # opengl32.dll, too, even when we are using Cygwin with X11.
    case "$GL_LIBS" in
      *-lopengl32*|*opengl32.lib*) ;;
      *) fp_save_LIBS="$LIBS"
         LIBS="$LIBS -lopengl32"
         AC_LINK_IFELSE([AC_LANG_PROGRAM([[@%:@include <GL/gl.h>]], [[glEnd()]])],
           [GL_LIBS="$GL_LIBS -lopengl32"; GL_LIBS0="$GL_LIBS0 -lopengl32"])
         LIBS="$fp_save_LIBS"
         ;;
    esac
  fi
fi
AC_SUBST([GL_CFLAGS])
AC_SUBST([GL_LIBS])
])# FP_CHECK_GL
