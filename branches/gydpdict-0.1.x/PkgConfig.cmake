# needs testing
SET(PKG_CONFIG_MIN_VERSION 0.20)
# variable names
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.6)

# check for pkg-config
# * PKG_CONFIG_EXECUTABLE - location of pkg-config executable
# * PKG_CONFIG_VERSION - version of pkg-config
# * PKG_CONFIG_FOUND - is 1 when sufficient version is found and 0 otherwise
MACRO(CHECK_PKG_CONFIG ver)
  IF(NOT PKG_CONFIG_FOUND)
    # initialize variables
    SET(PKG_CONFIG_EXECUTABLE)
    SET(PKG_CONFIG_VERSION)

    # convert environment path
    FILE(TO_CMAKE_PATH $ENV{PATH} ENV_PATH)

    # check for pkg-config message
    MESSAGE(STATUS "Check for pkg-config")

    # check if program present
    FIND_PROGRAM(PKG_CONFIG_EXECUTABLE NAMES pkg-config PATHS ${ENV_PATH} NO_DEFAULT_PATH)
    MARK_AS_ADVANCED(PKG_CONFIG_EXECUTABLE)

    IF(NOT DEFINED PKG_CONFIG_EXECUTABLE)
      MESSAGE(STATUS "Check for pkg-config -- missing")
      MESSAGE(FATAL_ERROR "The pkg-config utility could not be found on your system.")
    ELSE(NOT DEFINED PKG_CONFIG_EXECUTABLE)
      MESSAGE(STATUS "Check for pkg-config -- ${PKG_CONFIG_EXECUTABLE}")
    ENDIF(NOT DEFINED PKG_CONFIG_EXECUTABLE)

    # check if needed version
    EXEC_PROGRAM(${PKG_CONFIG_EXECUTABLE}
      ARGS --atleast-pkgconfig-version ${ver}
      OUTPUT_VARIABLE junk RETURN_VALUE return_value)

    # check version message
    MESSAGE(STATUS "Check for pkg-config >= ${ver}")

    # get version
    EXEC_PROGRAM(${PKG_CONFIG_EXECUTABLE}
      ARGS --version
      OUTPUT_VARIABLE PKG_CONFIG_VERSION)
    MESSAGE(STATUS "Check for pkg-config >= ${ver} -- ${PKG_CONFIG_VERSION}")

    IF(return_value)
      MESSAGE(FATAL_ERROR "Your version of pkg-config is too old.")
      SET(PKG_CONFIG_FOUND 0)
    ELSE(return_value)
      SET(PKG_CONFIG_FOUND 1)
    ENDIF(return_value)

  ENDIF(NOT PKG_CONFIG_FOUND)
ENDMACRO(CHECK_PKG_CONFIG)

# check for required package with pkg-config
# * ${package}_VERSION - version of package
# * ${package}_CFLAGS - cflags of package
# * ${package}_LIBS - libs of package
MACRO(CHECK_PKG_CONFIG_PACKAGE package ver)
  IF(NOT PKG_CONFIG_FOUND)
    CHECK_PKG_CONFIG(${PKG_CONFIG_MIN_VERSION})
  ENDIF(NOT PKG_CONFIG_FOUND)

  # initialize variables
  SET(${package}_VERSION)
  SET(${package}_CFLAGS)
  SET(${package}_LIBS)

  # check if pkg-config entry exists
  EXEC_PROGRAM(${PKG_CONFIG_EXECUTABLE}
    ARGS --exists ${package}
    OUTPUT_VARIABLE junk RETURN_VALUE return_value)

  # check for package message
  MESSAGE(STATUS "Check for ${package} >= ${ver}")

  IF(return_value)
    MESSAGE(STATUS "Check for ${package} >= ${ver} -- missing")
    MESSAGE(STATUS "Please install at last version ${ver} of package ${package} or adjust PKG_CONFIG_PATH environment variable.")
    MESSAGE(FATAL_ERROR "The required package ${package} was not found on your system.")
  ENDIF(return_value)

  # check if needed version
  EXEC_PROGRAM(${PKG_CONFIG_EXECUTABLE}
    ARGS --atleast-version ${ver} ${package}
    OUTPUT_VARIABLE junk RETURN_VALUE return_value)

  # get version
  EXEC_PROGRAM(${PKG_CONFIG_EXECUTABLE}
    ARGS --modversion ${package}
    OUTPUT_VARIABLE ${package}_VERSION)
  MESSAGE(STATUS "Check for ${package} >= ${ver} -- ${${package}_VERSION}")

  IF(return_value)
    MESSAGE(STATUS "Please install at last version ${ver} of package ${package} or adjust PKG_CONFIG_PATH environment variable.")
    MESSAGE(FATAL_ERROR "Your version of package ${package} is too old.")
  ENDIF(return_value)

  # get cflags
  EXEC_PROGRAM(${PKG_CONFIG_EXECUTABLE}
    ARGS --cflags ${package}
    OUTPUT_VARIABLE ${package}_CFLAGS)
  # hack strange empty line behaviour
  STRING(REGEX REPLACE "^[ ]+\n$" "" ${package}_CFLAGS ${${package}_CFLAGS})
  MESSAGE(STATUS "Check for ${package} >= ${ver} -- CFLAGS=\"${${package}_CFLAGS}\"")

  # get libs
  EXEC_PROGRAM(${PKG_CONFIG_EXECUTABLE}
    ARGS --libs ${package}
    OUTPUT_VARIABLE ${package}_LIBS)
  # hack strange empty line behaviour
  STRING(REGEX REPLACE "^[ ]+\n$" "" ${package}_LIBS ${${package}_LIBS})
  MESSAGE(STATUS "Check for ${package} >= ${ver} -- LIBS=\"${${package}_LIBS}\"")

