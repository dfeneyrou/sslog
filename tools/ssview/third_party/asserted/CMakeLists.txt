# ====================================
# asserted
# ====================================

# Requires C++17 or above
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wno-sign-conversion -Werror)

file(GLOB ASSERTED_SRC CONFIGURE_DEPENDS *.h)

# Library definition
# ==================
add_library(asserted STATIC ${ASSERTED_SRC})
add_library(ssview::asserted ALIAS asserted)

target_include_directories(asserted PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# Library installation
# ====================

file(GLOB headerfiles "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
set_target_properties(asserted PROPERTIES PUBLIC_HEADER "${headerfiles}")
set_target_properties(asserted PROPERTIES LINKER_LANGUAGE CXX) # .cc extension is not properly switching to C++...

include(CMakePackageConfigHelpers)
write_basic_package_version_file(asserted-config-version.cmake VERSION 1.0.0 COMPATIBILITY SameMajorVersion)

include(GNUInstallDirs)

install(
  TARGETS asserted
  EXPORT assertedTargets
  INCLUDES
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT asserted_Runtime
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT asserted_Runtime
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT asserted_Development
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/asserted COMPONENT asserted_Development)

install(EXPORT assertedTargets FILE asserted-targets.cmake NAMESPACE ssview::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/asserted COMPONENT asserted_Development)

export(TARGETS asserted NAMESPACE ssview:: FILE asserted-targets.cmake)

install(FILES asserted-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/asserted-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/asserted COMPONENT asserted_Development)

