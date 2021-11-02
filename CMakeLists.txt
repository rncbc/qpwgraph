cmake_minimum_required (VERSION 3.13)

project(qpwgraph
  VERSION 0.0.1
  DESCRIPTION "A PipeWire Graph Qt GUI Interface"
  LANGUAGES C CXX)

execute_process (
  COMMAND git describe --tags --dirty --abbrev=6
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_DESCRIBE_OUTPUT
  RESULT_VARIABLE GIT_DESCRIBE_RESULT
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if (GIT_DESCRIBE_RESULT EQUAL 0)
  set (GIT_VERSION "${GIT_DESCRIBE_OUTPUT}")
  string (REGEX REPLACE "^[^0-9]+" "" GIT_VERSION "${GIT_VERSION}")
  string (REGEX REPLACE "-g"   "git." GIT_VERSION "${GIT_VERSION}")
  string (REGEX REPLACE "[_|-]"   "." GIT_VERSION "${GIT_VERSION}")
  set (PROJECT_VERSION "${GIT_VERSION}")
endif ()


# Enable Qt6 build preference.
option (CONFIG_QT6 "Enable Qt6 build (default=yes)" 1)


if (CMAKE_BUILD_TYPE MATCHES "Debug")
  set (CONFIG_DEBUG 1)
else ()
  set (CONFIG_DEBUG 0)
endif ()


include (GNUInstallDirs)


# Check for Qt...
if (CONFIG_QT6)
  find_package (Qt6 QUIET)
  if (NOT Qt6_FOUND)
    set (CONFIG_QT6 0)
  endif ()
endif ()

if (CONFIG_QT6)
  find_package (QT QUIET NAMES Qt6)
else ()
  find_package (QT QUIET NAMES Qt5)
endif ()

find_package (Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)


add_subdirectory (src)
