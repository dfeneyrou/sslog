# ====================================
# implot
# ====================================

# Requires C++17 or above
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wno-sign-conversion -Werror)

file(GLOB IMPLOT_SRC CONFIGURE_DEPENDS *.cpp *.h)

# Library definition
# ==================
add_library(implot STATIC ${IMPLOT_SRC})
add_library(ssview::implot ALIAS implot)

target_link_libraries(implot PRIVATE ssview::dear_imgui)
target_include_directories(implot PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# Library installation
# ====================

file(GLOB headerfiles "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
set_target_properties(implot PROPERTIES PUBLIC_HEADER "${headerfiles}")
include(CMakePackageConfigHelpers)
write_basic_package_version_file(implot-config-version.cmake VERSION 1.0.0 COMPATIBILITY SameMajorVersion)

include(GNUInstallDirs)

install(
  TARGETS implot
  EXPORT implotTargets
  INCLUDES
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT implot_Runtime
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT implot_Runtime
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT implot_Development
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/implot COMPONENT implot_Development)

install(EXPORT implotTargets FILE implot-targets.cmake NAMESPACE ssview::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/implot COMPONENT implot_Development)

export(TARGETS implot NAMESPACE ssview:: FILE implot-targets.cmake)

install(FILES implot-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/implot-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/implot COMPONENT implot_Development)
