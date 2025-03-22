# ====================================
# sslogread: library to parse and query logs
# ====================================

# Requires C++17 or above
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compilation flags
# =================

if(MSVC)
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
else()
  add_compile_options(-Wall -Wextra)
endif()

# ZSTD decompression support, if available
find_package(zstd)
if(zstd_FOUND)
  add_definitions(-DWITH_ZSTD=1)
  if(zstd::libzstd_static)
    set(SSLOGREAD_ZSTD_LIBS zstd::libzstd_static)
  else()
    set(SSLOGREAD_ZSTD_LIBS zstd::libzstd_shared)
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
target_include_directories(sslogread INTERFACE include)
target_include_directories(sslogread PRIVATE include)
target_link_libraries(sslogread sslog ${SSLOGREAD_ZSTD_LIBS} Threads::Threads)

# Library installation
# ====================
file(GLOB headerfiles "${CMAKE_CURRENT_SOURCE_DIR}/include/sslogread/*.h")
install(FILES ${headerfiles} DESTINATION include)
install(TARGETS sslogread ARCHIVE DESTINATION lib)
