# ======================
# ssview application
# ======================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

file(GLOB VIEWER_SRC CONFIGURE_DEPENDS *.cpp *.h)

add_executable(ssview ${VIEWER_SRC})

target_link_libraries(
  ssview
  PRIVATE ssview::asserted
          ssview::dear_imgui
          ssview::implot
          ssview::base
          ssview::os
          ssview::graphic
          sslog::sslogread
          Threads::Threads)

get_target_property(OPTIONS ssview COMPILE_OPTIONS)
message(STATUS "Compile options for ssview: ${OPTIONS}")
