include(CMakeFindDependencyMacro)
find_dependency(Threads)
find_dependency(zstd)
find_dependency(sslog)
include(${CMAKE_CURRENT_LIST_DIR}/sslogread-targets.cmake)
