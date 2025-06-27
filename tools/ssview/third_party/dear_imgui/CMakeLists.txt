# ====================================
# dear_imgui
# ====================================

# Requires C++17 or above
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wno-sign-conversion -Werror)

file(GLOB IMGUI_SRC CONFIGURE_DEPENDS *.cpp *.h)

# Library definition
# ==================
add_library(dear_imgui STATIC ${IMGUI_SRC})
add_library(ssview::dear_imgui ALIAS dear_imgui)

target_include_directories(dear_imgui PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# Library installation
# ====================

file(GLOB headerfiles "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
set_target_properties(dear_imgui PROPERTIES PUBLIC_HEADER "${headerfiles}")
include(CMakePackageConfigHelpers)
write_basic_package_version_file(dear_imgui-config-version.cmake VERSION 1.0.0 COMPATIBILITY SameMajorVersion)

include(GNUInstallDirs)

install(
  TARGETS dear_imgui
  EXPORT dear_imguiTargets
  INCLUDES
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT dear_imgui_Runtime
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT dear_imgui_Runtime
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT dear_imgui_Development
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dear_imgui COMPONENT dear_imgui_Development)

install(EXPORT dear_imguiTargets FILE dear_imgui-targets.cmake NAMESPACE ssview::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dear_imgui COMPONENT dear_imgui_Development)

export(TARGETS dear_imgui NAMESPACE ssview:: FILE dear_imgui-targets.cmake)

install(FILES dear_imgui-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/dear_imgui-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dear_imgui COMPONENT dear_imgui_Development)
