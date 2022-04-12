# project (qpwgraph)

set (CMAKE_INCLUDE_CURRENT_DIR ON)

set (CMAKE_AUTOUIC ON)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
  file (REMOVE ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
endif ()
configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)


set (HEADERS
  qpwgraph.h
  qpwgraph_config.h
  qpwgraph_canvas.h
  qpwgraph_command.h
  qpwgraph_connect.h
  qpwgraph_port.h
  qpwgraph_node.h
  qpwgraph_item.h
  qpwgraph_sect.h
  qpwgraph_pipewire.h
  qpwgraph_alsamidi.h
  qpwgraph_alsamidi.h
  qpwgraph_patchbay.h
  qpwgraph_systray.h
  qpwgraph_form.h
)

set (SOURCES
  qpwgraph.cpp
  qpwgraph_config.cpp
  qpwgraph_canvas.cpp
  qpwgraph_command.cpp
  qpwgraph_connect.cpp
  qpwgraph_port.cpp
  qpwgraph_node.cpp
  qpwgraph_item.cpp
  qpwgraph_sect.cpp
  qpwgraph_pipewire.cpp
  qpwgraph_alsamidi.cpp
  qpwgraph_patchbay.cpp
  qpwgraph_systray.cpp
  qpwgraph_form.cpp
)

set (FORMS
  qpwgraph_form.ui
)

set (RESOURCES
  qpwgraph.qrc
)

add_executable (${PROJECT_NAME}
  ${HEADERS}
  ${SOURCES}
  ${FORMS}
  ${RESOURCES}
)

# Add some debugger flags.
if (CONFIG_DEBUG AND UNIX AND NOT APPLE)
  set (CONFIG_DEBUG_OPTIONS -g -fsanitize=address -fno-omit-frame-pointer)
  target_compile_options (${PROJECT_NAME} PRIVATE ${CONFIG_DEBUG_OPTIONS})
  target_link_options (${PROJECT_NAME} PRIVATE ${CONFIG_DEBUG_OPTIONS})
endif ()

set_target_properties (${PROJECT_NAME} PROPERTIES C_STANDARD 99)
set_target_properties (${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

include(FindPkgConfig)
pkg_check_modules (PIPEWIRE REQUIRED IMPORTED_TARGET libpipewire-0.3)
if (PIPEWIRE_FOUND)
  target_link_libraries (${PROJECT_NAME} PRIVATE PkgConfig::PIPEWIRE)
else ()
  message (WARNING "*** PipeWire library not found.")
endif ()

if (CONFIG_ALSA_MIDI)
  pkg_check_modules (ALSA REQUIRED IMPORTED_TARGET alsa)
  if (ALSA_FOUND)
    target_link_libraries (${PROJECT_NAME} PRIVATE PkgConfig::ALSA)
  endif ()
endif ()

target_link_libraries (${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries (${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Xml)
target_link_libraries (${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Svg)

if (CONFIG_SYSTEM_TRAY)
  target_link_libraries (${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Network)
endif ()

install (TARGETS ${PROJECT_NAME} RUNTIME
  DESTINATION ${CMAKE_INSTALL_BINDIR})
install (FILES images/${PROJECT_NAME}.png
  RENAME org.rncbc.${PROJECT_NAME}.png
  DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/apps)
install (FILES images/${PROJECT_NAME}.svg
  RENAME org.rncbc.${PROJECT_NAME}.svg
  DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install (FILES appdata/org.rncbc.${PROJECT_NAME}.desktop
  DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install (FILES appdata/org.rncbc.${PROJECT_NAME}.metainfo.xml
  DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install (FILES mimetypes/org.rncbc.${PROJECT_NAME}.xml
  DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages)
install (FILES mimetypes/org.rncbc.${PROJECT_NAME}.application-x-${PROJECT_NAME}-patchbay.png
  DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/mimetypes)
install (FILES mimetypes/org.rncbc.${PROJECT_NAME}.application-x-${PROJECT_NAME}-patchbay.svg
  DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/mimetypes)

install (FILES man1/qpwgraph.1
  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

