IF(CMAKE_COMPILER_IS_GNUCC)
  SET(VAR HAVE_MMX_FLAG)
  SET(CMAKE_REQUIRED_FLAGS "-mmmx")
  CHECK_C_SOURCE_COMPILES(
"
int main(){__builtin_ia32_emms();}
" ${VAR})

  IF(${VAR})
    SET(ADD_CFLAGS "${ADD_CFLAGS} -mmmx")
  ENDIF(${VAR})

  SET(VAR HAVE_SSE2_FLAG)
  SET(CMAKE_REQUIRED_FLAGS "-msse2")
  CHECK_C_SOURCE_COMPILES(
"
int main(){double test;__builtin_ia32_loadupd(&test);}
" ${VAR})

  IF(${VAR})
    SET(ADD_CFLAGS "${ADD_CFLAGS} -msse2")
  ENDIF(${VAR})

  SET(CMAKE_REQUIRED_FLAGS "")
ELSE(CMAKE_COMPILER_IS_GNUCC)

  MESSAGE("You may need to specify the correct compiler flag to enable MMX and SSE2.")
ENDIF(CMAKE_COMPILER_IS_GNUCC)