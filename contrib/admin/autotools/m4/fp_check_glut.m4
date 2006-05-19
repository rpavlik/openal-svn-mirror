# FP_CHECK_GLUT
# -------------
AC_DEFUN([FP_CHECK_GLUT],
[AC_REQUIRE([FP_CHECK_GLU])
AC_REQUIRE([FP_CHECK_QUARTZ_OPENGL])

if test x"$use_quartz_opengl" = xno; then
  GLUT_CFLAGS="$GLU_CFLAGS0"
  GLUT_LIBS="$GLU_LIBS0"

  if test x"$no_x" != xyes; then
    GLUT_LIBS="$X_PRE_LIBS -lXmu -lXi $X_EXTRA_LIBS $GLUT_LIBS"
  fi

  AC_CHECK_HEADERS([windows.h])

  fp_save_cppflags="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS $X_CFLAGS"
  AC_CHECK_HEADERS([GL/glut.h])
  CPPFLAGS="$fp_save_cppflags"

  # Note 1: On Cygwin with X11, GL/GLU functions use the "normal" calling
  # convention, but GLUT functions use stdcall. To get this right, it is
  # necessary to include <windows.h> first.
  # Note 2: MinGW/MSYS comes without a GLUT header, so we use Cygwin's one in
  # that case.
  FP_CHECK_GL_HELPER([GLUT], [-lglut32 -lglut], [
#if HAVE_WINDOWS_H
#include <windows.h>
#endif
#if HAVE_GL_GLUT_H
#include <GL/glut.h>
#else
#include "glut_local.h"
#endif
    ], [glutMainLoop()])
fi

AC_SUBST([GLUT_CFLAGS])
AC_SUBST([GLUT_LIBS])
])# FP_CHECK_GLUT
