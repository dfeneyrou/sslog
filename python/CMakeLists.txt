# ================================
# sslog python reader module
# ================================

find_package(Python3 COMPONENTS Interpreter REQUIRED)

# ZSTD decompression support. Mandatory for the Python module, for more reliable distribution
find_package(zstd REQUIRED)

# Some definitions set(SETUP_PY_IN ${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in)
set(SETUP_PY ${CMAKE_CURRENT_BINARY_DIR}/setup.py)

# Allows to easily track "dirtiness" without knowing the name of the real output files
set(FAKE_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/python_reader_install)

# Dependency files
file(
  GLOB
  DEPS
  CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/../python/setup.py
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib/include/sslog.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../tools/sslogread/include/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../tools/sslogread/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../tools/sslogread/src/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../python/sslogread/pySslogReader.cpp)

# Create the setup.py on the build side (copy of the (manual usage) setup.py but with path
# automatically set by cmake)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../python/setup.py ${SETUP_PY})

# Command which builds and installs the library. Officially, it creates an artificial
# "dependency_timestamp" file, which is easier to track than the real output of the build
add_custom_command(OUTPUT ${FAKE_OUTPUT} COMMAND ${Python3_EXECUTABLE} -m build --wheel --sdist
                   COMMAND ${CMAKE_COMMAND} -E touch ${FAKE_OUTPUT} DEPENDS ${DEPS})

# Add the target
add_custom_target(python_reader ALL DEPENDS ${FAKE_OUTPUT})

# Add the installation command
install(
  CODE "execute_process(COMMAND ${Python3_EXECUTABLE} \"${CMAKE_SOURCE_DIR}/cmake/pip_install_whl_package.py\" ${Python3_EXECUTABLE} \"${CMAKE_CURRENT_BINARY_DIR}\")"
)
