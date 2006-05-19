# FP_CHECK_GLU
# ------------
AC_DEFUN([FP_CHECK_GLU],
[AC_REQUIRE([FP_CHECK_GL])dnl
GLU_CFLAGS="$GL_CFLAGS0"
GLU_LIBS="$GL_LIBS0"

if test x"$use_quartz_opengl" = xno; then
  FP_CHECK_GL_HELPER([GLU], [-lglu32 -lGLU], [@%:@include <GL/glu.h>], [gluNewQuadric()])
fi

AC_SUBST([GLU_CFLAGS])
AC_SUBST([GLU_LIBS])
])# FP_CHECK_GLU
