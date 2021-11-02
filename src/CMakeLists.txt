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


set_target_properties (${PROJECT_NAME} PROPERTIES C_STANDARD 99)
set_target_properties (${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

if (CONFIG_DEBUG)
  target_compile_options (${PROJECT_NAME} PRIVATE -g)
else ()
  target_compile_options (${PROJECT_NAME} PRIVATE -O2)
endif ()

include(FindPkgConfig)
pkg_check_modules (PIPEWIRE REQUIRED IMPORTED_TARGET libpipewire-0.3)
if (PIPEWIRE_FOUND)
  target_link_libraries (${PROJECT_NAME} PRIVATE PkgConfig::PIPEWIRE)
endif ()

target_link_libraries (${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

install (TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
