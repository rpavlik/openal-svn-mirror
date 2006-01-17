SET(VAR HAVE_VISIBILITY)

IF(NOT DEFINED ${VAR})
  FILE(WRITE "${CMAKE_BINARY_DIR}/CMakeTmp/src.c"
  "void __attribute__((visibility(\"default\"))) test() {}
  #ifdef __INTEL_COMPILER
  #error ICC breaks with binutils and visibility
  #endif
  int main(){}
  ")

  MESSAGE(STATUS "Performing Test ${VAR}")
  TRY_COMPILE(${VAR}
              ${CMAKE_BINARY_DIR}
              ${CMAKE_BINARY_DIR}/CMakeTmp/src.c
              CMAKE_FLAGS
              "-DCOMPILE_DEFINITIONS:STRING=-fvisibility=hidden")

  SET(${VAR} 1 CACHE INTERNAL "Test ${FUNCTION}")
  IF(${VAR})
    MESSAGE(STATUS "Performing Test ${VAR} - Success")
  ELSE(${VAR})
    MESSAGE(STATUS "Performing Test ${VAR} - Failed")
  ENDIF(${VAR})
ENDIF(NOT DEFINED ${VAR})

IF(${VAR})
  ADD_DEFINITIONS(-fvisibility=hidden)
  ADD_DEFINITIONS(-DHAVE_GCC_VISIBILITY)
ENDIF(${VAR})
