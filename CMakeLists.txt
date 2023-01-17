cmake_minimum_required (VERSION 3.15)

project(qpwgraph
  VERSION 0.3.10
  DESCRIPTION "A PipeWire Graph Qt GUI Interface"
  HOMEPAGE_URL "https://gitlab.freedesktop.org/rncbc/qpwgraph"
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
  string (REGEX REPLACE "^1_"      "" GIT_VERSION "${GIT_VERSION}")
  string (REGEX REPLACE "^[_vV]+"  "" GIT_VERSION "${GIT_VERSION}")
  string (REGEX REPLACE "-g"   "git." GIT_VERSION "${GIT_VERSION}")
  string (REGEX REPLACE "[_|-]"   "." GIT_VERSION "${GIT_VERSION}")
  execute_process (
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_REVPARSE_OUTPUT
    RESULT_VARIABLE GIT_REVPARSE_RESULT
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if (GIT_REVPARSE_RESULT EQUAL 0 AND NOT GIT_REVPARSE_OUTPUT STREQUAL "main")
    set (GIT_VERSION "${GIT_VERSION} [${GIT_REVPARSE_OUTPUT}]")
  endif ()
  set (PROJECT_VERSION "${GIT_VERSION}")
endif ()


# Enable ALSA MIDI support option.
option (CONFIG_ALSA_MIDI "Enable ALSA MIDI support (default=yes)" 1)

# Enable system-tray icon support option.
option (CONFIG_SYSTEM_TRAY "Enable system-tray icon support (default=yes)" 1)

# Enable Wayland support option.
option (CONFIG_WAYLAND "Enable Wayland support (EXPERIMENTAL) (default=no)" 0)

# Enable Qt6 build preference.
option (CONFIG_QT6 "Enable Qt6 build (default=yes)" 1)


if (CMAKE_BUILD_TYPE MATCHES "Debug")
  set (CONFIG_DEBUG 1)
  set (CONFIG_BUILD_TYPE "debug")
else ()
  set (CONFIG_DEBUG 0)
  set (CONFIG_BUILD_TYPE "release")
  set (CMAKE_BUILD_TYPE "Release")
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

find_package (Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Xml Svg)

if (CONFIG_SYSTEM_TRAY)
  find_package (Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Network)
endif ()


add_subdirectory (src)


# Configuration status
macro (SHOW_OPTION text value)
  if (${value})
    message ("${text}: yes")
  else ()
    message ("${text}: no")
  endif ()
endmacro ()

message   ("\n  ${PROJECT_NAME} ${PROJECT_VERSION} (Qt ${QT_VERSION})")
message   ("\n  Build target . . . . . . . . . . . . . . . . . . .: ${CONFIG_BUILD_TYPE}\n")
show_option ("  ALSA MIDI support  . . . . . . . . . . . . . . . ." CONFIG_ALSA_MIDI)
show_option ("  System-tray icon support . . . . . . . . . . . . ." CONFIG_SYSTEM_TRAY)
message   ("\n  Install prefix . . . . . . . . . . . . . . . . . .: ${CMAKE_INSTALL_PREFIX}\n")
