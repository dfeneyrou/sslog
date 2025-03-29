
[![Build Status](https://github.com/dfeneyrou/sslog/workflows/build/badge.svg)](https://github.com/dfeneyrou/sslog/actions?workflow=build)

<img src="docs/images/logo.png" alt="Speedy Structured logging library" width="280" align="right" />

**Speedy Structured C++17 logging library**

To achieve 30 million logs per second, string information is pre-processed at compile-time so that practically only arguments are stored at run-time.

Furthermore, compact storage is obtained by writing duplicate strings only once, in binary form and with generalized delta-encoding.

Costly formatting is deferred to logs exploitation phase, enabling dynamic filtering, queries and transforms: <br>
Logs act as a tiny database.

The flexible query interface is available in shell, python or C++, and is suitable for

 - Standard log display
 - Deep log analysis
 - Value plotting
 - Verification of behavior during testing

<br>
In including just one header file.

## Features

 - Single header library
 - No dependencies
 - Nanosecond scale [performance](#benchmarks)
 - Compact binary storage
   - Several times smaller than text storage
   - Even smaller: support of transparent post-compression with Zstandard
 - [Detailed logs](#details-on-demand) upon request with a configurable time radius (covering also before)
   - Tailored to capture sensitive periods and their cause
 - Easy structured logs in a printf-like interface
   - Arguments are typed, with optional name and unit
 - Support of [binary buffer](#binary-buffer-logging) logging
 - Rotating files
 - Thread-safe
 - Synchronous console logging
 - Asynchronous file storage
 - Compile time selection of [groups of logs](#selection-of-groups-of-log-at-compile-time)
 - Simple and [powerful configuration](#log-notification-callback)
 - Crash friendly, stack trace appended and logs flushed
 - Post-processing batteries included
   - `sscat`: `cat`-like [tool](https://dfeneyrou.github.io/sslog/sscat.md.html) with filters and transforms
   - `libsslogread`: [C++ library](https://dfeneyrou.github.io/sslog/sslogread_cpp_api.md.html) for reading logs
   - Python `sslogread` module: [wrapper](https://dfeneyrou.github.io/sslog/sslogread_python_module.md.html) for libsslogread
 - Support of UTC and local time
 - Linux & Windows support

## Install

Copy and include the single header `sslog.h`. <br>
Then start logging!

Optionally, the python reader module can be easily installed with `pypi`:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ shell
python3 -m pip install sslogread
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Benchmarks

Raw results from the internal benchmark (run `test/run_suite.py -s performance`):
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
==============================================================================
| sslog - Logging runtime 0 param             | 26.0 ns or 38.46 Mlog/s      |
| sslog - Logging runtime 1 param int         | 28.3 ns or 35.39 Mlog/s      |
| sslog - Logging runtime 1 param string      | 33.2 ns or 30.11 Mlog/s      |
| sslog - Logging runtime 4 params int        | 33.9 ns or 29.51 Mlog/s      |
| sslog - Logging runtime skipped log         | 0.1 ns or 13543.67 Mlog/s    |
|                                             |                              |
| sslog - compactness - full text             | 100% (1e6 logs / 151.9 MB)   |
| sslog - compactness - sslog         vs text | 11.9 %                       |
| sslog - compactness - sslog+zstd -5 vs text | 3.0 %                        |
|                                             |                              |
| sslog - Header include                      | 0.703 s                      |
| sslog - Header include - SSLOG_DISABLE=1    | 0.243 s                      |
|                                             |                              |
| sslog - Library code size (-O2)             | 45208 bytes                  |
| sslog - Log code size 0 param (-O2)         | 60 bytes/log                 |
| sslog - Log code size 1 param int (-O2)     | 80 bytes/log                 |
| sslog - Log code size 1 param string (-O2)  | 72 bytes/log                 |
| sslog - Log code size 4 params int (-O2)    | 112 bytes/log                |
|                                             |                              |
| sslog - Log compilation speed (-O0)         | 6578 log/s                   |
| sslog - Log compilation speed (-O2)         | 914 log/s                    |
==============================================================================
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Highlights:
 - More than **30 million log/s**
 - Storage **10x times more compact** than text (30x with `zstd -5`) <br>
   Important: these ratio varies with the logs dynamic and content (formatter, argument quantity and values) <br>
   Note: these lab figures are too optimistic. Measures on real applications rather show 5x (and 15x) more compact storage.
 - Compilation time cost per unit is around 0.7 s
   - This cost is due in part to the internal usage of standard libraries (chrono, threads, etc...) <br>
     It is amortized if standard libraries are required also in the including file.

<details>
<summary>Benchmark setup</summary>

 - hardware: laptop with a Core I7-7600U, g++ 13.3.0
 - Configuration:
    - Store on disk only, large collection buffer
    - Compactness evaluated on 4 threads looping 10000 times on 25 different logs with varying arguments
 - Runtime performance measured on 50 millions logs up to disk storage
</details>

<details>
<summary> See comparison with spdlog </summary>

Raw results from the internal benchmark (run `test/run_suite.py -s performance_spdlog` with `spdlog-dev` installed):
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
==============================================================================
| spdlog - Logging runtime 0 param            | 344.3 ns or 2.90 Mlog/s      |
| spdlog - Logging runtime 1 param int        | 349.0 ns or 2.87 Mlog/s      |
| spdlog - Logging runtime 1 param string     | 350.1 ns or 2.86 Mlog/s      |
| spdlog - Logging runtime 4 params int       | 410.6 ns or 2.44 Mlog/s      |
| spdlog - Logging runtime skipped log        | 2.5 ns or 392.63 Mlog/s      |
|                                             |                              |
| spdlog - Header include                     | 3.035 s                      |
|                                             |                              |
| spdlog - Library code size (-O2)            | 241840 bytes (+ shared libs) |
| spdlog - Log code size 0 param (-O2)        | 20 bytes/log                 |
| spdlog - Log code size 1 param int (-O2)    | 56 bytes/log                 |
| spdlog - Log code size 1 param string (-O2) | 56 bytes/log                 |
| spdlog - Log code size 4 params int (-O2)   | 144 bytes/log                |
|                                             |                              |
| spdlog - Log compilation speed (-O0)        | 11008 log/s                  |
| spdlog - Log compilation speed (-O2)        | 2447 log/s                   |
==============================================================================
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Highlights:
 - Performances are **10x slower** than `sslog` mainly due to the online formatting
 - Output is unstructured immutable text. Format specifiers and long strings directly influence the compactness.
 - Compilation time cost per unit is heavy, around 3.0 s (maybe also amortized)

<details>
<summary>Benchmark setup</summary>

 - `spdlog` version 1.12 (from Ubuntu 24.04.2 LTS), compiled version
 - Configuration similar to `sslog`:
   - Asynchronous logger, thread-safe sink `basic_file_sink_mt`
   - 1 thread pool with queue size = 65535, `block` overflow policy, no console
 - Runtime performance measured on 1 million logs up to storage
</details>

</details>

## Examples

### Basic logging with names and units

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C++
#include "sslog.h"

int main()
{
    ssInfo("myApp/main", "First message on console with sslog!");
    ssError("myApp/main", "First error with parameters: %d %-8s %f %08lx",
            14, "stream", 3.14, 1234567890UL);

    // The optional text before and after an argument is interpreted as name and unit
    // The category (first string) aims typically user-specific classification
    ssDebug("/animal/pet/dog", "surname=%s weight=%4.1f_kg", "mirza", 12.856);
    ssTrace("/animal/pet/cat", "surname=%s weight=%4.1f_kg", "misty", 5.5);
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

See [here](https://dfeneyrou.github.io/sslog/logging_reference_api.md.html#loggingapi) for details on the logging API.

### Details on demand

A unique feature is the logging of details upon request.

In the example below:
 - Levels equal or above `info` are always logged
 - Levels equal or above `trace` are logged only around the moment when "details are requested" <br>
   This period spreads **before** and after the time of request.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C++
#include "sslog.h"

int main()
{
    // Configure the level of the details and the file split rules
    sslog::Sink config;
    config.storageLevel = sslog::Level::info;  // Standard logging at "info" level
    config.detailsLevel = sslog::Level::trace; // On request, enables details up to 'trace' (for storage)
    config.fileMaxBytes = 10000;               // Granularity of 10 KB
    ssSetSink(config);

    for (int iter = 0; iter < 100000; ++iter)
    {
        ssInfo("Always logged and stored");
        ssDebug("Logged and stored only around the iteration 50000. Current iteration is %05d", iter);

        // Request details up to 'trace' before and after this particular moment
        // The duration of the surrounding context capture is configurable
        if (iter == 50000) ssRequestForDetails();
    }
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### Query with "sscat" shell tool

Display all the logs (no filtering) in text:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ shell
sscat <log dir>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Select the logs on a category matching `*engine*` but not on a thread matching `worker*`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ shell
sscat <log dir> -nt "worker*" -c "*engine*"
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Select the logs on category `transaction` and which possess an argument named `id` which has a value higher than 356 and lower or equal to 1000
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ shell
sscat <log dir> -c "transaction" -a "id>356" -a "id<=1000"
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

See [here](https://dfeneyrou.github.io/sslog/sscat.md.html) for details.

### Structured output in JSON with "sscat" shell tool

The option `-j` switches to the JSON output:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ shell
sscat <log dir> -j
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### Query in Python

Display all the logs (no filtering) in text:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Python
import sslogread
session = sslogread.load("/path/to/my/log_folder")

result = session.query()
for log in result:
   print("%d) %s" % (log['timestampUtcNs'], log['format'])) # Simple display
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Select the logs on a category matching `*engine*` but not on a thread matching `worker*`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Python
result = session.query( { 'no_thread': "worker*", 'category':'*engine*' } )
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Select the logs on category `transaction` and which possess an argument named `id` which has a value higher than 356 and lower or equal to 1000
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Python
result = session.query( { 'category': "transaction", 'arguments': ["id>356", "id<=1000"] } )
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

See [here](https://dfeneyrou.github.io/sslog/sslogread_python_module.md.html#sslogread.logsession.query) for details on the Python reader module.

### Query in C++

Display all the logs (no filtering) in text:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C++
std::string errorMessage;
sslogread::LogSession session;
if(!session.init("/path/to/my/log/folder", errorMessage)) {
    fprintf(stderr, "Error: %s\n", errorMessage.c_str());
    exit(1)
}

if(!session->query({}, [session](const sslogread::LogStruct& log) {

                 // Format the string with arguments (=custom vsnprintf with our argument list, see below)
                 char filledFormat[1024];
                 sslogread::vsnprintf(filledFormat, sizeof(filledFormat), session->getIndexedString(log.formatIdx), log.args, session);

                 // Some simple display on console (there are better ways)
                 printf("[timestampUtcNs=%lu  thread=%s  category=%s  buffer=%s] %s\n",
                     log.timestampUtcNs, session->getIndexedString(log.threadIdx), session->getIndexedString(log.categoryIdx),
                     log.buffer.empty()? "No" : "Yes", filledFormat);

             },
             errorMessage)
{
    fprinf(stderr, "Error: %s\n", errorMessage.c_str());
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Select the logs on a category matching `*engine*` but not on a thread matching `worker*`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C++
Rule rule;
rule.category = "*engine*";
rule.noThread = "worker*";

std::string errorMessage;
if(!session.query({rule}, [](const sslogread::LogStruct& log) { /* ... */ }, errorMessage)
{
    fprinf(stderr, "Error: %s\n", errorMessage.c_str());
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Select the logs on category `transaction` and which possess an argument named `id` which has a value higher than 356 and lower or equal to 1000
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C++
Rule rule;
rule.category = "transaction";
rule.arguments.push_back("id>356");   // Note: comparison works also with strings (alphanumerically)
rule.arguments.push_back("id<=1000");

/* ... */
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

See [here](https://dfeneyrou.github.io/sslog/sslogread_cpp_api.md.html#sslogreadc++api/logsession::query) for details on the C++ reader API.

### Binary buffer logging

The `printf` interface provides built-in format checks,is wildly known and is quite expressive. <br>
In this cotext, inserting support of custom structure cannot be easily supported.

An exception is made for the valuable case of generic binary buffers (dumps, images, packets, CAN messages...).

The API is similar to `sslog` standard logging with the suffix "Buffer" and with additional arguments: buffer pointer and size before the format string.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C++
#include "sslog.h"
#include <vector>

int main()
{
    ssSetConsoleLevel(sslog::Level::trace);

    std::vector<uint8_t> buffer{0xDE, 0xAD, 0xBE, 0xEF};

    ssDebugBuffer("myApp/archive", buffer.data(), buffer.size(),
                  "Standard log with a binary buffer attached. Its sweet name is %s, id:%d",
                  "Francis", 314);
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

See [here](https://dfeneyrou.github.io/sslog/logging_reference_api.md.html#loggingreferenceapi/loggingapi/binarybuffers) for details.

### Log notification callback

A custom log handler, or some special processing on high level logs? <br>
Simply register a callback and set the start level.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C++
#include "sslog.h"

int main()
{
    sslog::Sink config;
    config.liveNotifLevel = sslog::Level::critical;
    config.liveNotifCbk = [] (uint64_t timestampUtcNs, uint32_t level, const char* threadName, const char* category,
                              const char* logContent, const uint8_t* binaryBuffer, uint32_t binaryBufferSize)
                              {
                                  sendEmergencyMail("support@world.com", timestampUtcNs, category, logContent);
                              };
    ssSetSink(config);
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### Multisink with fully configurable console display

The outcome of the logging process is easy to configure with [`ssSetSink()`](https://dfeneyrou.github.io/sslog/configuration_reference_api.md.html#loggingconfiguration/sssetsink):
 - name of the log folder
 - log level for console (default is `info`)
 - log level for disk (default is `off `)
 - the formatter of the console display
 - the level for details storage feature
 - the level for live notification and callback
 - the file split and rotation
 - ...

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C++
{
    // Always thread-safe, asynchronous storage, synchronous console display and live notifications
    sslog::Sink config;
    config.pathPattern = "logdir";
    config.storageLevel = sslog::Level::trace;
    config.consoleLevel = sslog::Level::debug;
    config.detailsLevel     = sslog::Level::off;
    config.liveNotifLevel   = sslog::Level::error;
    config.liveNotifCbk     = myEmergencyLogProcessCallback;
    config.consoleFormatter = "[%L] [%Y-%m-%dT%H:%M:%S.%f%z] [%c] [thread %t] %v%Q";
    ssSetSink(config);

    ssTrace("myApp/main", "Message stored on disk but not displayed on console.");
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Also, the log path may contain some date formatters:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C++
{
    // Date formatters can be used on the log path
    sslog::Sink config;
    config.path = "logdir-%y%m%d-%HH%M_%S"; // Ex: logdir-250619-08H15_54
    ssSetSink(config);
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

See [here](https://dfeneyrou.github.io/sslog/configuration_reference_api.md.html#loggingconfiguration/sssetsink) for details on the logger configuration API.

### Rotating files

No logger is complete without manipulation on the log files splitting and counting:
 - Split on size
 - Split on duration
 - Limit on quantity of files
 - Limit on age of files

The [`ssSetSink()`](https://dfeneyrou.github.io/sslog/configuration_reference_api.md.html#loggingconfiguration/sssetsink) API is still the one to use:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C++
{
    sslog::Sink config;
    config.path = "logdir";
    config.fileMaxBytes = 5 * 1024*1024; // Rotate after reaching 5 MB
    config.fileMaxDurationSec = 600;     // Or rotate after 10 mn
    config.fileMaxQty = 10;              // Keep the last 10 files
    config.fileMaxFileAgeSec = 3600;     // Keep only files more recent than 1 hour
    ssSetSink(config);
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### Selection of groups of log at compile-time

Some logs in some conditions may not be desirable, whatever their level.

Logs can be grouped and activated at compile time only when needed, with zero-cost when disabled. <br>
Each logging API has a "group" variant, easily identified by the `ssg` prefix and the group name as first parameter:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C++
// To define somewhere in the current file, a header, in the build system...
#define SSLOG_GROUP_VERY_LOW_LEVEL 0    # One group of logs (disabled)
#define SSLOG_GROUP_BINARY_DUMPS 1      # Another group of logs (enabled)

// When a group name is used, the prefix SSLOG_GROUP_ is not present

ssgError(VERY_LOW_LEVEL, "myApp/main", "First error with parameters: %d %-8s %f %08lx", 14, "stream", 3.14, 1234567890UL);

std::vector<uint8_t> buffer{0xDE, 0xAD, 0xBE, 0xEF};
ssgDebugBuffer(BINARY_DUMPS, "myApp/archive/dump", buffer.data(), buffer.size(),
             "Dump of received packet number=%d from client:%s_id", 314, "George");
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

See [here](https://dfeneyrou.github.io/sslog/logging_reference_api.md.html#compile-timeopt-inofgroupsoflogs) for details.

## FAQ

<details>
<summary> How can I further reduce the log storage size with `zstd`? </summary>

Once the logging session is finished, use for instance:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ shell
zstd --rm -r -5 sslogDb
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The role of the options are:
 - `-r` (recursive) ensures that each file in the folder are processed
 - `--rm` removes the original files, once the compression is successful (safe)
 - `-5` is the compression level, trade-off between speed and final size. It is adjusted from `-1` (very fast) to `-19` (high compression).

If built with the `zstd-dev` package installed on the system, reader tools transparently read compressed log files.

</details>

<details>
<summary> I do not see the logged stracktrace when a crash occurs... </summary>

Logging the stack trace requires two conditions:
 - presence of the stacktrace libraries (native on Windows, `libunwind-dev` and `libdw-dev` on Linux)
 - presence of the SSLOG_STACKTRACE=1 directive as a compilation flag (for *all* compilation units)
</details>

## Dependencies

**The repository does not require any external dependencies.**

Details on internal dependencies are:
 - The single-header logger part of `sslog` uses only C++17 standard libraries.
 - The C++ `sslogread` library internally uses a modified version of the vsnprintf code from [stb library](https://github.com/nothings/stb)  (Public domain)
 - The reader Python module requires Python3.7 or above.
 - The documentation internally uses a modified version of [Markdeep](https://casual-effects.com/markdeep) (BSD license), which turns a markdown file into a web page. Just look at the source of this page if that sounds interesting.

Optional dependencies:
 - On linux, if `libdw` and `libunwind` libraries are installed and the compilation flag SSLOG_STACKTRACE=1 is positioned, the stacktrace is logged when a crash occurs
 - if `libzstd` library is installed, `sslogread` library reads and decompresses any .zst log files in a transparent manner.

## License

Released under the [MIT license](LICENSE)
