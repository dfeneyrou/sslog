# ====================================
# base
# ====================================

# Requires C++17 or above
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wno-sign-conversion -Werror)

file(GLOB BASE_SRC CONFIGURE_DEPENDS *.cpp *.h)

# Library definition
# ==================
add_library(base STATIC ${BASE_SRC})
add_library(ssview::base ALIAS base)

target_link_libraries(base PRIVATE ssview::asserted)
target_include_directories(base PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                                       $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# Library installation
# ====================

file(GLOB headerfiles "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
set_target_properties(base PROPERTIES PUBLIC_HEADER "${headerfiles}")
set_target_properties(base PROPERTIES LINKER_LANGUAGE CXX) # .cc extension is not properly switching
                                                           # to C++...

include(CMakePackageConfigHelpers)
write_basic_package_version_file(base-config-version.cmake VERSION 1.0.0
                                 COMPATIBILITY SameMajorVersion)

include(GNUInstallDirs)

install(
  TARGETS base
  EXPORT baseTargets
  INCLUDES
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT base_Runtime
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT base_Runtime
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT base_Development
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/base COMPONENT base_Development)

install(EXPORT baseTargets FILE base-targets.cmake NAMESPACE ssview::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/base COMPONENT base_Development)

export(TARGETS base NAMESPACE ssview:: FILE base-targets.cmake)

install(FILES base-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/base-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/base COMPONENT base_Development)
