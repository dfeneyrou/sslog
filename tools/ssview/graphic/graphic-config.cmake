include(CMakeFindDependencyMacro)
find_dependency(ssview::base ssview::os)
include(${CMAKE_CURRENT_LIST_DIR}/graphic-targets.cmake)
