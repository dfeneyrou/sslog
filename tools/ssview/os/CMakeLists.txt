# ====================================
# os
# ====================================

# Requires C++17 or above
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(X11 REQUIRED)

add_compile_options(-Wall -Wextra -Wno-sign-conversion -Werror)

file(GLOB OS_SRC CONFIGURE_DEPENDS *.cpp *.h)

# Library definition
# ==================
add_library(os STATIC ${OS_SRC})
add_library(ssview::os ALIAS os)

target_link_libraries(os PRIVATE ssview::base ssview::asserted ${X11_LIBRARIES})
target_include_directories(os PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                                     $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# Library installation
# ====================

file(GLOB headerfiles "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
set_target_properties(os PROPERTIES PUBLIC_HEADER "${headerfiles}")
set_target_properties(os PROPERTIES LINKER_LANGUAGE CXX) # .cc extension is not properly switching
                                                         # to C++...

include(CMakePackageConfigHelpers)
write_basic_package_version_file(os-config-version.cmake VERSION 1.0.0
                                 COMPATIBILITY SameMajorVersion)

include(GNUInstallDirs)

install(
  TARGETS os
  EXPORT osTargets
  INCLUDES
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT os_Runtime
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT os_Runtime
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT os_Development
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/os COMPONENT os_Development)

install(EXPORT osTargets FILE os-targets.cmake NAMESPACE ssview::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/os COMPONENT os_Development)

export(TARGETS os NAMESPACE ssview:: FILE os-targets.cmake)

install(FILES os-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/os-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/os COMPONENT os_Development)
