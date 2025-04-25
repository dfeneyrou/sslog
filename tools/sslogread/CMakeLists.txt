# ====================================
# sslogread: library to parse and query logs
# ====================================

# Requires C++17 or above
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ZSTD decompression support, if available
find_package(zstd)

# Compilation flags
# =================

if(MSVC)
  # Windows
  add_compile_options(/DUNICODE) # Unicode app
  add_compile_options(/W4 /permissive-)
  add_compile_options(/wd4996) # Disable the "This function or variable may be unsafe", pushing for
                               # not well supported extensions
  add_compile_options(/wd4324) # Disable the "structure was padded due to alignment specifier". Yes,
                               # we use alignas(), no problem with that
  add_compile_options(/wd4201) # Disable the "nonstandard extension used: nameless struct/union"
  add_compile_options(/wd4127) # Disable the "conditional expression is constant" warning,
                               # applicable only from C++17
  add_compile_options(/EHsc)

  if(zstd_FOUND)
    add_definitions(-DWITH_ZSTD=1)
    set(SSLOGREAD_ZSTD_LIBS ${zstd_LIBRARY})
    set(SSLOGREAD_ZSTD_INCLUDE ${zstd_INCLUDE_DIR})
  endif()

else()
  # Linux
  add_compile_options(-Wall -Wextra -Wno-sign-conversion -Werror)

  if(zstd_FOUND)
    add_definitions(-DWITH_ZSTD=1)
    if(zstd::libzstd_static)
      set(SSLOGREAD_ZSTD_LIBS zstd::libzstd_static)
    else()
      set(SSLOGREAD_ZSTD_LIBS zstd::libzstd_shared)
    endif()
  endif()

endif()

# Add user flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CUSTOM_FLAGS}")

set(SSLOGREAD_SRC
    src/stringDb.cpp
    src/logSession.cpp
    src/parserHelpers.cpp
    src/vsnprintf.cpp
    src/utils.cpp
    include/sslogread/utils.h
    include/sslogread/sslogread.h)

# Library definition
# ==================
add_library(sslogread STATIC ${SSLOGREAD_SRC})
add_library(sslog::sslogread ALIAS sslogread)

if(NOT TARGET sslog::sslog)
  find_package(sslog)
endif()

target_include_directories(sslogread PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                                            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_include_directories(sslogread PRIVATE include "${SSLOGREAD_ZSTD_INCLUDE}")
target_link_libraries(sslogread PUBLIC sslog::sslog ${SSLOGREAD_ZSTD_LIBS} Threads::Threads)

# Library installation
# ====================
file(GLOB headerfiles "${CMAKE_CURRENT_SOURCE_DIR}/include/sslogread/*.h")
set_target_properties(sslogread PROPERTIES PUBLIC_HEADER "${headerfiles}")

include(CMakePackageConfigHelpers)
write_basic_package_version_file(sslogread-config-version.cmake VERSION 1.0.0
                                 COMPATIBILITY SameMajorVersion)

include(GNUInstallDirs)

install(
  TARGETS sslogread
  EXPORT sslogreadTargets
  INCLUDES
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT sslogread_Runtime
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT sslogread_Runtime
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT sslogread_Development
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sslogread COMPONENT sslogread_Development)

install(EXPORT sslogreadTargets FILE sslogread-targets.cmake NAMESPACE sslog::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sslogread COMPONENT sslogread_Development)

export(TARGETS sslogread NAMESPACE sslog:: FILE sslogread-targets.cmake)

install(FILES sslogread-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/sslogread-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sslogread COMPONENT sslogread_Development)
