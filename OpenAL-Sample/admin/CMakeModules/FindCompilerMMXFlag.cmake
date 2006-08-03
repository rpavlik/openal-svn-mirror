SET(VAR HAVE_MMX_FLAG)

IF(CMAKE_COMPILER_IS_GNUCC)
  SET(CMAKE_REQUIRED_FLAGS "-mmmx")
  CHECK_C_SOURCE_COMPILES(
"
int main(){__builtin_ia32_emms();}
" ${VAR})
  SET(CMAKE_REQUIRED_FLAGS "")

  IF(${VAR})
    SET(ADD_CFLAGS "${ADD_CFLAGS} -mmmx")
  ENDIF(${VAR})
ELSE(CMAKE_COMPILER_IS_GNUCC)
  MESSAGE("You may need to specify the correct compiler flag to enable MMX.")
ENDIF(CMAKE_COMPILER_IS_GNUCC)