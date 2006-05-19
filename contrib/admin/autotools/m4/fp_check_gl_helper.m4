# FP_CHECK_GL_HELPER(LIBNAME, LIBS, INCLUDES, FUNCTION-BODY)
# ----------------------------------------------------------
# Try each library in LIBS to successfully link INCLUDES plus FUNCTION-BODY,
# setting LIBNAME_CFLAGS and LIBNAME_LIBS to the corresponding values. Sets
# no_LIBNAME to "yes" if no suitable library was found. (LIBNAME_CFLAGS0
# contains the value of LIBNAME_CFLAGS without CPPFLAGS, and LIBNAME_LIBS0
# contains the value of LIBNAME_LIBS without LDFLAGS, but these are only
# used internally.)
AC_DEFUN([FP_CHECK_GL_HELPER],
[AC_CACHE_CHECK([for $1 library], [fp_cv_check_$1_lib],
  [fp_cv_check_$1_lib="no"
  fp_save_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS ${$1_CFLAGS}"
  fp_save_LIBS="$LIBS"
  for fp_try_lib in $2; do
    # transform "-lfoo" to "foo.lib" when using cl
    if test x"$CC" = xcl; then
      fp_try_lib=`echo $fp_try_lib | sed -e 's/^-l//' -e 's/$/.lib/'`
    fi
    LIBS="$fp_try_lib ${$1_LIBS} $fp_save_LIBS"
    AC_LINK_IFELSE([AC_LANG_PROGRAM([$3], [$4])], [fp_cv_check_$1_lib="$fp_try_lib ${$1_LIBS}"; break])
  done
  LIBS="$fp_save_LIBS"
  CPPFLAGS="$fp_save_CPPFLAGS"])

  if test x"$fp_cv_check_$1_lib" = xno; then
    no_$1=yes
    $1_CFLAGS=
    $1_LIBS=
  else
    $1_CFLAGS0="${$1_CFLAGS}"
    $1_CFLAGS="$CPPFLAGS ${$1_CFLAGS0}"
    $1_LIBS0="$fp_cv_check_$1_lib"
    $1_LIBS="$LDFLAGS ${$1_LIBS0}"
  fi
])# FP_CHECK_GL_HELPER
