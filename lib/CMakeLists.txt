cmake_minimum_required(VERSION 3.15)
project(sslog_lib VERSION 1.0.0 DESCRIPTION "Speedy Structured Logger")

# Build the logging library
add_library(sslog INTERFACE)
add_library(sslog::sslog ALIAS sslog)

target_include_directories(sslog INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                                           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
set(headers include/sslog.h)
set_target_properties(sslog PROPERTIES PUBLIC_HEADER "${headers}")

include(CMakePackageConfigHelpers)
write_basic_package_version_file(sslog-config-version.cmake VERSION 1.0.0
                                 COMPATIBILITY SameMajorVersion)

include(GNUInstallDirs)

install(
  TARGETS sslog
  EXPORT sslogTargets
  INCLUDES
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT sslog_Runtime
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT sslog_Runtime
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT sslog_Development
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT sslog_Development)

install(EXPORT sslogTargets FILE sslog-targets.cmake NAMESPACE sslog::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sslog COMPONENT sslog_Development)

export(TARGETS sslog NAMESPACE sslog:: FILE sslog-targets.cmake)

install(FILES sslog-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/sslog-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sslog COMPONENT sslog_Development)
