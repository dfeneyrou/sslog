# ====================================
# graphic
# ====================================

# Requires C++17 or above
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(X11 REQUIRED)

if(NOT X11_Xrender_FOUND)
  message(FATAL_ERROR "Xrender library has not been found")
endif()
if(NOT X11_Xrandr_FOUND)
  message(FATAL_ERROR "Xrandr library has not been found")
endif()

add_compile_options(-Wall -Wextra -Wno-sign-conversion -Werror)

file(GLOB GRAPHIC_SRC CONFIGURE_DEPENDS *.cpp *.h)

# Library definition
# ==================
add_library(graphic STATIC ${GRAPHIC_SRC})
add_library(ssview::graphic ALIAS graphic)

target_link_libraries(graphic PRIVATE ssview::asserted ssview::base OpenGL::GL ${X11_LIBRARIES}
                                      ${X11_Xrender_LIB} ${X11_Xrandr_LIB})
target_include_directories(graphic PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                                          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# Library installation
# ====================

file(GLOB headerfiles "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
set_target_properties(graphic PROPERTIES PUBLIC_HEADER "${headerfiles}")

include(CMakePackageConfigHelpers)
write_basic_package_version_file(graphic-config-version.cmake VERSION 1.0.0
                                 COMPATIBILITY SameMajorVersion)

include(GNUInstallDirs)

install(
  TARGETS graphic
  EXPORT graphicTargets
  INCLUDES
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT graphic_Runtime
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT graphic_Runtime
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT graphic_Development
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/graphic COMPONENT graphic_Development)

install(EXPORT graphicTargets FILE graphic-targets.cmake NAMESPACE ssview::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/graphic COMPONENT graphic_Development)

export(TARGETS graphic NAMESPACE ssview:: FILE graphic-targets.cmake)

install(FILES graphic-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/graphic-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/graphic COMPONENT graphic_Development)
