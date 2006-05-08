## FindPkgConfig.cmake
## by  Albert Strasheim <http://students . ee . sun . ac . za/~albert/>
##     Alex Brooks (a.brooks at acfr . usyd . edu . au)
##     Prakash Punnoor (prakash at punnoor . de)
##
## This module finds packages using pkg-config, which retrieves
## information about packages from special metadata files.
##
## See http://www . freedesktop . org/Software/pkgconfig/
##
## -------------------------------------------------------------------
##
## Usage:
##
## INCLUDE( ${CMAKE_ROOT}/Modules/FindPkgConfig.cmake)
##
## IF ( CMAKE_PKGCONFIG_EXECUTABLE )
##
##     # Find all the librtk stuff with pkg-config
##     PKGCONFIG( "librtk >= 2.0" HAVE_RTK RTK_INCLUDE_DIRS RTK_DEFINES RTK_LINK_DIRS RTK_LIBS )
##
## ELSE  ( CMAKE_PKGCONFIG_EXECUTABLE )
##
##     # Can't find pkg-config -- have to find librtk somehow else
##
## ENDIF ( CMAKE_PKGCONFIG_EXECUTABLE )
##
##
## Notes:
##
## You can set the PKG_CONFIG_PATH environment variable to tell
## pkg-config where to search for .pc files. See pkg-config(1) for
## more information.
##
#
# FIXME: IF(WIN32) pkg-config --msvc-syntax ENDIF(WIN32) ???
#
# FIXME: Parsing of pkg-config output is specific to gnu-style flags
#

FIND_PROGRAM(CMAKE_PKGCONFIG_EXECUTABLE NAMES pkg-config)
IF(CMAKE_PKGCONFIG_EXECUTABLE MATCHES "NOTFOUND")
  MESSAGE(STATUS "pkg-config executable NOT FOUND")
ENDIF(CMAKE_PKGCONFIG_EXECUTABLE MATCHES "NOTFOUND")

MARK_AS_ADVANCED(CMAKE_PKGCONFIG_EXECUTABLE)

########################################

MACRO(PKGCONFIG_PARSE_FLAGS FLAGS INCLUDES DEFINES)

  #MESSAGE("DEBUG: FLAGS: ${FLAGS}")
  # We need to make sure that input doesn't consist of one ENTER
  SET(SFLAGS " ${FLAGS}")
  STRING(ASCII 32 10 FLAGS_EMPTY)

  IF(SFLAGS MATCHES ${FLAGS_EMPTY})
    SET(${INCLUDES} "")
    SET(${DEFINES} "")
  ELSE(SFLAGS MATCHES ${FLAGS_EMPTY})
    STRING(REGEX MATCHALL "-I[^ ]*" ${INCLUDES} "${FLAGS}")
    STRING(REGEX REPLACE "-I" "" ${INCLUDES} "${${INCLUDES}}")

    # twice, as we want to get rid of includes trailing spaces
    STRING(REGEX REPLACE "-I[^ ]* " "" ${DEFINES} "${FLAGS}")
    STRING(REGEX REPLACE "-I[^ ]*" "" ${DEFINES} "${${DEFINES}}")
  ENDIF(SFLAGS MATCHES ${FLAGS_EMPTY})
    #MESSAGE("DEBUG: INCLUDES: ${${INCLUDES}}")
    #MESSAGE("DEBUG: DEFINES: ${${DEFINES}}")

ENDMACRO(PKGCONFIG_PARSE_FLAGS)

########################################

MACRO(PKGCONFIG_PARSE_LIBS LIBS LINKDIRS LINKLIBS)

  #MESSAGE("DEBUG: LIBS: ${LIBS}")

  STRING(REGEX MATCHALL "-L[^ ]*" ${LINKDIRS} "${LIBS}")
  STRING(REGEX REPLACE "-L" "" ${LINKDIRS} "${${LINKDIRS}}")
  #MESSAGE("DEBUG: LINKDIRS: ${${LINKDIRS}}")

  STRING(REGEX MATCHALL "-l[^ ]*" ${LINKLIBS} "${LIBS}")
  STRING(REGEX REPLACE "-l" "" ${LINKLIBS} "${${LINKLIBS}}")
  #MESSAGE("DEBUG: LINKLIBS: ${${LINKLIBS}}")

ENDMACRO(PKGCONFIG_PARSE_LIBS)

########################################