ENDMACRO(CHECK_PKG_CONFIG_PACKAGE)

# check for optional package with pkg-config
# * ${package}_VERSION - version of package
# * ${package}_CFLAGS - cflags of package
# * ${package}_LIBS - libs of package
MACRO(CHECK_PKG_CONFIG_PACKAGE_OPTIONAL package ver name help default)
  # set variable as cache
  IF(DEFINED use_${name})
    SET(use_${name} use_${name} CACHE BOOL ${help})
  ELSE(DEFINED use_${name})
    SET(use_${name} ${default} CACHE BOOL ${help})
  ENDIF(DEFINED use_${name})

  # perform package test
  IF(use_${name})
    CHECK_PKG_CONFIG_PACKAGE(${package} ${ver})
  ENDIF(use_${name})
ENDMACRO(CHECK_PKG_CONFIG_PACKAGE_OPTIONAL)

MACRO(CHECK_PKG_CONFIG_PACKAGE_IS_VERSION package ver return)
  IF(NOT PKG_CONFIG_FOUND)
    CHECK_PKG_CONFIG(${PKG_CONFIG_MIN_VERSION})
  ENDIF(NOT PKG_CONFIG_FOUND)

  # get version
  EXEC_PROGRAM(${PKG_CONFIG_EXECUTABLE}
    ARGS --exact-version ${ver} ${package}
    OUTPUT_VARIABLE junk RETURN_VALUE return)

  IF(NOT return)
    SET(return 1)
  ELSE(NOT return)
    SET(return 0)
  ENDIF(NOT return)
ENDMACRO(CHECK_PKG_CONFIG_PACKAGE_IS_VERSION)

MACRO(CHECK_PKG_CONFIG_PACKAGE_IS_VERSION_LOWER package ver return)
  IF(NOT PKG_CONFIG_FOUND)
    CHECK_PKG_CONFIG(${PKG_CONFIG_MIN_VERSION})
  ENDIF(NOT PKG_CONFIG_FOUND)

  # get version
  EXEC_PROGRAM(${PKG_CONFIG_EXECUTABLE}
    ARGS --max-version ${ver} ${package}
    OUTPUT_VARIABLE junk RETURN_VALUE return)

  IF(NOT return)
    SET(return 1)
  ELSE(NOT return)
    SET(return 0)
  ENDIF(NOT return)
ENDMACRO(CHECK_PKG_CONFIG_PACKAGE_IS_VERSION_LOWER)

MACRO(CHECK_PKG_CONFIG_PACKAGE_IS_VERSION_HIGHER package ver return)
  IF(NOT PKG_CONFIG_FOUND)
    CHECK_PKG_CONFIG(${PKG_CONFIG_MIN_VERSION})
  ENDIF(NOT PKG_CONFIG_FOUND)

  # get version
  EXEC_PROGRAM(${PKG_CONFIG_EXECUTABLE}
    ARGS --atleast-version ${ver} ${package}
    OUTPUT_VARIABLE junk RETURN_VALUE return)

  IF(NOT return)
    SET(return 1)
  ELSE(NOT return)
    SET(return 0)
  ENDIF(NOT return)
ENDMACRO(CHECK_PKG_CONFIG_PACKAGE_IS_VERSION_HIGHER)
