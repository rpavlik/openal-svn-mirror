# FP_CHECK_QUARTZ_OPENGL
# ----------------------
AC_DEFUN([FP_CHECK_QUARTZ_OPENGL],
[AC_REQUIRE([FP_ARG_OPENGL])
AC_REQUIRE([AC_CANONICAL_TARGET])

use_quartz_opengl=no
if test x"$enable_opengl" = xyes; then
  case $target_os in
  darwin*)
    AC_DEFINE([USE_QUARTZ_OPENGL], [1],
              [Define to 1 if native OpenGL should be used on Mac OS X])
    use_quartz_opengl=yes
    ;;
  esac
fi

GLU_FRAMEWORKS=
GLUT_FRAMEWORKS=
if test x"$use_quartz_opengl" = xyes; then
  GLU_FRAMEWORKS=OpenGL
  GLUT_FRAMEWORKS=GLUT
fi
AC_SUBST([GLU_FRAMEWORKS])
AC_SUBST([GLUT_FRAMEWORKS])
])# FP_CHECK_QUARTZ_OPENGL