MACRO(PKGCONFIG LIBRARY FOUND INCLUDE_DIRS DEFINES LINKDIRS LINKLIBS)

  SET(${FOUND} 0)

  MESSAGE(STATUS "pkg-config: Looking for ${LIBRARY}")

  IF(NOT CMAKE_PKGCONFIG_EXECUTABLE MATCHES "NOTFOUND")
    # MESSAGE("DEBUG: pkg-config executable found")

    EXEC_PROGRAM(${CMAKE_PKGCONFIG_EXECUTABLE}
      ARGS "'${LIBRARY}'"
      OUTPUT_VARIABLE PKGCONFIG_OUTPUT
      RETURN_VALUE PKGCONFIG_RETURN)

    IF(NOT PKGCONFIG_RETURN)

      # set C_FLAGS and CXX_FLAGS
      EXEC_PROGRAM(${CMAKE_PKGCONFIG_EXECUTABLE}
        ARGS "--cflags '${LIBRARY}'"
        OUTPUT_VARIABLE CMAKE_PKGCONFIG_C_FLAGS)
      PKGCONFIG_PARSE_FLAGS( "${CMAKE_PKGCONFIG_C_FLAGS}" ${INCLUDE_DIRS} ${DEFINES} )

      # set LIBRARIES
      EXEC_PROGRAM(${CMAKE_PKGCONFIG_EXECUTABLE}
        ARGS "--libs '${LIBRARY}'"
        OUTPUT_VARIABLE CMAKE_PKGCONFIG_LIBRARIES)
      PKGCONFIG_PARSE_LIBS ( "${CMAKE_PKGCONFIG_LIBRARIES}" ${LINKDIRS} ${LINKLIBS} )

      SET(${FOUND} 1)
      MESSAGE(STATUS "pkg-config: Looking for ${LIBRARY} -- found")
    ELSE(NOT PKGCONFIG_RETURN)

      MESSAGE(STATUS "pkg-config: Looking for ${LIBRARY} -- not found")
    ENDIF(NOT PKGCONFIG_RETURN)
  ENDIF(NOT CMAKE_PKGCONFIG_EXECUTABLE MATCHES "NOTFOUND")

  #search for foo-config
  IF(CMAKE_PKGCONFIG_EXECUTABLE MATCHES "NOTFOUND" OR PKGCONFIG_RETURN)
    MESSAGE(STATUS "Looking for ${LIBRARY}-config")
    FIND_PROGRAM(CMAKE_${LIBRARY}-CONFIG NAMES ${LIBRARY}-config)
    IF(NOT CMAKE_${LIBRARY}-CONFIG MATCHES "NOTFOUND")
      # set C_FLAGS and CXX_FLAGS
      EXEC_PROGRAM(${CMAKE_${LIBRARY}-CONFIG}
        ARGS "--cflags"
        OUTPUT_VARIABLE CMAKE_CONFIG_C_FLAGS)
      PKGCONFIG_PARSE_FLAGS( "${CMAKE_CONFIG_C_FLAGS}" ${INCLUDE_DIRS} ${DEFINES} )

      # set LIBRARIES
      EXEC_PROGRAM(${CMAKE_${LIBRARY}-CONFIG}
        ARGS "--libs"
        OUTPUT_VARIABLE CMAKE_CONFIG_LIBRARIES)
      PKGCONFIG_PARSE_LIBS ( "${CMAKE_CONFIG_LIBRARIES}" ${LINKDIRS} ${LINKLIBS} )

      SET(${FOUND} 1)
      MESSAGE(STATUS "Looking for ${LIBRARY}-config -- found")
    ELSE(NOT CMAKE_${LIBRARY}-CONFIG MATCHES "NOTFOUND")

      SET(${INCLUDE_DIRS} "")
      SET(${DEFINES} "")
      SET(${LINKDIRS} "")
      SET(${LINKLIBS} "")
      MESSAGE(STATUS "Looking for ${LIBRARY}-config -- not found")
    ENDIF(NOT CMAKE_${LIBRARY}-CONFIG MATCHES "NOTFOUND")
  ENDIF(CMAKE_PKGCONFIG_EXECUTABLE MATCHES "NOTFOUND" OR PKGCONFIG_RETURN)
  #MESSAGE("Have  ${LIBRARY}       : ${${FOUND}}")
  #MESSAGE("${LIBRARY} include dirs: ${${INCLUDE_DIRS}}")
  #MESSAGE("${LIBRARY} defines     : ${${DEFINES}}")
  #MESSAGE("${LIBRARY} link dirs   : ${${LINKDIRS}}")
  #MESSAGE("${LIBRARY} link libs   : ${${LINKLIBS}}")

ENDMACRO(PKGCONFIG)
