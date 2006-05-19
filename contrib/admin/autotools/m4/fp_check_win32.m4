# FP_CHECK_WIN32
# --------------
# If Windows is the target platform (e.g. MinGW/MSYS or Cygwin with
# -mno-cygwin), the variable "fp_is_win32" is set to "yes", otherwise (e.g. *nix
# systems or plain Cygwin) it is set to "no".
AC_DEFUN([FP_CHECK_WIN32],
[AC_CACHE_CHECK([for Windows environment], [fp_cv_is_win32],
  [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [
#if !_WIN32
   syntax error;
#endif
])], [fp_cv_is_win32=yes], [fp_cv_is_win32=no])])
fp_is_win32="$fp_cv_is_win32"[]dnl
])# FP_CHECK_WIN32
