# FP_PATH_XTRA
# ------------
# Same as AC_PATH_XTRA, but works even for broken Cygwins which try to include
# the non-existant <gl/mesa_wgl.h> header when -mno-cygwin is used.
AC_DEFUN([FP_PATH_XTRA],
[AC_REQUIRE([FP_CHECK_WIN32])
if test x"$fp_is_win32" = xyes; then
  no_x=yes
else
  AC_PATH_XTRA
fi
])# FP_PATH_XTRA
