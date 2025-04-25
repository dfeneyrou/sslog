# ======================
# 'sslog' test program
# ======================

# Requires C++17 or above
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Library dependencies
# ====================
if(WIN32)
  # Windows: all libraries are built-in
  if(NOT ${CUSTOM_FLAGS} MATCHES ".*SSLOG_STACKTRACE=0.*")
    message("Stacktrace enabled for testsslog")
    add_definitions(-DSSLOG_STACKTRACE=1)
  endif()
else(WIN32)
  if(NOT CUSTOM_FLAGS MATCHES ".*SSLOG_STACKTRACE=0.*")
    # Linux: if libunwind and libdw are present, the stacktrace feature is activated
    find_package(LibUnwind)
    find_package(LibDw)
    if(LibUnwind_FOUND AND LibDw_FOUND)
      add_definitions(-DSSLOG_STACKTRACE=1)
      message("Stacktrace enabled for testsslog")
      set(STACKTRACE_LIBS ${LibUnwind_LIBRARY} ${LibDw_LIBRARY})
    endif()
  endif()
endif(WIN32)

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
  add_compile_options(-Wall -Wextra -pedantic)

endif()

# Add user flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CUSTOM_FLAGS}")

set(TESTSSLOG_SRC testSslog.cpp testPart.cpp testPart.h)

# Test program executable
# =======================
add_executable(testsslog ${TESTSSLOG_SRC})
target_link_libraries(testsslog PRIVATE sslog::sslog ${STACKTRACE_LIBS} Threads::Threads)
