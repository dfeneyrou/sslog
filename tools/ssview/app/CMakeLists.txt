# ======================
# Graph viewer application
# ======================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

file(GLOB VIEWER_SRC CONFIGURE_DEPENDS *.cpp *.h)

find_package(LibUnwind)
find_package(LibDw)
if(LibUnwind_FOUND AND LibDw_FOUND)
  add_definitions(-DASSERTED_WITH_STRACKTRACE=1)
  message("'Asserted' feature 'stacktrace' enabled for testprogram")
  set(STACKTRACE_LIBS ${LibUnwind_LIBRARY} ${LibDw_LIBRARY})
endif()

find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)

# External Graphviz dependency (no cmake-config available)
include(FindPkgConfig)
pkg_search_module(gvc REQUIRED libgvc libcgraph libcdt)
link_directories(${gvc_LIBRARY_DIRS})
include_directories(${gvc_INCLUDE_DIRS})

# Internal Dear ImGui
if(NOT TARGET ssview::dear_imgui)
  find_package(dear_imgui REQUIRED)
endif()

# Internal ImPlot
if(NOT TARGET ssview::implot)
  find_package(implot REQUIRED)
endif()

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
          ${gvc_LIBRARIES}
          ${STACKTRACE_LIBS}
          Threads::Threads)
