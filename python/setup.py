# The MIT License (MIT)
#
# Copyright(c) 2025, Damien Feneyrou <dfeneyrou@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files(the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions :
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import io
import os
import os.path
import sys
import shutil
import pathlib
from setuptools import setup, find_packages, Extension

# - Require C++17
# - ZSTD decompression support. Mandatory for the Python module, for more reliable distribution
extra_compilation_flags = ["-DSSLOG_NO_AUTOSTART=1", "-DWITH_ZSTD=1"]
extra_link_args = []
extra_data_files = None
if sys.platform != "win32":
    extra_compilation_flags += ["-std=c++17"]
    extra_link_args += ["-l:libzstd.a"]
else:
    extra_compilation_flags += ["/std:c++17"]
    extra_link_args += ["libzstd.dll.a"]
    extra_data_files = [('lib\\site-packages\\', [os.getenv("zstd_LIBRARY")])]  # Also package the Zstd DLL

classifiers_list = [
    "Intended Audience :: Developers",
    "Intended Audience :: Science/Research",
    "Programming Language :: Python :: 3",
    "Programming Language :: Python :: 3.8",
    "Programming Language :: Python :: 3.9",
    "Programming Language :: Python :: 3.10",
    "Programming Language :: Python :: 3.11",
    "Programming Language :: Python :: 3.12",
    "Programming Language :: Python :: Implementation :: CPython",
    "Environment :: Console",
    "Operating System :: Microsoft :: Windows",
    "Operating System :: POSIX :: Linux",
    "Topic :: Software Development",
]

# If in-source, copy sslog inside the folder (constraint from setup.py)
external_source_files = [
    "tools/sslogread/src/logSession.cpp", "tools/sslogread/src/parserHelpers.cpp", "tools/sslogread/src/stringDb.cpp",
    "tools/sslogread/src/vsnprintf.cpp", "tools/sslogread/src/utils.cpp", "python/sslogread/pySslogReader.cpp"
]
external_libinclude_files = ["tools/sslogread/include/sslogread/sslogread.h", "tools/sslogread/include/sslogread/utils.h"]
isSdist = (sys.argv[1] == "sdist")  # Building from the already made source package

if os.path.isfile("../../lib/include/sslog.h"):
    # Rebuild the sources so that a source package can be formed
    # Drawback: the Python target is rebuilt each time...
    pathlib.Path("sslogread").mkdir(exist_ok=True)
    pathlib.Path("sslogread/sslogread").mkdir(exist_ok=True)
    for f in external_source_files:
        shutil.copyfile("../../"+f, "sslogread/%s" % os.path.split(f)[1])
    shutil.copyfile("../../lib/include/sslog.h", "sslogread/sslog.h")
    shutil.copyfile("../../tools/sslogread/include/sslogread/sslogread.h", "sslogread/sslogread/sslogread.h")
    shutil.copyfile("../../tools/sslogread/include/sslogread/utils.h", "sslogread/sslogread/utils.h")
    shutil.copyfile("../../python/README.md", "README.md")

# Read the sslog version from the C++ header library
with io.open("sslogread/sslog.h", encoding="UTF-8") as versionFile:
    SSLOG_VERSION = (
        [l for l in versionFile.read().split("\n") if "SSLOG_VERSION " in l][0]
        .split()[2].replace('"', "")
    )

# Read the content of the readme file in this folder
with io.open("README.md", encoding="UTF-8") as readmeFile:
    long_description = readmeFile.read()

# Build the sources
sources = [os.path.normpath("sslogread/%s" % os.path.split(f)[1]) for f in external_source_files]
if isSdist:
    sources.extend(["sslogread/sslog.h", "sslogread/sslogread/sslogread.h", "sslogread/sslogread/utils.h"])

# Build call
setup(
    name="sslogread",
    version=SSLOG_VERSION,
    author="Damien Feneyrou",
    author_email="dfeneyrou@gmail.com",
    license="MIT",
    description="sslog reader library",
    long_description=long_description,
    long_description_content_type="text/markdown",
    classifiers=classifiers_list, python_requires=">=3.7",
    url="https://github.com/dfeneyrou/sslog",
    packages=find_packages(),
    ext_modules=[Extension("sslogread", sources=sources, extra_compile_args=extra_compilation_flags, extra_link_args=extra_link_args, include_dirs=["sslogread"])],
    data_files=extra_data_files
)
