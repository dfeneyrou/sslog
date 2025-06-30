// The MIT License (MIT)
//
// Copyright(c) 2025, Damien Feneyrou <dfeneyrou@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)  // Disable Windows's secure API warnings
#endif

// Speedy Structured C++17 logging library
//
//  - Single header library
//  - No dependencies
//  - Very high performance
//    - Nanosecond scale
//  - Logs are structured and queryable
//    - With fields type, name, and units
//    - Can be used as part of a test suite
//  - Rotating files
//  - On-demand detailed logs
//  - Compact binary storage
//  - Simple printf-like interface, extended with binary buffers
//  - Thread-safe
//  - Synchronous console logging (colors supported)
//  - Asynchronous file storage in one dedicated thread.
//  - Simple but powerful configuration
//  - Crash friendly, stack trace is appended and logs are flushed
//  - Comes with tooling
//    - `sscat`: Cat tool with filtering capabilities and JSON output
//    - `libsslogread`: C++ library for logs reading
//    - `sslog` module: Python wrapper for libsslogread

// =============================================
// Library configuration
// =============================================

// Define this flag to fully disable 'sslog' at compile time
#ifndef SSLOG_DISABLE

// IMPORTANT: the following compilation flags must be consistent in all inclusions of this header:
// ===============================================================================================
//  - either pass the flags from build system
//  - either modify this file (easy but software update will be more cumbersome)
//  - either encapsulate this header in another one, with the customized flags

// Set this flag to 1 to disable the printf format check.
// About "fmt support":
//   This flag is useful if you use the 'fmt'-like formatting style based on brackets.
//   Caution: 'fmt' is not directly supported. Only empty brackets '{}' is compatible, and
//            only disk storage and ulcat will work properly, not the console display.
//   The format specification inside brackets differs and must follow the printf specification (ex: {5.2}, {-08X} ...).
//   In this case, the type is inferred from the one of the argument.
//   Bonus: std::string and std::string_view are supported in this case.
//   Note: '{' is escaped with double opening brackets ('{{') . Closing bracket does not requires escaping.
#ifndef SSLOG_NO_PRINTF_FORMAT_CHECK
#define SSLOG_NO_PRINTF_FORMAT_CHECK 0
#endif

// Set this flag to 1 to disable the auto start. Note that it must be done for all includes
#ifndef SSLOG_NO_AUTOSTART
#define SSLOG_NO_AUTOSTART 0
#endif

// Enable installing some signal (ABRT, FPE, ILL, SEGV, TERM and INT (if SSLOG_CATCH_SIGINT is 1) ) handlers
//  If enabled, last collected data before crash will be flushed, which helps further investigation.
//  Default is enabled.
#ifndef SSLOG_NO_CATCH_SIGNALS
#define SSLOG_NO_CATCH_SIGNALS 0
#endif

// Enable installing SIGINT signal (applicable only if SSLOG_NO_CATCH_SIGNALS is 0)
//  Default is enabled.
#ifndef SSLOG_NO_CATCH_SIGINT
#define SSLOG_NO_CATCH_SIGINT 0
#endif

// Stacktrace logging when a crash occurs
//   On linux, stack trace logging is disabled by default as it requires libunwind.so (stack unwinding) and libdw.so from elfutils (elf and
//   DWARF infos reading)
//    (apt install libunwind-dev libdw-dev)
//   On Windows, stacktrace logging is enabled by default as base system libraries cover the requirements.
//   Note 1: The executable shall contain debug information. If no debug information is present, the stacktrace will just be a list of
//   addresses.
//          If the non-stripped executable version is available, they can be manually decoded with 'addr2line' or equivalent tool.
//   Note 2: The "external strings" feature has no effect on the stacktrace logging, as dynamic strings are used.
#ifndef SSLOG_STACKTRACE
#if defined(_MSC_VER)
#define SSLOG_STACKTRACE 1
#else
#define SSLOG_STACKTRACE 0
#endif
#endif

// **Internal use only**. This virtualizes the time, enabling quick testing of time related features
#ifndef SSLOG_VIRTUAL_TIME_FOR_TEST_ONLY
#define SSLOG_VIRTUAL_TIME_FOR_TEST_ONLY 0
#endif

// =============================================
// Includes
// =============================================

#if defined(_MSC_VER)
// Windows base header (hard to avoid this include...)
#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers. If it is a problem, just comment this line
#include <windows.h>
#endif  // if defined(_MSC_VER)

#include <cinttypes>  // Platform independent printf for integers (PRI64...)
#include <condition_variable>
#include <csignal>  // For raising signals in crash handler
#include <cstddef>  // For size_t
#include <cstdint>  // For sized types like uintXXX_t
#include <cstdlib>  // For abort(), quick_exit...
#include <mutex>
#include <string>

#ifdef __unix__
#if defined(__x86_64__)
#include <x86intrin.h>  // rdtsc
#endif
#endif

#include <atomic>   // Lock-free thread safety is done with atomics
#include <cassert>  // For static_assert()
#include <chrono>   // For time_since_epoch()
#include <cstdarg>  // For va_list used in logs
#include <cstdio>   // For snprintf etc...
#include <cstring>  // For string copy (dynamic strings)
#include <thread>
#if defined(__unix__)
#include <sys/syscall.h>
#include <unistd.h>  // For syscall(SYS_gettid)
#endif
#if defined(_MSC_VER)
#include <processthreadsapi.h>  // For GetCurrentThreadId() in the core logging
#endif                          // if defined(_MSC_VER)

// Stack trace
#if SSLOG_STACKTRACE == 1

#if defined(__unix__)
#define UNW_LOCAL_ONLY
#include <cxxabi.h>            // For demangling names
#include <elfutils/libdwfl.h>  // Elf reading (need package libdw-dev)
#include <libunwind.h>         // Stack unwinding (need package libunwind-dev)
#endif                         // if defined(__unix__)

#if defined(_MSC_VER)
#include <dbghelp.h>         // For the symbol decoding
#include <errhandlingapi.h>  // For the HW exceptions
#pragma comment(lib, "DbgHelp.lib")
#endif  // if defined(_MSC_VER)

#endif  // if SSLOG_STACKTRACE==1

#endif  // ifndef SSLOG_DISABLE

#ifndef SSLOG_EXPERIMENTAL_FILESYSTEM
#include <filesystem>
#define SSLOG_FS_NAMESPACE std::filesystem
#else
// For old gcc only
#include <experimental/filesystem>
#define SSLOG_FS_NAMESPACE std::experimental::filesystem
#endif
#include <functional>
#include <vector>

// This line is unfortunately the only way found to remove the zero-arguments-variadic-macro and the
// prohibited-anonymous-structs warnings with GCC when the build is using the option -pedantic
#if !defined(_MSC_VER)
#pragma GCC system_header
#endif

namespace sslog
{

// =============================================
// Public structures, always declared
// =============================================

enum class Level { trace = 0, debug = 1, info = 2, warn = 3, error = 4, critical = 5, off = 6 };

enum class ConsoleMode { Off, Mono, Color };

enum class SaturationPolicy { Drop, Wait };

using LiveNotifCbk = std::function<void(uint64_t timestampUtcNs, uint32_t level, const char* threadName, const char* category,
                                        const char* logContent, const uint8_t* binaryBuffer, uint32_t binaryBufferSize)>;
struct Collector {
    // Size of the new string collection buffer between 2 async flushs (2 buffers instanciated)
    uint32_t stringBufferBytes = 100000;

    // Size of the data collection buffer between 2 async flushs (4 buffers instanciated)
    uint32_t dataBufferBytes = 1000000;

    // Maximum latency of buffer flushing
    double flushingMaxLatencyMs = 1000.;

    // Policy when the data buffers are saturated (which means bad dimensioning BTW)
    // - 'Wait' ensures the log integrity, at the price of possible slow-down of the logging threads
    // - 'Drop' ensures constant log duration in logging threads, at the price of the content integrity
    // Note: strings are always in 'wait' policy
    SaturationPolicy dataSaturationPolicy = SaturationPolicy::Wait;
};

struct Sink {
    // Console
    Level       consoleLevel     = Level::info;
    ConsoleMode consoleMode      = ConsoleMode::Color;
    std::string consoleFormatter = "[%L] [%Y-%m-%dT%H:%M:%S.%f%z] [%c] [thread %t] %v%Q";
    bool        useUTCTime       = false;  // Default is localtime

    // Live notification
    Level        liveNotifLevel = Level::off;
    LiveNotifCbk liveNotifCbk;

    // Storage
    std::string path;                                   // No storage if the pattern is empty. Date formatters allowed (ex: "log-%H%M%S")
    Level       storageLevel            = Level::info;  // No storage if level is 'off'
    uint64_t    splitFileMaxBytes       = 0;            // 0 means no file split based on size
    uint32_t    splitFileMaxDurationSec = 0;            // 0 means no file split based on duration
    uint32_t    fileMaxQty              = 0;            // Maximum quantity of rolling files. 0 means no limit
    uint32_t    fileMaxFileAgeSec       = 0;            // Older files are removed. 0 means no age limit

    // Storage for details
    // This feature keeps log files on disk only around moments a request for details is raised.
    // The details level must be strictly lower than the disk storage leve for the feature to be active
    // Note: the details will be kept on a data file basis, so data file split shall be configured
    Level    detailsLevel             = Level::off;
    uint32_t detailsBeforeAfterMinSec = 5;  // Garanteed time for keeping details before and after the request
};

struct Stats {
    uint32_t storedStrings;
    uint32_t storedLogs;
    uint32_t storedBytes;
    uint32_t droppedLogs;
    uint32_t delayedLogs;
    uint32_t delayedStrings;

    uint32_t maxUsageDataBufferBytes;    // Maximum used size in the collection data buffer
    uint32_t maxUsageStringBufferBytes;  // Maximum used size in the collection string buffer

    uint32_t createdDataFiles;
    uint32_t removedDataFiles;

    uint32_t requestForDetailsQty;
};

// =============================================
// Disable 'sslog' at compile time: stubs
// =============================================

#ifdef SSLOG_DISABLE

#define ssSetCollector(config_)           (void)(config_)
#define ssSetSink(config_)                (void)(config_)
#define ssSetStoragePath(path_)           (void)(path_)
#define ssSetStorageLevel(level_)         (void)(level_)
#define ssSetConsoleLevel(level_)         (void)(level_)
#define ssSetConsoleFormatter(formatter_) (void)(formatter_)
#define ssGetCollector()                  sslog::Collector()
#define ssGetSink()                       sslog::Sink()
#define ssRequestForDetails()
#define ssGetStats() \
    sslog::Stats {}
#define ssStart()
#define ssStop()
#define ssIsEnabled(level_)          false
#define ssgIsEnabled(group_, level_) false
#define ssSetThreadName(name_)       (void)(name_)

// Empty macros
#define ssEmptyImpl_() \
    do {               \
    } while (0)
#define ssTrace(category_, format_, ...)             ssEmptyImpl_()
#define ssgTrace(group_, category_, format_, ...)    ssEmptyImpl_()
#define ssDebug(category_, format_, ...)             ssEmptyImpl_()
#define ssgDebug(group_, category_, format_, ...)    ssEmptyImpl_()
#define ssInfo(category_, format_, ...)              ssEmptyImpl_()
#define ssgInfo(group_, category_, format_, ...)     ssEmptyImpl_()
#define ssWarn(category_, format_, ...)              ssEmptyImpl_()
#define ssgWarn(group_, category_, format_, ...)     ssEmptyImpl_()
#define ssError(category_, format_, ...)             ssEmptyImpl_()
#define ssgError(group_, category_, format_, ...)    ssEmptyImpl_()
#define ssCritical(category_, format_, ...)          ssEmptyImpl_()
#define ssgCritical(group_, category_, format_, ...) ssEmptyImpl_()

#define ssTraceBuffer(category_, buffer_, format_, ...)             ssEmptyImpl_()
#define ssgTraceBuffer(group_, category_, buffer_, format_, ...)    ssEmptyImpl_()
#define ssDebugBuffer(category_, buffer_, format_, ...)             ssEmptyImpl_()
#define ssgDebugBuffer(group_, category_, buffer_, format_, ...)    ssEmptyImpl_()
#define ssInfoBuffer(category_, buffer_, format_, ...)              ssEmptyImpl_()
#define ssgInfoBuffer(group_, category_, buffer_, format_, ...)     ssEmptyImpl_()
#define ssWarnBuffer(category_, buffer_, format_, ...)              ssEmptyImpl_()
#define ssgWarnBuffer(group_, category_, buffer_, format_, ...)     ssEmptyImpl_()
#define ssErrorBuffer(category_, buffer_, format_, ...)             ssEmptyImpl_()
#define ssgErrorBuffer(group_, category_, buffer_, format_, ...)    ssEmptyImpl_()
#define ssCriticalBuffer(category_, buffer_, format_, ...)          ssEmptyImpl_()
#define ssgCriticalBuffer(group_, category_, buffer_, format_, ...) ssEmptyImpl_()

#endif  // ifdef SSLOG_DISABLE

// ==============================================
// Enabled 'sslog'
// ==============================================

#ifndef SSLOG_DISABLE

#define ssSetCollector(config_)           sslog::priv::setCollector(config_)
#define ssSetSink(config_)                sslog::priv::setSink(config_)
#define ssSetStoragePath(path_)           sslog::priv::setStoragePath(path_)
#define ssSetStorageLevel(level_)         sslog::priv::setStorageLevel(level_)
#define ssSetConsoleLevel(level_)         sslog::priv::setConsoleLevel(level_)
#define ssSetConsoleFormatter(formatter_) sslog::priv::setConsoleFormatter(formatter_)
#define ssGetCollector()                  sslog::priv::getCollector()
#define ssGetSink()                       sslog::priv::getSink()
#define ssRequestForDetails()             sslog::priv::requestForDetails()
#define ssGetStats()                      sslog::priv::getStats()
#define ssStart()                         sslog::priv::start()
#define ssStop()                          sslog::priv::stop()
#define ssSetThreadName(name_)            sslog::priv::setThreadName(name_)
#define ssIsEnabled(level_)               (sslog::priv::gc.enabled && level_ >= sslog::priv::gc.minLoggingLevel)
#define ssgIsEnabled(group_, level_)      (SSG_IS_COMPILE_TIME_ENABLED_(group_) && ssIsEnabled(level_))

// Logging
#if defined(__GNUC__) && SSLOG_NO_PRINTF_FORMAT_CHECK != 1
#define SSLOG_PRINTF_FORMAT_CHECK(format_, ...) \
    (void)sizeof(printf(format_, ##__VA_ARGS__)) /* Check consistency of printf arguments. "-Werror=format" is recommended */
#else
#define SSLOG_PRINTF_FORMAT_CHECK(format_, ...)
#endif

#define sslogImpl_(level_, category_, format_, ...)                                                                                  \
    do {                                                                                                                             \
        if (ssIsEnabled(sslog::Level::level_)) {                                                                                     \
            SSLOG_PRINTF_FORMAT_CHECK(format_, ##__VA_ARGS__);                                                                       \
            sslog::priv::log(true, SSLOG_STRINGHASH(format_), SSLOG_STRINGHASH(category_), format_, category_, sslog::Level::level_, \
                             ##__VA_ARGS__);                                                                                         \
        }                                                                                                                            \
    } while (0)
#define ssgLogImpl_(level_, group_, category_, format_, ...) \
    SSLOG_PRIV_IF(SSG_IS_COMPILE_TIME_ENABLED_(group_), sslogImpl_(level_, category_, format_, ##__VA_ARGS__), do {} while (0))

#define ssTrace(category_, format_, ...)             sslogImpl_(trace, category_, format_, ##__VA_ARGS__)
#define ssgTrace(group_, category_, format_, ...)    ssgLogImpl_(trace, group_, category_, format_, ##__VA_ARGS__)
#define ssDebug(category_, format_, ...)             sslogImpl_(debug, category_, format_, ##__VA_ARGS__)
#define ssgDebug(group_, category_, format_, ...)    ssgLogImpl_(debug, group_, category_, format_, ##__VA_ARGS__)
#define ssInfo(category_, format_, ...)              sslogImpl_(info, category_, format_, ##__VA_ARGS__)
#define ssgInfo(group_, category_, format_, ...)     ssgLogImpl_(info, group_, category_, format_, ##__VA_ARGS__)
#define ssWarn(category_, format_, ...)              sslogImpl_(warn, category_, format_, ##__VA_ARGS__)
#define ssgWarn(group_, category_, format_, ...)     ssgLogImpl_(warn, group_, category_, format_, ##__VA_ARGS__)
#define ssError(category_, format_, ...)             sslogImpl_(error, category_, format_, ##__VA_ARGS__)
#define ssgError(group_, category_, format_, ...)    ssgLogImpl_(error, group_, category_, format_, ##__VA_ARGS__)
#define ssCritical(category_, format_, ...)          sslogImpl_(critical, category_, format_, ##__VA_ARGS__)
#define ssgCritical(group_, category_, format_, ...) ssgLogImpl_(critical, group_, category_, format_, ##__VA_ARGS__)

#define sslogBufferImpl_(level_, category_, buffer_, bufferSize_, format_, ...)                                                      \
    do {                                                                                                                             \
        if (ssIsEnabled(sslog::Level::level_)) {                                                                                     \
            SSLOG_PRINTF_FORMAT_CHECK(format_, ##__VA_ARGS__);                                                                       \
            sslog::priv::logBuffer(SSLOG_STRINGHASH(format_), SSLOG_STRINGHASH(category_), format_, category_, sslog::Level::level_, \
                                   buffer_, bufferSize_, ##__VA_ARGS__);                                                             \
        }                                                                                                                            \
    } while (0)
#define ssgLogBufferImpl_(level_, group_, category_, buffer_, bufferSize_, format_, ...)                                         \
    SSLOG_PRIV_IF(                                                                                                               \
        SSG_IS_COMPILE_TIME_ENABLED_(group_), sslogBufferImpl_(level_, category_, buffer_, bufferSize_, format_, ##__VA_ARGS__), \
        do {} while (0))

#define ssTraceBuffer(category_, buffer_, bufferSize_, format_, ...) \
    sslogBufferImpl_(trace, category_, buffer_, bufferSize_, format_, ##__VA_ARGS__)
#define ssgTraceBuffer(group_, category_, buffer_, bufferSize_, format_, ...) \
    ssgLogBufferImpl_(trace, group_, category_, buffer_, bufferSize_, format_, ##__VA_ARGS__)
#define ssDebugBuffer(category_, buffer_, bufferSize_, format_, ...) \
    sslogBufferImpl_(debug, category_, buffer_, bufferSize_, format_, ##__VA_ARGS__)
#define ssgDebugBuffer(group_, category_, buffer_, bufferSize_, format_, ...) \
    ssgLogBufferImpl_(debug, group_, category_, buffer_, bufferSize_, format_, ##__VA_ARGS__)
#define ssInfoBuffer(category_, buffer_, bufferSize_, format_, ...) \
    sslogBufferImpl_(info, category_, buffer_, bufferSize_, format_, ##__VA_ARGS__)
#define ssgInfoBuffer(group_, category_, buffer_, bufferSize_, format_, ...) \
    ssgLogBufferImpl_(info, group_, category_, buffer_, bufferSize_, format_, ##__VA_ARGS__)
#define ssWarnBuffer(category_, buffer_, bufferSize_, format_, ...) \
    sslogBufferImpl_(warn, category_, buffer_, bufferSize_, format_, ##__VA_ARGS__)
#define ssgWarnBuffer(group_, category_, buffer_, bufferSize_, format_, ...) \
    ssgLogBufferImpl_(warn, group_, category_, buffer_, bufferSize_, format_, ##__VA_ARGS__)
#define ssErrorBuffer(category_, buffer_, bufferSize_, format_, ...) \
    sslogBufferImpl_(error, category_, buffer_, bufferSize_, format_, ##__VA_ARGS__)
#define ssgErrorBuffer(group_, category_, buffer_, bufferSize_, format_, ...) \
    ssgLogBufferImpl_(error, group_, category_, buffer_, bufferSize_, format_, ##__VA_ARGS__)
#define ssCriticalBuffer(category_, buffer_, bufferSize_, format_, ...) \
    sslogBufferImpl_(critical, category_, buffer_, bufferSize_, format_, ##__VA_ARGS__)
#define ssgCriticalBuffer(group_, category_, buffer_, bufferSize_, format_, ...) \
    ssgLogBufferImpl_(critical, group_, category_, buffer_, bufferSize_, format_, ##__VA_ARGS__)

// =============================================
// Internal base macros & helpers
// =============================================

#define SSLOG_STRINGHASH(s) sslog::priv::forceCompileTimeElseError_<sslog::priv::fnv1a_(s, SSLOG_FNV_HASH_OFFSET_)>::compileTimeValue

// Conditional inclusion macro trick
#define SSLOG_PRIV_IF(cond, foo1, foo2)       SSLOG_PRIV_IF_IMPL(cond, foo1, foo2)
#define SSLOG_PRIV_IF_IMPL(cond, foo1, foo2)  SSLOG_PRIV_IF_IMPL2(cond, foo1, foo2)
#define SSLOG_PRIV_IF_IMPL2(cond, foo1, foo2) SSLOG_PRIV_IF_##cond(foo1, foo2)
#define SSLOG_PRIV_IF_0(foo1, foo2)           foo2
#define SSLOG_PRIV_IF_1(foo1, foo2)           foo1

#define SSG_IS_COMPILE_TIME_ENABLED_(group_) SSLOG_GROUP_##group_

// Hash salt in case of collision (very unlikely with 64 bits hash)
#ifndef SSLOG_HASH_SALT
#define SSLOG_HASH_SALT 0
#endif

#define SSLOG_FNV_HASH_OFFSET_ (14695981039346656037ULL + SSLOG_HASH_SALT)
#define SSLOG_FNV_HASH_PRIME_  1099511628211ULL

// Library version
#define SSLOG_VERSION     "0.8.3a"
#define SSLOG_VERSION_NUM 803  // Monotonic number. 100 per version component. Official releases are muliple of 100

// Storage protocol version
#define SSLOG_STORAGE_FORMAT_VERSION 1

// Argument flags on 4 bits
#define SSLOG_DATA_NONE   0
#define SSLOG_DATA_S8     1
#define SSLOG_DATA_U8     2
#define SSLOG_DATA_S16    3
#define SSLOG_DATA_U16    4
#define SSLOG_DATA_S32    5
#define SSLOG_DATA_U32    6
#define SSLOG_DATA_S64    7
#define SSLOG_DATA_U64    8
#define SSLOG_DATA_FLOAT  9
#define SSLOG_DATA_DOUBLE 10
#define SSLOG_DATA_STRING 11
#define SSLOG_DATA_BUFFER 12  // Special handling as size is not fixed
#define SSLOG_DATA_QTY    13

// Header flag: 3 bits for log levels
#define SSLOG_LEVEL_SHIFT    0
#define SSLOG_LEVEL_MASK     7
#define SSLOG_LEVEL_BASE     0
#define SSLOG_LEVEL_TRACE    0
#define SSLOG_LEVEL_DEBUG    1
#define SSLOG_LEVEL_INFO     2
#define SSLOG_LEVEL_WARN     3
#define SSLOG_LEVEL_ERROR    4
#define SSLOG_LEVEL_CRITICAL 5
#define SSLOG_LEVEL_NONE     6
#define SSLOG_LEVEL_QTY      7

// Header flag: 2 bits for the format coding size
#define SSLOG_FORMAT_SHIFT   3
#define SSLOG_FORMAT_MASK    3
#define SSLOG_FORMAT_0_BYTES (0 << SSLOG_FORMAT_SHIFT)
#define SSLOG_FORMAT_1_BYTES (1 << SSLOG_FORMAT_SHIFT)
#define SSLOG_FORMAT_2_BYTES (2 << SSLOG_FORMAT_SHIFT)
#define SSLOG_FORMAT_3_BYTES (3 << SSLOG_FORMAT_SHIFT)

// Header flag: 2 bits for the category coding size
#define SSLOG_CATEGORY_SHIFT   5
#define SSLOG_CATEGORY_MASK    3
#define SSLOG_CATEGORY_0_BYTES (0 << SSLOG_CATEGORY_SHIFT)
#define SSLOG_CATEGORY_1_BYTES (1 << SSLOG_CATEGORY_SHIFT)
#define SSLOG_CATEGORY_2_BYTES (2 << SSLOG_CATEGORY_SHIFT)
#define SSLOG_CATEGORY_3_BYTES (3 << SSLOG_CATEGORY_SHIFT)

// Header flag: 2 bits for the thread coding size
#define SSLOG_THREADIDX_SHIFT   7
#define SSLOG_THREADIDX_MASK    3
#define SSLOG_THREADIDX_0_BYTES (0 << SSLOG_THREADIDX_SHIFT)
#define SSLOG_THREADIDX_1_BYTES (1 << SSLOG_THREADIDX_SHIFT)
#define SSLOG_THREADIDX_2_BYTES (2 << SSLOG_THREADIDX_SHIFT)
#define SSLOG_THREADIDX_3_BYTES (3 << SSLOG_THREADIDX_SHIFT)

// Header flag: 2 bits for the timestamp coding size
#define SSLOG_TS_SHIFT   9
#define SSLOG_TS_MASK    3
#define SSLOG_TS_2_BYTES (0 << SSLOG_TS_SHIFT)
#define SSLOG_TS_3_BYTES (1 << SSLOG_TS_SHIFT)
#define SSLOG_TS_4_BYTES (2 << SSLOG_TS_SHIFT)
#define SSLOG_TS_8_BYTES (3 << SSLOG_TS_SHIFT)

// [Platform specific]
// System thread ID getter, implemented for both Windows and Linux
#ifndef SSLOG_GET_SYS_THREAD_ID
#if defined(__unix__)
#define SSLOG_GET_SYS_THREAD_ID() (uint32_t)(syscall(SYS_gettid))
#endif
#if defined(_MSC_VER)
#define SSLOG_GET_SYS_THREAD_ID() GetCurrentThreadId()
#endif
#endif

namespace priv
{

// Some types
typedef uint64_t clockTick_t;
typedef uint64_t clockNs_t;
typedef uint64_t hashStr_t;

typedef void (*ssSignalHandler_t)(int);
#if defined(_MSC_VER) && SSLOG_STACKTRACE == 1
extern "C" {
typedef unsigned long(__stdcall* rtlWalkFrameChain_t)(void**, unsigned long, unsigned long);
}
#endif

// Redefine our min and max functions due to windows macros
template<class T1, class T2>
T1
sslogMax(T1 a, T2 b)
{
    return (T1)(a > b ? a : b);
}
template<class T1, class T2>
T1
sslogMin(T1 a, T2 b)
{
    return (T1)(a < b ? a : b);
}

// Static string management
// String hash functions is Fowler–Noll–Vo (trade-off between: easy to do at compile time / performances / spreading power)
constexpr hashStr_t
fnv1a_(const char* s, hashStr_t offset)
{
    return (!(*s)) ? offset : fnv1a_(s + 1, (offset ^ ((hashStr_t)(*s))) * SSLOG_FNV_HASH_PRIME_);
}
template<hashStr_t V>
struct forceCompileTimeElseError_ {
    static constexpr hashStr_t compileTimeValue = V;
};

// Internal use only: testing helper
#if SSLOG_VIRTUAL_TIME_FOR_TEST_ONLY != 0
inline clockTick_t ssLogTestVirtualTime = 0x8000000000000000ULL;
void
testIncrementVirtualTimeMs(uint64_t milliseconds)
{
    ssLogTestVirtualTime += milliseconds * 1000000ULL;
}
#endif

// High resolution clock. The effective clock frequency will be calibrated at initialization time
inline clockTick_t
getHiResClockTick()
{
#if SSLOG_VIRTUAL_TIME_FOR_TEST_ONLY == 0
    // Normal behavior
#if defined(_MSC_VER) || defined(__x86_64__)
    return clockTick_t(__rdtsc());
#elif __ARM_ARCH_ISA_A64
    clockTick_t cntvct;
    asm volatile("mrs %0, cntvct_el0; " : "=r"(cntvct)::"memory");
    return cntvct;
#else
    // C++11 standard case (slower but more portable)
    return (clockTick_t)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
#else
    // Testing behavior: virtual time
    return ssLogTestVirtualTime;
#endif
}

// System clock
inline clockNs_t
getUtcSystemClockNs()
{
#if SSLOG_VIRTUAL_TIME_FOR_TEST_ONLY == 0
    // Normal behavior
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#else
    // Testing behavior: virtual time
    return 0x8000000000000000ULL + ssLogTestVirtualTime;
#endif
}

inline const char*
getLevelName(Level l)
{
    constexpr const char* levelStrings[SSLOG_LEVEL_QTY] = {"trace", "debug", "info", "warn", "error", "critical", "off"};
    return levelStrings[sslogMin((uint32_t)l, (uint32_t)(SSLOG_LEVEL_QTY - 1))];
}

inline Level
getLevelFromName(const std::string& name)
{
    if (name == "trace") { return Level::trace; }
    if (name == "debug") { return Level::debug; }
    if (name == "info") { return Level::info; }
    if (name == "warn" || name == "warning") { return Level::warn; }
    if (name == "error") { return Level::error; }
    if (name == "critical") { return Level::critical; }
    return Level::off;
}

// Simple flat hashmap with linear open addressing.
// Requirements: simple, fast, with few allocations, and specialized for our string->index problem:
//   no deletion, key=hash, hash is never zero, hash is 'well spread enough' to avoid clusters,
//   value is a trivially copyable structure, we never insert twice the same key, table size is a power of two
template<class T>
class FlatHashTable
{
   public:
    FlatHashTable(int size = 1024)
    {  // This initial value should allow a control on the potential reallocation
        int sizePo2 = 1;
        while (sizePo2 < size) sizePo2 *= 2;
        rehash(sizePo2);
    }  // Start with a reasonable size (and 32 KB on a 64 bits platform)
    ~FlatHashTable() { delete[] _nodes; }
    int  size() const { return _size; }
    bool empty() const { return (_size == 0); }
    void clear()
    {
        _size = 0;
        for (int i = 0; i < _maxSize; ++i) _nodes[i].hash = 0;
    }
    void insert(hashStr_t hash, T value)
    {
        unsigned int idx = (unsigned int)hash & _mask;
        while (_nodes[idx].hash) idx = (idx + 1) & _mask;  // Always stops because load factor < 1
        _nodes[idx].hash  = hash;                          // Never zero, so "non empty"
        _nodes[idx].value = value;
        _size += 1;
        if (_size * 3 > _maxSize * 2) rehash(2 * _maxSize);  // Max load factor is 0.66
    }
    bool find(hashStr_t hash, T& value) const
    {
        unsigned int idx = (unsigned int)hash & _mask;
        while (1) {  // Always stops because load factor <= 0.66
            if (_nodes[idx].hash == hash) {
                value = _nodes[idx].value;
                return true;
            }
            if (_nodes[idx].hash == 0) return false;  // Empty node
            idx = (idx + 1) & _mask;
        }
        return false;  // Never reached
    }
    bool exist(hashStr_t hash) const
    {
        unsigned int idx = (unsigned int)hash & _mask;
        while (1) {  // Always stops because load factor <= 0.66
            if (_nodes[idx].hash == hash) return true;
            if (_nodes[idx].hash == 0) return false;  // Empty node
            idx = (idx + 1) & _mask;
        }
        return false;  // Never reached
    }
    bool replace(hashStr_t hash, const T& newValue)
    {
        unsigned int idx = (unsigned int)hash & _mask;
        while (1) {  // Always stops because load factor <= 0.66
            if (_nodes[idx].hash == hash) {
                _nodes[idx].value = newValue;
                return true;
            }
            if (_nodes[idx].hash == 0) return false;  // Empty node
            idx = (idx + 1) & _mask;
        }
        return false;  // Never reached
    }
    void rehash(int maxSize)
    {
        int   oldSize = _maxSize;
        Node* old     = _nodes;
        _nodes        = new Node[maxSize];  // 'hash' are set to zero (=empty)
        _maxSize      = maxSize;
        _mask         = (unsigned int)(maxSize - 1);
        _size         = 0;
        for (int i = 0; i < oldSize; ++i) {  // Transfer the previous filled nodes
            if (old[i].hash == 0) continue;
            insert(old[i].hash, old[i].value);
        }
        delete[] old;
    }

   private:
    struct Node {
        hashStr_t hash = 0;  // Hash and key are the same here
        T         value;
    };
    Node*        _nodes   = 0;
    unsigned int _mask    = 0;
    int          _size    = 0;
    int          _maxSize = 0;
    FlatHashTable(const FlatHashTable& other);      // To please static analyzers
    FlatHashTable& operator=(FlatHashTable other);  // To please static analyzers
};

class TimeConverter
{
   public:
    void init(double tickToNs, clockNs_t utcSystemClockOriginNs, clockTick_t steadyClockOriginTick)
    {
        _tickToNs = tickToNs;
        updateSync(utcSystemClockOriginNs, steadyClockOriginTick);
    }

    inline void updateSync(clockNs_t utcSystemClockOriginNs, clockTick_t steadyClockOriginTick)
    {
        _utcSystemClockOriginNs = utcSystemClockOriginNs;
        _steadyClockOriginTick  = steadyClockOriginTick;
    }

    inline clockNs_t getUtcNs(clockTick_t tick) const
    {
        return _utcSystemClockOriginNs + (clockNs_t)(_tickToNs * (double)((int64_t)(tick - _steadyClockOriginTick)));
    }

    inline clockTick_t durationNsToTick(double durationNs) const { return (clockTick_t)(durationNs / _tickToNs); }

    inline double getTickToNs() const { return _tickToNs; }

   private:
    double      _tickToNs               = 1.;
    clockNs_t   _utcSystemClockOriginNs = 0;
    clockTick_t _steadyClockOriginTick  = 0;
};

class TextFormatter
{
   public:
    void init(const char* formatterPattern, bool withUtcTime, bool withColor, clockNs_t recordStartUtcNs)
    {
        if (!_isTimezoneInitialized) {
            _isTimezoneInitialized = true;
            tm     infolocal, infogm;
            time_t rawtime;
            time(&rawtime);
#if defined(_MSC_VER)
            (void)localtime_s(&infolocal, &rawtime);
            (void)gmtime_s(&infogm, &rawtime);
#else
            (void)localtime_r(&rawtime, &infolocal);
            (void)gmtime_r(&rawtime, &infogm);
#endif
            _localTimeBiasNs = 3600000000000LL * (infolocal.tm_hour - infogm.tm_hour);
            memset(_timezoneBuffer, 0, sizeof(_timezoneBuffer));
            std::strftime(_timezoneBuffer, 8, "%z", &infolocal);
            if (strlen(_timezoneBuffer) == 5 && (_timezoneBuffer[0] == '+' || _timezoneBuffer[0] == '-')) {
                // Add ':' between the hours and minutes to match the non-compact ISO standard
                memmove(_timezoneBuffer + 4, _timezoneBuffer + 3, 2);
                _timezoneBuffer[3] = ':';
            }
        }
        _withColor        = withColor;
        _withUtcTime      = withUtcTime;
        _recordStartUtcNs = recordStartUtcNs;

        // Decompose the formatter pattern for more efficient display
        _tokens.clear();
        const char* p    = formatterPattern;
        const char* pEnd = formatterPattern + strlen(formatterPattern);
        while (pEnd - p >= 2) {
            // Get the token type
            char ttype = ' ';  // space means None
            if (*p == '%') {   // Only the first loop may not enter
                ttype = *(++p);
                ++p;
            }
            // Get the post text
            const char* p2 = p;
            while (*p2 != 0 && *p2 != '%') ++p2;
            _tokens.push_back({ttype, std::string(p, p2)});
            p = p2;
        }
    }

    // Console display formatter
    // The pattern catalog is:
    //  %t 	Thread id
    //  %v 	The actual text to log
    //  %c 	Category
    //  %L 	The log level of the message
    //  %l 	Short log level of the message
    //  %a 	Abbreviated weekday name
    //  %A 	Full weekday name
    //  %b 	Abbreviated month name
    //  %B 	Full month name
    //  %y 	Year in 2 digits
    //  %Y 	Year in 4 digits
    //  %m 	Month
    //  %d 	Day of month
    //  %p 	AM/PM
    //  %z 	ISO 8601 offset from UTC in timezone
    //  %H 	Hours in 24 format
    //  %h 	Hours in 12 format
    //  %M 	Minutes
    //  %S 	Seconds
    //  %e 	Millisecond part of the current second
    //  %f 	Microsecond part of the current second
    //  %g 	Nanosecond part of the current second
    //  %E 	Millisecond since epoch
    //  %F 	Microsecond since epoch
    //  %G 	Nanosecond since epoch
    //  %I  Milliseconds since start of the record
    //  %J  Microseconds since start of the record
    //  %K  Nanoseconds  since start of the record
    //  %Q  end of line and multiline binary buffer dump
    //  %q  ' (+ buffer[N])' or nothing if empty
    void format(char* outBuf, uint32_t outBufSize, clockNs_t timestampUtcNs, uint32_t level, const char* threadName, const char* category,
                const char* logContent, const uint8_t* binaryBuffer, uint32_t binaryBufferSize, bool addEndOfLine)
    {
        // Constants
        constexpr const char* levelStrColor[SSLOG_LEVEL_QTY]      = {"\033[37mTRACE\033[m",
                                                                     "\033[37mDEBUG\033[m",
                                                                     "\033[32m INFO\033[m",
                                                                     "\033[33m\033[1m WARN\033[m",
                                                                     "\033[1m\033[41mERROR\033[m",
                                                                     "\033[1m\033[41mCRITICAL\033[m",
                                                                     ""};
        constexpr const char* levelStrMono[SSLOG_LEVEL_QTY]       = {"TRACE", "DEBUG", " INFO", " WARN", "ERROR", "CRITICAL", ""};
        constexpr const char* shortLevelStrColor[SSLOG_LEVEL_QTY] = {"\033[37mT\033[m",
                                                                     "\033[37mD\033[m",
                                                                     "\033[32mI\033[m",
                                                                     "\033[33m\033[1mW\033[m",
                                                                     "\033[1m\033[41mE\033[m",
                                                                     "\033[1m\033[41mC\033[m",
                                                                     ""};
        constexpr const char* shortLevelStrMono[SSLOG_LEVEL_QTY]  = {"T", "D", "I", "W", "E", "C", ""};
        constexpr const char* shortDayName[7]                     = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
        constexpr const char* dayName[7]         = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
        constexpr const char* shortMonthName[12] = {"Jan", "Feb", "Mar", "April", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        constexpr const char* monthName[12]      = {"January", "February", "March",     "April",   "May",      "June",
                                                    "July",    "August",   "September", "October", "November", "December"};

        // Get usable information about time & date of the log
        uint64_t recordStartUtcNs = _recordStartUtcNs;
        if (!_withUtcTime) {
            timestampUtcNs += _localTimeBiasNs;
            recordStartUtcNs += _localTimeBiasNs;
        }
        std::time_t epoch_time = static_cast<std::time_t>(timestampUtcNs / 1000000000);
#if defined(_MSC_VER)
        std::tm ptmTmp;
        (void)gmtime_s(&ptmTmp, &epoch_time);
        std::tm* ptm = &ptmTmp;
#else
        std::tm* ptm = std::gmtime(&epoch_time);  // Much faster than localtime, corrected with the added bias
#endif

        // Assemble the pattern tokens as specified
        char* p       = outBuf;
        char* pEnd    = p + outBufSize - 8;  // 8 = Margin to finish the line in all cases
        int   tmpSize = 0;
        for (const Token& token : _tokens) {
            switch (token.ttype) {
                case 't':
                    tmpSize = (int)sslogMin(strlen(threadName), (size_t)(pEnd - p));
                    if (tmpSize > 0) {
                        memcpy(p, threadName, tmpSize);
                        p += tmpSize;
                    }
                    break;
                case 'v':
                    tmpSize = (int)sslogMin(strlen(logContent), (size_t)(pEnd - p));
                    if (tmpSize > 0) {
                        memcpy(p, logContent, tmpSize);
                        p += tmpSize;
                    }
                    break;
                case 'c':
                    tmpSize = (int)sslogMin(strlen(category), (size_t)(pEnd - p));
                    if (tmpSize > 0) {
                        memcpy(p, category, tmpSize);
                        p += tmpSize;
                    }
                    break;
                case 'L': {
                    const char* levelStr = _withColor ? levelStrColor[(level >= SSLOG_LEVEL_QTY - 1) ? SSLOG_LEVEL_QTY - 1 : level] :
                                                        levelStrMono[(level >= SSLOG_LEVEL_QTY - 1) ? SSLOG_LEVEL_QTY - 1 : level];
                    tmpSize              = (int)sslogMin(strlen(levelStr), (size_t)(pEnd - p));
                    if (tmpSize > 0) {
                        memcpy(p, levelStr, tmpSize);
                        p += tmpSize;
                    }
                } break;
                case 'l': {
                    const char* levelStr = _withColor ? shortLevelStrColor[(level >= SSLOG_LEVEL_QTY - 1) ? SSLOG_LEVEL_QTY - 1 : level] :
                                                        shortLevelStrMono[(level >= SSLOG_LEVEL_QTY - 1) ? SSLOG_LEVEL_QTY - 1 : level];
                    tmpSize              = (int)sslogMin(strlen(levelStr), (size_t)(pEnd - p));
                    if (tmpSize > 0) {
                        memcpy(p, levelStr, tmpSize);
                        p += tmpSize;
                    }
                } break;
                case 'z':
                    tmpSize = (int)sslogMin(strlen(_timezoneBuffer), (size_t)(pEnd - p));
                    if (tmpSize > 0) {
                        memcpy(p, _timezoneBuffer, tmpSize);
                        p += tmpSize;
                    }
                    break;
                case 'a':
                    if (pEnd - p >= 2) { p += snprintf(p, pEnd - p, "%s", shortDayName[ptm->tm_wday]); }
                    break;
                case 'A':
                    if (pEnd - p >= 2) { p += snprintf(p, pEnd - p, "%s", dayName[ptm->tm_wday]); }
                    break;
                case 'b':
                    if (pEnd - p >= 2) { p += snprintf(p, pEnd - p, "%s", shortMonthName[ptm->tm_mon]); }
                    break;
                case 'B':
                    if (pEnd - p >= 2) { p += snprintf(p, pEnd - p, "%s", monthName[ptm->tm_mon]); }
                    break;
                case 'y':
                    if (pEnd - p >= 2) { p += snprintf(p, pEnd - p, "%02d", (1900 + ptm->tm_year) % 100); }
                    break;
                case 'Y':
                    if (pEnd - p >= 4) { p += snprintf(p, pEnd - p, "%04d", 1900 + ptm->tm_year); }
                    break;
                case 'm':
                    if (pEnd - p >= 2) { p += snprintf(p, pEnd - p, "%02d", ptm->tm_mon + 1); }
                    break;
                case 'd':
                    if (pEnd - p >= 2) { p += snprintf(p, pEnd - p, "%02d", ptm->tm_mday); }
                    break;
                case 'H':
                    if (pEnd - p >= 2) { p += snprintf(p, pEnd - p, "%02d", ptm->tm_hour); }
                    break;
                case 'h':
                    if (pEnd - p >= 2) { p += snprintf(p, pEnd - p, "%02d", ptm->tm_hour % 12); }
                    break;
                case 'p':
                    if (pEnd - p >= 2) { p += snprintf(p, pEnd - p, "%s", (ptm->tm_hour >= 12) ? "PM" : "AM"); }
                    break;
                case 'M':
                    if (pEnd - p >= 2) { p += snprintf(p, pEnd - p, "%02d", ptm->tm_min); }
                    break;
                case 'S':
                    if (pEnd - p >= 2) { p += snprintf(p, pEnd - p, "%02d", ptm->tm_sec); }
                    break;
                case 'e':
                    if (pEnd - p >= 3) { p += snprintf(p, pEnd - p, "%03" PRIu64, (timestampUtcNs / 1000000UL) % 1000UL); }
                    break;
                case 'f':
                    if (pEnd - p >= 6) { p += snprintf(p, pEnd - p, "%06" PRIu64, (timestampUtcNs / 1000UL) % 1000000UL); }
                    break;
                case 'g':
                    if (pEnd - p >= 9) { p += snprintf(p, pEnd - p, "%09" PRIu64, timestampUtcNs % 1000000000UL); }
                    break;
                case 'E':
                    if (pEnd - p >= 14) { p += snprintf(p, pEnd - p, "%" PRIu64, timestampUtcNs / 1000000UL); }
                    break;
                case 'F':
                    if (pEnd - p >= 17) { p += snprintf(p, pEnd - p, "%" PRIu64, timestampUtcNs / 1000UL); }
                    break;
                case 'G':
                    if (pEnd - p >= 20) { p += snprintf(p, pEnd - p, "%" PRIu64, timestampUtcNs); }
                    break;
                case 'I':
                    if (pEnd - p >= 6) { p += snprintf(p, pEnd - p, "%" PRIu64, (timestampUtcNs - recordStartUtcNs) / 1000000UL); }
                    break;
                case 'J':
                    if (pEnd - p >= 9) { p += snprintf(p, pEnd - p, "%" PRIu64, (timestampUtcNs - recordStartUtcNs) / 1000UL); }
                    break;
                case 'K':
                    if (pEnd - p >= 12) { p += snprintf(p, pEnd - p, "%" PRIu64, timestampUtcNs - recordStartUtcNs); }
                    break;
                case 'q':
                    if (pEnd - p >= 29 && binaryBufferSize > 0) { p += snprintf(p, pEnd - p, " (+ buffer[%u])", binaryBufferSize); }
                    break;
                case 'Q':
                    if (pEnd - p >= 130 && binaryBufferSize > 0) {
                        *p++ = '\n';
                        for (uint32_t i = 0; i < binaryBufferSize && pEnd - p > 128; ++i) {  // 128 is enough for one full line
                            if ((i % 32) == 0) { p += snprintf(p, pEnd - p, "        %04x   ", i); }
                            p += snprintf(p, pEnd - p, "%02X ", binaryBuffer[i]);
                            if (((i + 1) % 32) == 0) {
                                p += snprintf(p, pEnd - p, "\n");
                            } else if (((i + 1) % 16) == 0) {
                                p += snprintf(p, pEnd - p, " ");
                            }
                        }
                        *p++ = '\n';
                        *p++ = 0;
                    }
                    break;

                default:
                    break;
            };

            // Add the post text
            if (!token.postText.empty()) {
                tmpSize = (int)sslogMin(token.postText.size(), (size_t)(pEnd - p));
                if (tmpSize > 0) {
                    memcpy(p, token.postText.data(), tmpSize);
                    p += tmpSize;
                }
            }
        }  // end of loop on tokens

        // Finalization
        if (p >= pEnd - 1) {
            p = pEnd - 1;
            // Mark the truncation
            *p++ = ' ';
            *p++ = '.';
            *p++ = '.';
            *p++ = '.';
        }
        if (addEndOfLine) *(p++) = '\n';
        *(p++) = 0;
    }

   private:
    bool      _withColor;
    bool      _withUtcTime;
    bool      _isTimezoneInitialized = false;
    char      _timezoneBuffer[8];     // Formatted time zone, assumed constant
    int64_t   _localTimeBiasNs  = 0;  // Call to localtime is very expensive due to the call to getenv("TZ")
    clockNs_t _recordStartUtcNs = 0;

    struct Token {
        char        ttype;
        std::string postText;
    };
    std::vector<Token> _tokens;
};

// Thread related information
struct ThreadContext {
    uint32_t nameIdx     = 0xFFFFFFFF;
    uint32_t reinitCount = 0;
    char     name[64]    = {0};  // Thread name persistency
};

inline thread_local ThreadContext threadLocalContext;

struct StringBank {
    uint32_t             offset = 0;
    std::vector<uint8_t> buffer;
};
struct DataBank {
    uint32_t             offset[2] = {0, 0};
    std::vector<uint8_t> buffer[2];
    clockNs_t   utcSystemClockOriginNs = 0ULL;  // System and high res clock synchronization. Snapshot just before events in the buffer
    clockTick_t steadyClockOriginTick  = 0ULL;
};

struct DataFile {
    uint32_t    fileNumber;
    clockTick_t endTick;
};

struct DeltaEncoding {
    clockTick_t lastLogTimestampTick = 0;
    uint32_t    lastLogFormatIdx     = 0;
    uint32_t    lastLogCategoryIdx   = 0;
    uint32_t    lastLogThreadId      = 0;
    bool        doLogFullEncoding    = true;
};

// Forward declarations
void
stop();

// Global service context, accessible from everywhere
inline struct GlobalContext {
    // Collection banks
    std::mutex       loggingLock;
    bool             enabled             = false;
    int              stringActiveBankNbr = 0;
    int              dataActiveBankNbr   = 0;
    StringBank       stringBanks[2];
    DataBank         dataBanks[2];
    std::atomic<int> isDataBufferSaturated{0};
    std::atomic<int> isStringBufferSaturated{0};
    clockTick_t      lastFlushedBufferTick = 0;
    clockNs_t        utcSystemClockStartNs = 0ULL;  // System clock of the start of the session
    uint32_t         reinitCount           = 0;

    // Delta encoding, context per buffer (standard & details)
    DeltaEncoding deltaEncoding[2];

    // Configs
    Collector                collector;
    Collector                newCollectorCfg;
    std::atomic<int>         newCollectorCfgPresent{0};
    Sink                     sink;
    Sink                     newSinkCfg;
    std::atomic<int>         newSinkCfgPresent{0};
    std::atomic<int>         forceFlush{0};
    Level                    minLoggingLevel = Level::trace;
    SSLOG_FS_NAMESPACE::path pathname;

    // Data collection
    uint8_t                 sessionId[8] = {0};
    FlatHashTable<uint32_t> lkupStringToIndex;
    FILE*                   fileBaseHandle      = 0;
    FILE*                   fileDataHandle[2]   = {0, 0};
    uint32_t                stringUniqueId      = 0;
    clockTick_t             dataFileSwitchTick  = 0;
    uint64_t                dataFileCurrentSize = 0;
    uint32_t                dataFileNumber      = 0;
    std::mutex              consoleMx;
    Stats                   stats = {};

    // Detailed file management
    std::vector<DataFile> existingDataFiles;
    std::atomic<int>      detailsRequested = {0};
    std::vector<DataFile> detailedFilesToDelete;
    clockTick_t           currentDetailedFileStartTick = 0;
    clockTick_t           detailedFileEndTick          = 0;

    // Misc
    TextFormatter textFormatter;
    TimeConverter timeConverter;

    // Threads
    std::thread*            threadFlusher         = 0;
    std::atomic<int>        threadFlusherFlagStop = {0};
    bool                    threadShallWakeUp     = false;
    int                     flushThreadId         = -1;  // To avoid locks when the flushing thread crashes
    bool                    threadIsStarted       = false;
    std::mutex              threadInitMx;
    std::condition_variable threadInitCv;
    std::mutex              threadSyncMx;
    std::condition_variable threadSyncCv;
    std::mutex              threadConfigMx;
    std::atomic<int>        threadIsConfigApplied{0};

    // Signals and HW exceptions
    bool              doNotUninit           = false;  // Emergency exit shall not clean ressources
    bool              signalHandlersSaved   = false;
    ssSignalHandler_t signalsOldHandlers[7] = {0};
#if defined(_MSC_VER)
    PVOID exceptionHandler = 0;
#if SSLOG_STACKTRACE == 1
    // Stacktrace
    rtlWalkFrameChain_t rtlWalkFrameChain = 0;
#endif
#endif

    void initCollector()
    {
        // Store new config
        collector = newCollectorCfg;
        newCollectorCfgPresent.store(0);

        // Apply it
        stringActiveBankNbr = 0;
        dataActiveBankNbr   = 0;
        for (int bankNbr = 0; bankNbr < 2; ++bankNbr) {
            DataBank& dataBank = dataBanks[bankNbr];
            for (int bufNbr = 0; bufNbr < 2; ++bufNbr) {
                dataBank.offset[bufNbr] = 0;
                dataBank.buffer[bufNbr].resize(collector.dataBufferBytes);
            }
            dataBank.steadyClockOriginTick  = getHiResClockTick();
            dataBank.utcSystemClockOriginNs = getUtcSystemClockNs();
            StringBank& stringBank          = stringBanks[bankNbr];
            stringBank.offset               = 0;
            stringBank.buffer.resize(collector.stringBufferBytes);
        }
        isDataBufferSaturated.store(0);
        isStringBufferSaturated.store(0);
    }

    void initStorageState()
    {
        srand((unsigned)time(0));
        for (size_t i = 0; i < 8; ++i) { sessionId[i] = (uint8_t)(std::rand() >> 16); }
        lkupStringToIndex.clear();
        stringUniqueId = 0;
        for (int bufNbr = 0; bufNbr < 2; ++bufNbr) {
            DeltaEncoding& de       = deltaEncoding[bufNbr];
            de.lastLogTimestampTick = 0;  // 0 is the expected state at the start of a logging session
            de.lastLogFormatIdx     = 0;
            de.lastLogCategoryIdx   = 0;
            de.lastLogThreadId      = 0;
            de.doLogFullEncoding    = true;
        }
        if (fileBaseHandle) { fclose(fileBaseHandle); }
        fileBaseHandle = 0;
        for (int bufNbr = 0; bufNbr < 2; ++bufNbr) {
            if (fileDataHandle[bufNbr]) { fclose(fileDataHandle[bufNbr]); }
            fileDataHandle[bufNbr] = 0;
        }
        dataFileCurrentSize   = 0;
        dataFileNumber        = 0;
        utcSystemClockStartNs = getUtcSystemClockNs();
        existingDataFiles.clear();
        detailsRequested.store(0);
        detailedFilesToDelete.clear();
        currentDetailedFileStartTick = 0;
        detailedFileEndTick          = 0;
        pathname                     = "";
        ++reinitCount;
        initCollector();  // Need to empty the banks too
    }

    void initSink()
    {
        // Store new config
        SSLOG_FS_NAMESPACE::path oldPathPattern = sink.path;
        sink                                    = newSinkCfg;
        newSinkCfgPresent.store(0);

        // Some precomputations
        minLoggingLevel    = sslogMin(sink.storageLevel, sink.detailsLevel);
        minLoggingLevel    = sslogMin(minLoggingLevel, sink.consoleLevel);
        minLoggingLevel    = sslogMin(minLoggingLevel, sink.liveNotifLevel);
        dataFileSwitchTick = getHiResClockTick() + timeConverter.durationNsToTick(1e9 * sink.splitFileMaxDurationSec);

        // Precompute information in the formatter (pattern decomposition, timezone...)
        textFormatter.init(sink.consoleFormatter.c_str(), sink.useUTCTime, sink.consoleMode == ConsoleMode::Color, utcSystemClockStartNs);

        // We rely on the fact that the initial pathname is empty
        // Reset the files and storage logging state
        if (oldPathPattern != sink.path || fileBaseHandle == nullptr) { initStorageState(); }

        // Initialize the storage, if active
        if (!sink.path.empty() && (sink.storageLevel < Level::off || sink.detailsLevel < Level::off) && fileBaseHandle == nullptr) {
            // Get the pathname from the path pattern
            char          tmpPath[256];
            TextFormatter tf;
            tf.init(sink.path.c_str(), sink.useUTCTime, false, utcSystemClockStartNs);
            tf.format(tmpPath, sizeof(tmpPath), timeConverter.getUtcNs(getHiResClockTick()), SSLOG_LEVEL_QTY - 1, "", "", "", nullptr, 0,
                      false);
            if (strlen(tmpPath) == 0) { snprintf(tmpPath, sizeof(tmpPath), "default_due_to_empty_pathname"); }
            pathname = std::string(tmpPath);

            // Ensure that the folder exists
            std::error_code ec;
            if (!SSLOG_FS_NAMESPACE::exists(pathname) && !SSLOG_FS_NAMESPACE::create_directory(pathname, ec)) {
                fprintf(stderr, "SSLOG error: unable to create the directory '%s': %s\n", pathname.string().c_str(), ec.message().c_str());
                enabled = false;  // The logging service is fully disabled
                return;
            }

            // Clean it from old files
            SSLOG_FS_NAMESPACE::directory_iterator di(pathname, ec);
            if (ec) {
                fprintf(stderr, "SSLOG error: unable to access the directory '%s': %s\n", pathname.string().c_str(), ec.message().c_str());
                enabled = false;
                return;
            }
            for (auto const& de : di) {
                if (SSLOG_FS_NAMESPACE::is_regular_file(de) && (strncmp("data", de.path().filename().string().c_str(), 4) == 0 ||
                                                                strncmp("base", de.path().filename().string().c_str(), 4) == 0)) {
                    SSLOG_FS_NAMESPACE::remove(de.path().string(), ec);
                }
            }
        }
    }

    void initTime()
    {
        // Measure the high performance clock frequency with the standard nanosecond clock
#if SSLOG_VIRTUAL_TIME_FOR_TEST_ONLY == 0
        double tickToNs = 1e6;
        for (int i = 0; i < 2; ++i) {  // Twice just in case an interrupt occurs during the first loop
            clockTick_t highPerfT0 = getHiResClockTick();
            const auto  stdT0      = std::chrono::high_resolution_clock::now();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            clockTick_t highPerfT1 = getHiResClockTick();
            const auto  stdT1      = std::chrono::high_resolution_clock::now();
            tickToNs = sslogMin(tickToNs, (double)std::chrono::duration_cast<std::chrono::nanoseconds>(stdT1 - stdT0).count() /
                                              (double)(highPerfT1 - highPerfT0));
        }
#else
        double tickToNs = 1.;  // Internal test only with virtual time
#endif
        timeConverter.init(tickToNs, getUtcSystemClockNs(), getHiResClockTick());
    }

    void init()
    {
        enabled = false;
        initTime();
        initStorageState();
        initSink();

        memset(&stats, 0, sizeof(Stats));
        doNotUninit = false;

        threadFlusherFlagStop.store(0);
        flushThreadId         = -1;
        threadIsStarted       = false;
        lastFlushedBufferTick = getHiResClockTick();
        signalHandlersSaved   = false;
        memset(signalsOldHandlers, 0, sizeof(signalsOldHandlers));
    }

} gc;

inline uint32_t
addString(hashStr_t strHash, const char* str)
{
    // If too big for string buffers, the string is truncated. The hash is kept as the original one.
    uint32_t stringBufferSize = (uint32_t)gc.stringBanks[gc.stringActiveBankNbr].buffer.size();  // Same whatever the bank
    size_t   strLength        = sslogMin((uint32_t)strlen(str), stringBufferSize - 1) + 1 /* zero terminated */;
    bool     hasWaited        = false;

    // Buffer saturation?
    while (gc.stringBanks[gc.stringActiveBankNbr].offset + strLength > stringBufferSize) {
        // Wait until enough space
        gc.loggingLock.unlock();
        {
            std::lock_guard<std::mutex> lkSync(gc.threadSyncMx);
            gc.isStringBufferSaturated.store(1);
            gc.threadShallWakeUp = true;
            gc.threadSyncCv.notify_one();
        }
        std::this_thread::yield();
        gc.loggingLock.lock();
        hasWaited = true;
    }
    if (hasWaited) { ++gc.stats.delayedStrings; }

    // Structure of the string buffer is an ordered list of C strings (zero terminated).
    // The order provides the integer index for each string
    StringBank& bank = gc.stringBanks[gc.stringActiveBankNbr];
    if (str) { memcpy(&bank.buffer[bank.offset], str, strLength); }
    bank.buffer[bank.offset + strLength - 1] = 0;  // Required in case of truncation
    bank.offset += (uint32_t)strLength;

    // Update the hash with this new string and return its index
    ++gc.stats.storedStrings;
    gc.lkupStringToIndex.insert(strHash, gc.stringUniqueId);
    return gc.stringUniqueId++;
}

// Dynamic string hashing
inline hashStr_t
hashString(const char* s, int maxCharQty = -1)
{
    hashStr_t strHash = SSLOG_FNV_HASH_OFFSET_;
    while (*s && maxCharQty--) strHash = (strHash ^ ((hashStr_t)(*s++))) * SSLOG_FNV_HASH_PRIME_;
    return (strHash == 0) ? 1 : strHash;  // Zero is a reserved value
}

inline uint32_t
getThreadIdx()
{
    ThreadContext* tCtx = &threadLocalContext;
    if (tCtx->nameIdx == 0xFFFFFFFF || tCtx->reinitCount != gc.reinitCount) {
        tCtx->reinitCount = gc.reinitCount;
        if (tCtx->nameIdx == 0xFFFFFFFF) { snprintf(tCtx->name, sizeof(tCtx->name), "%u", SSLOG_GET_SYS_THREAD_ID()); }
        hashStr_t strHash = hashString(tCtx->name);
        if (!gc.lkupStringToIndex.find(strHash, tCtx->nameIdx)) { tCtx->nameIdx = addString(strHash, tCtx->name); }
    }
    return tCtx->nameIdx;
}

inline const char*
getThreadName()
{
    ThreadContext* tCtx = &threadLocalContext;
    if (tCtx->nameIdx == 0xFFFFFFFF || tCtx->reinitCount != gc.reinitCount) {
        tCtx->reinitCount = gc.reinitCount;
        if (tCtx->nameIdx == 0xFFFFFFFF) { snprintf(tCtx->name, sizeof(tCtx->name), "%u", SSLOG_GET_SYS_THREAD_ID()); }
        hashStr_t strHash = hashString(tCtx->name);
        if (!gc.lkupStringToIndex.find(strHash, tCtx->nameIdx)) { tCtx->nameIdx = addString(strHash, tCtx->name); }
    }
    return threadLocalContext.name;
}

inline void
setThreadName(const char* name)
{
    std::lock_guard<std::mutex> lk(gc.loggingLock);
    ThreadContext*              tCtx = &threadLocalContext;
    snprintf(tCtx->name, sizeof(tCtx->name), "%s", name);
    tCtx->reinitCount = gc.reinitCount;
    hashStr_t strHash = hashString(tCtx->name);
    if (!gc.lkupStringToIndex.find(strHash, tCtx->nameIdx)) { tCtx->nameIdx = addString(strHash, tCtx->name); }
}

// Variadic set of functions to compute the storage size at compile time
#define SSLOG_GET_LOG_SIZE_IMPL(argType_t, storedArgType_t)          \
    template<typename... Args>                                       \
    inline int getLogStorageSize(argType_t /*value*/, Args... args)  \
    {                                                                \
        return sizeof(storedArgType_t) + getLogStorageSize(args...); \
    }                                                                \
    template<typename... Args>                                       \
    inline int getArgQty(argType_t /*value*/, Args... args)          \
    {                                                                \
        return 1 + getArgQty(args...);                               \
    }
#define SSLOG_GET_LOG_SIZE_DECL(argType_t)                \
    template<typename... Args>                            \
    int getLogStorageSize(argType_t value, Args... args); \
    template<typename... Args>                            \
    int getArgQty(argType_t value, Args... args);

template<typename... Args>
inline int
getLogStorageSize([[maybe_unused]] Args... args)
{
    return 0;
}
template<typename... Args>
inline int
getArgQty()
{
    return 0;
}
SSLOG_GET_LOG_SIZE_DECL(bool)
SSLOG_GET_LOG_SIZE_DECL(int8_t)
SSLOG_GET_LOG_SIZE_DECL(uint8_t)
SSLOG_GET_LOG_SIZE_DECL(int16_t)
SSLOG_GET_LOG_SIZE_DECL(uint16_t)
SSLOG_GET_LOG_SIZE_DECL(int32_t)
SSLOG_GET_LOG_SIZE_DECL(uint32_t)
SSLOG_GET_LOG_SIZE_DECL(float)
SSLOG_GET_LOG_SIZE_DECL(void*)
SSLOG_GET_LOG_SIZE_DECL(int64_t)
SSLOG_GET_LOG_SIZE_DECL(uint64_t)
SSLOG_GET_LOG_SIZE_DECL(double)
SSLOG_GET_LOG_SIZE_DECL(char*)
SSLOG_GET_LOG_SIZE_DECL(const char*)
SSLOG_GET_LOG_SIZE_DECL(const std::string&)
SSLOG_GET_LOG_SIZE_DECL(const std::string_view&)
SSLOG_GET_LOG_SIZE_IMPL(bool, int8_t)
SSLOG_GET_LOG_SIZE_IMPL(int8_t, int8_t)
SSLOG_GET_LOG_SIZE_IMPL(uint8_t, uint8_t)
SSLOG_GET_LOG_SIZE_IMPL(int16_t, int16_t)
SSLOG_GET_LOG_SIZE_IMPL(uint16_t, uint16_t)
SSLOG_GET_LOG_SIZE_IMPL(int32_t, int32_t)
SSLOG_GET_LOG_SIZE_IMPL(uint32_t, uint32_t)
SSLOG_GET_LOG_SIZE_IMPL(float, float)
SSLOG_GET_LOG_SIZE_IMPL(void*, uint64_t)
SSLOG_GET_LOG_SIZE_IMPL(int64_t, int64_t)
SSLOG_GET_LOG_SIZE_IMPL(uint64_t, uint64_t)
SSLOG_GET_LOG_SIZE_IMPL(double, double)
SSLOG_GET_LOG_SIZE_IMPL(char*, uint32_t)                   /* Stored as a string ID */
SSLOG_GET_LOG_SIZE_IMPL(const char*, uint32_t)             /* Stored as a string ID */
SSLOG_GET_LOG_SIZE_IMPL(const std::string&, uint32_t)      /* Stored as a string ID */
SSLOG_GET_LOG_SIZE_IMPL(const std::string_view&, uint32_t) /* Stored as a string ID */

#define SSLOG_LOG_ARG_IMPL(argType_t, storedArgType_t, flagType)                                           \
    template<typename... Args>                                                                             \
    inline void logArgument(int argIdx, uint8_t* bufFlag, uint8_t* bufData, argType_t value, Args... args) \
    {                                                                                                      \
        if (argIdx & 1) {                                                                                  \
            *bufFlag |= flagType;                                                                          \
        } /* Write a quartet per argument*/                                                                \
        else {                                                                                             \
            *bufFlag = (flagType << 4);                                                                    \
        }                                                                                                  \
        uint8_t* src = (uint8_t*)&value;                                                                   \
        for (int i = sizeof(storedArgType_t) - 1; i >= 0; --i) { (*bufData++) = (*src++); }                \
        logArgument(argIdx + 1, bufFlag + ((argIdx & 1) ? 1 : 0), bufData, args...);                       \
    }
#define SSLOG_LOG_ARG_DECL(argType_t) \
    template<typename... Args>        \
    inline void logArgument(int argIdx, uint8_t* bufFlag, uint8_t* bufData, argType_t value, Args... args);
inline void
logArgument(int /*argIdx*/, uint8_t* /*bufFlag*/, uint8_t* /*bufData*/)
{
}
SSLOG_LOG_ARG_DECL(bool)
SSLOG_LOG_ARG_DECL(int8_t)
SSLOG_LOG_ARG_DECL(uint8_t)
SSLOG_LOG_ARG_DECL(int16_t)
SSLOG_LOG_ARG_DECL(uint16_t)
SSLOG_LOG_ARG_DECL(int32_t)
SSLOG_LOG_ARG_DECL(uint32_t)
SSLOG_LOG_ARG_DECL(float)
SSLOG_LOG_ARG_DECL(void*)
SSLOG_LOG_ARG_DECL(int64_t)
SSLOG_LOG_ARG_DECL(uint64_t)
SSLOG_LOG_ARG_DECL(double)
SSLOG_LOG_ARG_DECL(const char*)
SSLOG_LOG_ARG_DECL(const std::string&)
SSLOG_LOG_ARG_DECL(const std::string_view&)
SSLOG_LOG_ARG_IMPL(bool, int8_t, SSLOG_DATA_S8)
SSLOG_LOG_ARG_IMPL(int8_t, int8_t, SSLOG_DATA_S8)
SSLOG_LOG_ARG_IMPL(uint8_t, uint8_t, SSLOG_DATA_U8)
SSLOG_LOG_ARG_IMPL(int16_t, int16_t, SSLOG_DATA_S16)
SSLOG_LOG_ARG_IMPL(uint16_t, uint16_t, SSLOG_DATA_U16)
SSLOG_LOG_ARG_IMPL(int32_t, int32_t, SSLOG_DATA_S32)
SSLOG_LOG_ARG_IMPL(uint32_t, uint32_t, SSLOG_DATA_U32)
SSLOG_LOG_ARG_IMPL(float, float, SSLOG_DATA_FLOAT)
SSLOG_LOG_ARG_IMPL(void*, uint64_t, SSLOG_DATA_U64)
SSLOG_LOG_ARG_IMPL(int64_t, int64_t, SSLOG_DATA_S64)
SSLOG_LOG_ARG_IMPL(uint64_t, uint64_t, SSLOG_DATA_U64)
SSLOG_LOG_ARG_IMPL(double, double, SSLOG_DATA_DOUBLE)

template<typename... Args>
inline void
logArgument(int argIdx, uint8_t* bufFlag, uint8_t* bufData, const char* value, Args... args)
{
    if (argIdx & 1) {
        *bufFlag |= SSLOG_DATA_STRING;
    } else {
        *bufFlag = (SSLOG_DATA_STRING << 4);
    }
    hashStr_t strHash   = hashString(value);
    uint32_t  stringIdx = 0;
    if (!gc.lkupStringToIndex.find(strHash, stringIdx)) { stringIdx = addString(strHash, value); }

    uint8_t* src = (uint8_t*)&stringIdx;
    for (int i = sizeof(uint32_t) - 1; i >= 0; --i) { (*bufData++) = (*src++); }
    logArgument(argIdx + 1, bufFlag + ((argIdx & 1) ? 1 : 0), bufData, args...);
}

template<typename... Args>
inline void
logArgument(int argIdx, uint8_t* bufFlag, uint8_t* bufData, const std::string& value, Args... args)
{
    if (argIdx & 1) {
        *bufFlag |= SSLOG_DATA_STRING;
    } else {
        *bufFlag = (SSLOG_DATA_STRING << 4);
    }
    hashStr_t strHash   = hashString(value.c_str());
    uint32_t  stringIdx = 0;
    if (!gc.lkupStringToIndex.find(strHash, stringIdx)) { stringIdx = addString(strHash, value.c_str()); }

    uint8_t* src = (uint8_t*)&stringIdx;
    for (int i = sizeof(uint32_t) - 1; i >= 0; --i) { (*bufData++) = (*src++); }
    logArgument(argIdx + 1, bufFlag + ((argIdx & 1) ? 1 : 0), bufData, args...);
}

template<typename... Args>
inline void
logArgument(int argIdx, uint8_t* bufFlag, uint8_t* bufData, const std::string_view& value, Args... args)
{
    if (argIdx & 1) {
        *bufFlag |= SSLOG_DATA_STRING;
    } else {
        *bufFlag = (SSLOG_DATA_STRING << 4);
    }
    hashStr_t strHash   = hashString(std::string(value).c_str());
    uint32_t  stringIdx = 0;
    if (!gc.lkupStringToIndex.find(strHash, stringIdx)) { stringIdx = addString(strHash, std::string(value).c_str()); }

    uint8_t* src = (uint8_t*)&stringIdx;
    for (int i = sizeof(uint32_t) - 1; i >= 0; --i) { (*bufData++) = (*src++); }
    logArgument(argIdx + 1, bufFlag + ((argIdx & 1) ? 1 : 0), bufData, args...);
}

inline uint8_t*
logHeader(bool withLock, int argQty, uint32_t logAllArgsSize, Level level, hashStr_t formatHash, hashStr_t categoryHash, const char* format,
          const char* category)
{
    // Get the string indexes
    assert(categoryHash != 0 && "The string for category must be static");
    uint32_t categoryIdx = 0;
    if (!gc.lkupStringToIndex.find(categoryHash, categoryIdx)) { categoryIdx = addString(categoryHash, category); }
    assert(formatHash != 0 && "The string for category must be static");
    uint32_t formatIdx = 0;
    if (!gc.lkupStringToIndex.find(formatHash, formatIdx)) { formatIdx = addString(formatHash, format); }

    uint16_t    flagType         = 0;
    uint32_t    headerSize       = 0;
    uint8_t*    buf              = nullptr;
    int         bufNbr           = (level < gc.sink.storageLevel) ? 1 : 0;  // 0 = storage, 1 = details
    int         formatIdxBytes   = 0;
    int         categoryIdxBytes = 0;
    int         threadIdxBytes   = 0;
    int         timestampBytes   = 0;
    uint32_t    threadIdx        = getThreadIdx();
    clockTick_t nowTick          = 0;
    bool        hasWaited        = false;

    // Loop until buffer resources are enough
    while (true) {
        flagType          = SSLOG_LEVEL_BASE + static_cast<uint16_t>(level);
        headerSize        = 2;  // The uint16_t flag
        DeltaEncoding& de = gc.deltaEncoding[bufNbr];

        // Find out the required bytes quantity to store the formatIdx, compared to the previous one
        uint32_t changedBits = de.doLogFullEncoding ? 0xFFFFFFFF : (formatIdx ^ de.lastLogFormatIdx);
        formatIdxBytes       = 0;
        if (changedBits == 0) {
            flagType |= SSLOG_FORMAT_0_BYTES;
        } else if ((changedBits & 0xFFFFFF00) == 0) {
            flagType |= SSLOG_FORMAT_1_BYTES;
            formatIdxBytes = 1;
        } else if ((changedBits & 0xFFFF0000) == 0) {
            flagType |= SSLOG_FORMAT_2_BYTES;
            formatIdxBytes = 2;
        } else {
            flagType |= SSLOG_FORMAT_3_BYTES;
            formatIdxBytes = 3;
        }
        headerSize += formatIdxBytes;

        // Find out the required bytes quantity to store the categoryIdx, compared to the previous one
        changedBits      = de.doLogFullEncoding ? 0xFFFFFFFF : (categoryIdx ^ de.lastLogCategoryIdx);
        categoryIdxBytes = 0;
        if (changedBits == 0) {
            flagType |= SSLOG_CATEGORY_0_BYTES;
        } else if ((changedBits & 0xFFFFFF00) == 0) {
            flagType |= SSLOG_CATEGORY_1_BYTES;
            categoryIdxBytes = 1;
        } else if ((changedBits & 0xFFFF0000) == 0) {
            flagType |= SSLOG_CATEGORY_2_BYTES;
            categoryIdxBytes = 2;
        } else {
            flagType |= SSLOG_CATEGORY_3_BYTES;
            categoryIdxBytes = 3;
        }
        headerSize += categoryIdxBytes;

        // Find out the required bytes quantity to store the threadIdx, compared to the previous one
        changedBits    = de.doLogFullEncoding ? 0xFFFFFFFF : (threadIdx ^ de.lastLogThreadId);
        threadIdxBytes = 0;
        if (changedBits == 0) {
            flagType |= SSLOG_THREADIDX_0_BYTES;
        } else if ((changedBits & 0xFFFFFF00) == 0) {
            flagType |= SSLOG_THREADIDX_1_BYTES;
            threadIdxBytes = 1;
        } else if ((changedBits & 0xFFFF0000) == 0) {
            flagType |= SSLOG_THREADIDX_2_BYTES;
            threadIdxBytes = 2;
        } else {
            flagType |= SSLOG_THREADIDX_3_BYTES;
            threadIdxBytes = 3;
        }
        headerSize += threadIdxBytes;

        // Find out the required bytes quantity to store the (monotonic) date, compared to the previous one
        nowTick = getHiResClockTick();
        clockTick_t changedTickBits =
            de.doLogFullEncoding ? 0xFFFFFFFFFFFFFFFFULL : (nowTick ^ de.lastLogTimestampTick);  // XOR marks the bits which differ
        timestampBytes = 2;
        if ((changedTickBits & 0xFFFFFFFFFFFF0000ULL) == 0) {
            flagType |= SSLOG_TS_2_BYTES;
        } else if ((changedTickBits & 0xFFFFFFFFFF000000ULL) == 0) {
            flagType |= SSLOG_TS_3_BYTES;
            timestampBytes = 3;
        } else if ((changedTickBits & 0xFFFFFFFF00000000ULL) == 0) {
            flagType |= SSLOG_TS_4_BYTES;
            timestampBytes = 4;
        } else {
            flagType |= SSLOG_TS_8_BYTES;
            timestampBytes = 8;
        }
        headerSize += timestampBytes;  // 3 bytes for formatIdx which is always present

        // Allocate the required size in the buffer
        DataBank& bank = gc.dataBanks[gc.dataActiveBankNbr];
        if (!withLock || bank.offset[bufNbr] + headerSize + logAllArgsSize <= bank.buffer[bufNbr].size()) {
            // Free space available, no need to loop
            ++gc.stats.storedLogs;
            buf = bank.buffer[bufNbr].data() + bank.offset[bufNbr];
            bank.offset[bufNbr] += headerSize + logAllArgsSize;
            break;
        }

        // Handle buffer saturation
        gc.isDataBufferSaturated.store(1);

        // Drop (or oversized for buffer)
        if (gc.collector.dataSaturationPolicy == SaturationPolicy::Drop || headerSize + logAllArgsSize > bank.buffer[bufNbr].size()) {
            ++gc.stats.droppedLogs;
            return nullptr;
        }

        // Wait for more space in buffer
        gc.loggingLock.unlock();
        {
            std::lock_guard<std::mutex> lkSync(gc.threadSyncMx);
            gc.threadShallWakeUp = true;
            gc.threadSyncCv.notify_one();
        }
        std::this_thread::yield();
        gc.loggingLock.lock();
        hasWaited = true;
    }

    // Commit the incremental coding
    DeltaEncoding& de       = gc.deltaEncoding[bufNbr];
    de.lastLogFormatIdx     = formatIdx;
    de.lastLogCategoryIdx   = categoryIdx;
    de.lastLogThreadId      = threadIdx;
    de.lastLogTimestampTick = nowTick;
    de.doLogFullEncoding    = false;
    if (hasWaited) { ++gc.stats.delayedLogs; }

    // Write the header
    (*buf++) = static_cast<uint8_t>(flagType & 0xFF);
    (*buf++) = static_cast<uint8_t>((flagType >> 8) & 0xFF);
    for (int i = 0; i < formatIdxBytes; ++i) {
        (*buf++) = formatIdx & 0xFF;
        formatIdx >>= 8;
    }
    for (int i = 0; i < categoryIdxBytes; ++i) {
        (*buf++) = categoryIdx & 0xFF;
        categoryIdx >>= 8;
    }
    for (int i = 0; i < threadIdxBytes; ++i) {
        (*buf++) = threadIdx & 0xFF;
        threadIdx >>= 8;
    }
    for (int i = 0; i < timestampBytes; ++i) {
        (*buf++) = nowTick & 0xFF;
        nowTick >>= 8;
    }

    // Clear the argument flags, including the last one as a terminaison
    for (int i = 0; i <= (argQty / 2); ++i) { buf[i] = 0; }

    return buf;  // Points after the header, at the start of the argument flags
}

// Helper required as va_list cannot handle "Args... args", it expects "..." instead
inline void
formatArgs(char* buffer, size_t bufferSize, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    (void)vsnprintf(buffer, bufferSize, format, ap);
    va_end(ap);
}

template<typename... Args>
inline void
log(bool withLock, hashStr_t formatHash, hashStr_t categoryHash, const char* format, const char* category, Level level, Args... args)
{
    if (withLock) gc.loggingLock.lock();
    if (!gc.enabled) {
        if (withLock) gc.loggingLock.unlock();
        return;
    }
    if (level >= gc.sink.storageLevel || level >= gc.sink.detailsLevel) {
        // Compute the log storage size
        int      argQty         = getArgQty(args...);
        int      argFlagsBytes  = 1 + (argQty / 2);
        uint32_t logAllArgsSize = argFlagsBytes + getLogStorageSize(args...);

        // Write the header then the values
        uint8_t* buf = logHeader(withLock, argQty, logAllArgsSize, level, formatHash, categoryHash, format, category);
        if (buf != nullptr) { logArgument(0, buf, buf + argFlagsBytes, args...); }
    }

    if (level >= gc.sink.consoleLevel || (level >= gc.sink.liveNotifLevel && gc.sink.liveNotifCbk)) {
        char fullLogBuffer[8192];
        char filledFormatBuffer[8192];
        formatArgs(filledFormatBuffer, sizeof(filledFormatBuffer), format, args...);
        if (level >= gc.sink.liveNotifLevel && gc.sink.liveNotifCbk) {
            gc.sink.liveNotifCbk(gc.timeConverter.getUtcNs(getHiResClockTick()), (uint32_t)level, getThreadName(), category,
                                 filledFormatBuffer, nullptr, 0);
        }
        if (level >= gc.sink.consoleLevel) {
            gc.textFormatter.format(fullLogBuffer, sizeof(fullLogBuffer), gc.timeConverter.getUtcNs(getHiResClockTick()), (uint32_t)level,
                                    getThreadName(), category, filledFormatBuffer, nullptr, 0, true);
            std::lock_guard<std::mutex> lkConsole(gc.consoleMx);
            fwrite(fullLogBuffer, 1, strlen(fullLogBuffer), stdout);  // fwrite is much faster than printf with non-formatting strings
        }
    }
    if (withLock) gc.loggingLock.unlock();
}

// Buffer logging
template<typename... Args>
inline void
logBuffer(hashStr_t formatHash, hashStr_t categoryHash, const char* format, const char* category, Level level, uint8_t* buffer,
          size_t bufferSize, Args... args)
{
    std::lock_guard<std::mutex> lkLogging(gc.loggingLock);
    if (!gc.enabled) { return; }
    if (level >= gc.sink.storageLevel || level >= gc.sink.detailsLevel) {
        // Compute the log storage size
        int      argQty         = getArgQty(args...) + 1;  // Last one is buffer
        int      argFlagsBytes  = 1 + (argQty / 2);
        uint32_t logAllArgsSize = argFlagsBytes + getLogStorageSize(args...);

        // Write the header
        uint8_t* buf = logHeader(true, argQty, (uint32_t)(logAllArgsSize + 4 + bufferSize /*size + buffer*/), level, formatHash,
                                 categoryHash, format, category);
        if (buf != nullptr) {
            // Write the arguments
            logArgument(0, buf, buf + argFlagsBytes, args...);
            // Then the buffer
            uint8_t* bufFlag = buf + ((argQty - 1) / 2);
            if ((argQty - 1) & 1) {  // Buffer is the last argument
                *bufFlag |= SSLOG_DATA_BUFFER;
            } else {
                *bufFlag = (SSLOG_DATA_BUFFER << 4);
            }
            buf += logAllArgsSize;
            // Write the buffer size (4 bytes)
            uint32_t       localBufferSize = (uint32_t)bufferSize;
            const uint8_t* src             = (uint8_t*)&localBufferSize;
            for (uint32_t i = 0; i < sizeof(uint32_t); ++i) { (*buf++) = (*src++); }  // 4 bytes for the size
            // Write the buffer content
            src = buffer;
            for (uint32_t i = 0; i < (uint32_t)bufferSize; ++i) { (*buf++) = (*src++); }
        }
    }
    if (level >= gc.sink.consoleLevel || (level >= gc.sink.liveNotifLevel && gc.sink.liveNotifCbk)) {
        char filledFormatBuffer[8192];
        formatArgs(filledFormatBuffer, sizeof(filledFormatBuffer), format, args...);
        if (level >= gc.sink.liveNotifLevel && gc.sink.liveNotifCbk) {
            gc.sink.liveNotifCbk(gc.timeConverter.getUtcNs(getHiResClockTick()), (uint32_t)level, getThreadName(), category,
                                 filledFormatBuffer, nullptr, 0);
        }
        if (level >= gc.sink.consoleLevel) {
            char fullLogBuffer[8192];
            gc.textFormatter.format(fullLogBuffer, sizeof(fullLogBuffer), gc.timeConverter.getUtcNs(getHiResClockTick()), (uint32_t)level,
                                    getThreadName(), category, filledFormatBuffer, buffer, (uint32_t)bufferSize, true);
            std::lock_guard<std::mutex> lkConsole(gc.consoleMx);
            fwrite(fullLogBuffer, 1, strlen(fullLogBuffer), stdout);
        }
    }
}

// =======================================================================================================
// Flushing task
// =======================================================================================================

inline void
createBaseFile()
{
    constexpr int headerSize = 32;
    uint8_t       header[headerSize];
    int           offset = 0;

    // Magic string to discriminate the type of file - 6 bytes
    for (int i = 0; i < 6; ++i) header[offset + i] = ("SSSSSS"[i]);
    offset += 6;

    // Format version - 2 bytes
    header[offset + 0] = (SSLOG_STORAGE_FORMAT_VERSION >> 0) & 0xFF;
    header[offset + 1] = (SSLOG_STORAGE_FORMAT_VERSION >> 8) & 0xFF;
    offset += 2;

    // Session random ID - just to check consistency of files
    assert(sizeof(gc.sessionId) == 8);
    for (size_t i = 0; i < 8; ++i) { header[offset + i] = gc.sessionId[i]; }
    offset += 8;

    // System 64 bits of the start of the session - 8 bytes
    clockNs_t tmp      = gc.utcSystemClockStartNs;
    header[offset + 0] = (tmp >> 0) & 0xFF;
    header[offset + 1] = (tmp >> 8) & 0xFF;
    header[offset + 2] = (tmp >> 16) & 0xFF;
    header[offset + 3] = (tmp >> 24) & 0xFF;
    header[offset + 4] = (tmp >> 32) & 0xFF;
    header[offset + 5] = (tmp >> 40) & 0xFF;
    header[offset + 6] = (tmp >> 48) & 0xFF;
    header[offset + 7] = static_cast<uint8_t>((tmp >> 56) & 0xFF);
    offset += 8;

    // High resolution rate - 8 bytes
    double tickToNs    = gc.timeConverter.getTickToNs();
    char*  tmp1        = (char*)&tickToNs;
    tmp                = *(uint64_t*)tmp1;  // Avoids warning about strict aliasing
    header[offset + 0] = (uint8_t)((tmp >> 0) & 0xFF);
    header[offset + 1] = ((uint8_t)(tmp >> 8) & 0xFF);  // Standard IEEE 754 format, little endian
    header[offset + 2] = (uint8_t)((tmp >> 16) & 0xFF);
    header[offset + 3] = (uint8_t)((tmp >> 24) & 0xFF);
    header[offset + 4] = (uint8_t)((tmp >> 32) & 0xFF);
    header[offset + 5] = (uint8_t)((tmp >> 40) & 0xFF);
    header[offset + 6] = (uint8_t)((tmp >> 48) & 0xFF);
    header[offset + 7] = (uint8_t)((tmp >> 56) & 0xFF);
    offset += 8;

    // Write
    assert(offset == headerSize);
    assert(!gc.sink.path.empty());
    if (gc.fileBaseHandle == nullptr) {
        // Create the base file. The directory has been created beforehand
        if ((gc.fileBaseHandle = fopen((gc.pathname / "base.sslog").string().c_str(), "wb")) == nullptr) {
            fprintf(stderr, "SSLOG error: unable to open the log base file '%s/base'\n", gc.pathname.string().c_str());
            gc.enabled = false;  // The logging service is fully disabled
            return;
        }
        fwrite((void*)&header[0], 1, headerSize, gc.fileBaseHandle);
    }
}

inline void
createNewDataFile(clockNs_t utcSystemClockOriginNs, clockTick_t steadyClockOriginTick)
{
    constexpr int headerSize = 32;
    uint8_t       header[headerSize];
    int           offset = 0;

    // Magic string to discriminate the type of file - 6 bytes
    for (int i = 0; i < 6; ++i) header[offset + i] = ("SSSSSS"[i]);
    offset += 6;

    // Format version - 2 bytes
    header[offset + 0] = (SSLOG_STORAGE_FORMAT_VERSION >> 0) & 0xFF;
    header[offset + 1] = (SSLOG_STORAGE_FORMAT_VERSION >> 8) & 0xFF;
    offset += 2;

    // Session random ID - just to check consistency of files
    assert(sizeof(gc.sessionId) == 8);
    for (size_t i = 0; i < 8; ++i) { header[offset + i] = gc.sessionId[i]; }
    offset += 8;

    // System 64 bits - 8 bytes
    clockNs_t tmp      = utcSystemClockOriginNs;
    header[offset + 0] = (tmp >> 0) & 0xFF;
    header[offset + 1] = (tmp >> 8) & 0xFF;
    header[offset + 2] = (tmp >> 16) & 0xFF;
    header[offset + 3] = (tmp >> 24) & 0xFF;
    header[offset + 4] = (tmp >> 32) & 0xFF;
    header[offset + 5] = (tmp >> 40) & 0xFF;
    header[offset + 6] = (tmp >> 48) & 0xFF;
    header[offset + 7] = static_cast<uint8_t>((tmp >> 56) & 0xFF);
    offset += 8;

    // High resolution counter - 8 bytes
    tmp                = steadyClockOriginTick;
    header[offset + 0] = (tmp >> 0) & 0xFF;
    header[offset + 1] = (tmp >> 8) & 0xFF;
    header[offset + 2] = (tmp >> 16) & 0xFF;
    header[offset + 3] = (tmp >> 24) & 0xFF;
    header[offset + 4] = (tmp >> 32) & 0xFF;
    header[offset + 5] = (tmp >> 40) & 0xFF;
    header[offset + 6] = (tmp >> 48) & 0xFF;
    header[offset + 7] = static_cast<uint8_t>((tmp >> 56) & 0xFF);
    offset += 8;

    // Create data file(s)
    assert(offset == headerSize);
    assert(!gc.pathname.empty());
    bool filesWereWritten = false;

    if (gc.fileDataHandle[0] == nullptr && gc.sink.storageLevel < Level::off) {
        char numberedFilename[32];
        snprintf(numberedFilename, sizeof(numberedFilename), "data%06u.sslog", gc.dataFileNumber);
        SSLOG_FS_NAMESPACE::path dataFilename = gc.pathname / numberedFilename;
        if ((gc.fileDataHandle[0] = fopen(dataFilename.string().c_str(), "wb")) == nullptr) {
            fprintf(stderr, "SSLOG error: unable to open the log data file '%s'\n", dataFilename.string().c_str());
            if (gc.fileBaseHandle != nullptr) fclose(gc.fileBaseHandle);
            gc.fileBaseHandle = nullptr;
            gc.enabled        = false;  // The logging service is fully disabled
            return;
        }
        fwrite((void*)&header[0], 1, headerSize, gc.fileDataHandle[0]);
        filesWereWritten = true;
    }

    if (gc.fileDataHandle[1] == nullptr && gc.sink.detailsLevel < gc.sink.storageLevel) {
        char numberedFilename[32];
        snprintf(numberedFilename, sizeof(numberedFilename), "data%06u.dtl.sslog", gc.dataFileNumber);
        SSLOG_FS_NAMESPACE::path dataFilename = gc.pathname / numberedFilename;
        if ((gc.fileDataHandle[1] = fopen(dataFilename.string().c_str(), "wb")) == nullptr) {
            fprintf(stderr, "SSLOG error: unable to open the log data file '%s'\n", dataFilename.string().c_str());
            if (gc.fileBaseHandle != nullptr) fclose(gc.fileBaseHandle);
            gc.fileBaseHandle = nullptr;
            if (gc.fileDataHandle[0] != nullptr) fclose(gc.fileDataHandle[0]);
            gc.fileDataHandle[0] = nullptr;
            gc.enabled           = false;  // The logging service is fully disabled
            return;
        }
        fwrite((void*)header, 1, headerSize, gc.fileDataHandle[1]);
        gc.currentDetailedFileStartTick =
            gc.dataBanks[gc.dataActiveBankNbr ^ 1].steadyClockOriginTick;  // Date of first log in first written bank
        filesWereWritten = true;
    }

    if (filesWereWritten) {
        gc.dataFileNumber++;
        gc.dataFileSwitchTick  = getHiResClockTick() + gc.timeConverter.durationNsToTick(1e9 * gc.sink.splitFileMaxDurationSec);
        gc.dataFileCurrentSize = 0;  // Sum of standard + details
        ++gc.stats.createdDataFiles;
    }
}

inline bool
periodicLogFlush(bool doForce)
{
    {
        // Ensure that all the write of the previous bank is finished
        std::unique_lock<std::mutex> lkLogging(gc.loggingLock);
        clockTick_t                  nowTick = getHiResClockTick();

        // Rate limit the flushing calls if *all* these conditions are fulfilled:
        if (
            // No forced work
            !doForce &&
            // No string buffer saturation
            gc.isStringBufferSaturated.load() == 0 &&
            // No data buffer saturation
            gc.isDataBufferSaturated.load() == 0 &&
            // The maximum flushing period is not reached (user defined)
            (nowTick - gc.lastFlushedBufferTick) < gc.timeConverter.durationNsToTick(1e6 * gc.collector.flushingMaxLatencyMs) &&
            // 1/8 of the string collection buffer is not exceeded
            gc.stringBanks[gc.stringActiveBankNbr].offset < gc.collector.stringBufferBytes / 8 &&
            // 1/8 of the data collection buffer is not exceeded (standard or details)
            gc.dataBanks[gc.dataActiveBankNbr].offset[0] < gc.collector.dataBufferBytes / 8 &&
            gc.dataBanks[gc.dataActiveBankNbr].offset[1] < gc.collector.dataBufferBytes / 8 &&
            // The file split rule by duration is not reached
            (gc.sink.splitFileMaxDurationSec == 0 || nowTick < gc.dataFileSwitchTick)) {
            return false;  // No need to recollect another time with short loop
        }
        gc.lastFlushedBufferTick = nowTick;
    }

    // Strings
    {
        int         nextStringActiveBankNbr = gc.stringActiveBankNbr ^ 1;
        StringBank& nextStringActiveBank    = gc.stringBanks[nextStringActiveBankNbr];
        uint32_t    stringBankByteSize      = nextStringActiveBank.offset;
        if (stringBankByteSize > 0) {
            // Create the base file if needed
            if (!gc.sink.path.empty() && gc.fileBaseHandle == nullptr) { createBaseFile(); }

            // Write the new strings
            if (gc.fileBaseHandle != nullptr) {
                fwrite((void*)nextStringActiveBank.buffer.data(), 1, stringBankByteSize, gc.fileBaseHandle);
                gc.stats.storedBytes += stringBankByteSize;
            }
            if (stringBankByteSize > gc.stats.maxUsageStringBufferBytes) { gc.stats.maxUsageStringBufferBytes = stringBankByteSize; }
            nextStringActiveBank.offset = 0;
        }

        // Install the yet inactive bank as a new fresh active one
        std::unique_lock<std::mutex> lkLogging(gc.loggingLock);
        gc.stringActiveBankNbr = nextStringActiveBankNbr;
    }

    // Get infos on the inactive data bank
    int       nextDataActiveBankNbr = gc.dataActiveBankNbr ^ 1;
    DataBank& nextDataActiveBank    = gc.dataBanks[nextDataActiveBankNbr];
    uint32_t  allDataBankBytes      = nextDataActiveBank.offset[0] + nextDataActiveBank.offset[1];
    for (int bufNbr = 0; bufNbr < 2; ++bufNbr) {
        if (nextDataActiveBank.offset[bufNbr] > gc.stats.maxUsageDataBufferBytes) {
            gc.stats.maxUsageDataBufferBytes = nextDataActiveBank.offset[bufNbr];
        }
    }

    // Create the data files if needed
    if (!gc.sink.path.empty() &&
        ((gc.fileDataHandle[0] == nullptr && gc.sink.storageLevel < Level::off && nextDataActiveBank.offset[0] > 0) ||
         (gc.fileDataHandle[1] == nullptr && gc.sink.detailsLevel < gc.sink.storageLevel && nextDataActiveBank.offset[1] > 0))) {
        createNewDataFile(nextDataActiveBank.utcSystemClockOriginNs, nextDataActiveBank.steadyClockOriginTick);
    }

    // Write the buffers
    for (int bufNbr = 0; bufNbr < 2; ++bufNbr) {
        if (gc.fileDataHandle[bufNbr] != nullptr && nextDataActiveBank.offset[bufNbr] > 0) {
            fwrite((void*)nextDataActiveBank.buffer[bufNbr].data(), 1, nextDataActiveBank.offset[bufNbr], gc.fileDataHandle[bufNbr]);
            gc.dataFileCurrentSize += nextDataActiveBank.offset[bufNbr];
            gc.stats.storedBytes += nextDataActiveBank.offset[bufNbr];
        }
    }

    // Install the yet inactive bank as a new fresh active one
    std::unique_lock<std::mutex> lkLogging(gc.loggingLock);
    nextDataActiveBank.offset[0]             = 0;
    nextDataActiveBank.offset[1]             = 0;
    gc.deltaEncoding[0].doLogFullEncoding    = true;  // Force full encoding at start of each batch as they could be the start of a new file
    gc.deltaEncoding[1].doLogFullEncoding    = true;
    nextDataActiveBank.steadyClockOriginTick = getHiResClockTick();  // Snapshot before any events in the buffer
    nextDataActiveBank.utcSystemClockOriginNs = getUtcSystemClockNs();
    gc.dataActiveBankNbr                      = nextDataActiveBankNbr;

    // Log it if some saturation has been detected
    if (gc.isDataBufferSaturated.load() != 0) {
        log(false, SSLOG_STRINGHASH("SATURATED DATA BUFFER"), SSLOG_STRINGHASH("SSLOG"), "SATURATED DATA BUFFER", "SSLOG",
            sslog::Level::error);
        gc.isDataBufferSaturated.store(0);
    }

    return (allDataBankBytes > 0);
}

inline void
cleanOldDataFiles(clockTick_t nowTick)
{
    // Remove data files in excess, or too old
    clockTick_t dataFileEndTick = nowTick - gc.timeConverter.durationNsToTick(1e9 * gc.sink.fileMaxFileAgeSec);
    while (!gc.existingDataFiles.empty() && ((gc.sink.fileMaxFileAgeSec > 0 && gc.existingDataFiles.front().endTick < dataFileEndTick) ||
                                             (gc.sink.fileMaxQty > 0 && gc.existingDataFiles.size() > gc.sink.fileMaxQty))) {
        std::error_code ec;
        char            numberedFilename[32];
        snprintf(numberedFilename, sizeof(numberedFilename), "data%06u.sslog", gc.existingDataFiles.front().fileNumber);
        if (SSLOG_FS_NAMESPACE::exists(gc.pathname / numberedFilename) && !SSLOG_FS_NAMESPACE::remove(gc.pathname / numberedFilename, ec)) {
            fprintf(stderr, "SSLOG error: unable to remove the details file '%s': %s\n", (gc.pathname / numberedFilename).string().c_str(),
                    ec.message().c_str());
        }
        snprintf(numberedFilename, sizeof(numberedFilename), "data%06u.dtl.sslog", gc.existingDataFiles.front().fileNumber);
        if (SSLOG_FS_NAMESPACE::exists(gc.pathname / numberedFilename) && !SSLOG_FS_NAMESPACE::remove(gc.pathname / numberedFilename, ec)) {
            fprintf(stderr, "SSLOG error: unable to remove the details file '%s': %s\n", (gc.pathname / numberedFilename).string().c_str(),
                    ec.message().c_str());
        }
        gc.existingDataFiles.erase(gc.existingDataFiles.begin());
        ++gc.stats.removedDataFiles;
    }
}

inline void
cleanOldDetailedFiles(clockTick_t nowTick)
{
    // Remove details files not covered by any request period
    clockTick_t detailedFileEndTick = nowTick - gc.timeConverter.durationNsToTick(1e9 * gc.sink.detailsBeforeAfterMinSec);
    while (!gc.detailedFilesToDelete.empty() && gc.detailedFilesToDelete.front().endTick < detailedFileEndTick) {
        std::error_code ec;
        char            numberedFilename[32];
        snprintf(numberedFilename, sizeof(numberedFilename), "data%06u.dtl.sslog", gc.detailedFilesToDelete.front().fileNumber);
        if (SSLOG_FS_NAMESPACE::exists(gc.pathname / numberedFilename) && !SSLOG_FS_NAMESPACE::remove(gc.pathname / numberedFilename, ec)) {
            fprintf(stderr, "SSLOG error: unable to remove the details file '%s': %s\n", (gc.pathname / numberedFilename).string().c_str(),
                    ec.message().c_str());
        }
        gc.detailedFilesToDelete.erase(gc.detailedFilesToDelete.begin());
    }
}

inline void
flushTask()
{
    // Start the flushing thread
    gc.flushThreadId         = SSLOG_GET_SYS_THREAD_ID();  // So that we can identify this thread if it crashes
    gc.lastFlushedBufferTick = getHiResClockTick();
    gc.dataFileSwitchTick    = getHiResClockTick() + gc.timeConverter.durationNsToTick(1e9 * gc.sink.splitFileMaxDurationSec);
    gc.enabled               = true;

    // Notify the initialization thread that the flushing thread is ready
    {
        std::lock_guard<std::mutex> lkInit(gc.threadInitMx);
        gc.threadIsStarted = true;
        gc.threadInitCv.notify_one();
    }

    // Thread loop
    while (!gc.threadFlusherFlagStop.load()) {
        // Apply collector config changes
        if (gc.newCollectorCfgPresent.load() != 0) {
            {
                std::unique_lock<std::mutex> lkLogging(gc.loggingLock);
                gc.initCollector();  // Content of collection buffers are lost
            }
            gc.threadIsConfigApplied.store(1);
        }

        // Apply sink config changes
        if (gc.newSinkCfgPresent.load() != 0) {
            {
                std::unique_lock<std::mutex> lkLogging(gc.loggingLock);
                gc.initSink();
            }
            gc.threadIsConfigApplied.store(1);
        }

        // Collect logs
        bool workWasDone = periodicLogFlush(false);

        // Is a new request for details received?
        clockTick_t nowTick = getHiResClockTick();
        if (gc.detailsRequested.exchange(0) != 0) {
            // Clear the list of detailed files to delete (covered by this request for details)
            gc.detailedFilesToDelete.clear();
            // Compute the new end date of the request-covered period
            gc.detailedFileEndTick = nowTick + gc.timeConverter.durationNsToTick(1e9 * gc.sink.detailsBeforeAfterMinSec);
            ++gc.stats.requestForDetailsQty;
        }

        // Handle the multiple data file strategy
        if (!gc.sink.path.empty() && (gc.fileDataHandle[0] != nullptr || gc.fileDataHandle[1] != nullptr) &&
            ((gc.sink.splitFileMaxDurationSec > 0 && nowTick > gc.dataFileSwitchTick) ||
             (gc.sink.splitFileMaxBytes > 0 && gc.dataFileCurrentSize >= gc.sink.splitFileMaxBytes))) {
            // Close the data file. The next one will be created when needed
            if (gc.fileDataHandle[0]) {
                gc.existingDataFiles.push_back({gc.dataFileNumber - 1, nowTick});
                fclose(gc.fileDataHandle[0]);
            }
            // Close the details file
            if (gc.fileDataHandle[1]) {
                fclose(gc.fileDataHandle[1]);
                // Store in the potential deletion list if not covered already by a request for details
                if (gc.currentDetailedFileStartTick >= gc.detailedFileEndTick) {
                    gc.detailedFilesToDelete.push_back({gc.dataFileNumber - 1, nowTick});
                }
            }
            gc.fileDataHandle[0] = 0;
            gc.fileDataHandle[1] = 0;
        }

        // Remove too old data files or if too much of them
        cleanOldDataFiles(nowTick);

        // Remove old detailed files not covered by a request
        cleanOldDetailedFiles(nowTick);

        // "Flush" is finished
        if (gc.forceFlush.load() != 0) {
            gc.forceFlush.store(0);
            gc.threadIsConfigApplied.store(1);
        }

        // Sleep only if no work was done
        if (!workWasDone) {
            std::unique_lock<std::mutex> lkSync(gc.threadSyncMx);
            gc.threadSyncCv.wait_for(lkSync, std::chrono::milliseconds(10),
                                     [&] { return gc.threadFlusherFlagStop.load() || gc.threadShallWakeUp; });
            gc.threadShallWakeUp = false;
        }
    }  // End of thread loop

    // Complete the flushing
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    periodicLogFlush(true);  // Flush the previous bank
    periodicLogFlush(true);  // Flush the current bank

    // Cleaning communication
    clockTick_t nowTick = getHiResClockTick();
    if (gc.fileBaseHandle) { fclose(gc.fileBaseHandle); }
    if (gc.fileDataHandle[0]) {
        fclose(gc.fileDataHandle[0]);
        gc.existingDataFiles.push_back({gc.dataFileNumber - 1, nowTick});
    }
    if (gc.fileDataHandle[1]) {
        fclose(gc.fileDataHandle[1]);
        if (gc.currentDetailedFileStartTick >= gc.detailedFileEndTick) {
            gc.detailedFilesToDelete.push_back({gc.dataFileNumber - 1, nowTick});
        }
    }
    cleanOldDataFiles(nowTick);
    cleanOldDetailedFiles(0xFFFFFFFFFFFFFFFFULL);  // Assume no more request for details sent until end of times
    gc.fileBaseHandle    = 0;
    gc.fileDataHandle[0] = 0;
    gc.fileDataHandle[1] = 0;
}

// =======================================================================================================
// Signals and exception handlers
// =======================================================================================================

#if defined(__unix__) && SSLOG_STACKTRACE == 1
inline void
crashLogStackTrace()
{
    constexpr int MaxStringLength = 2048;
    char          msgStr[MaxStringLength];

    // Initialize libunwind
    unw_context_t uc;
    unw_getcontext(&uc);
    unw_cursor_t cursor;
    unw_init_local(&cursor, &uc);
    char       localMsgStr[1024];
    unw_word_t offset;
    unw_word_t ip;

    // Initialize DWARF reading
    char*          debugInfoPath = nullptr;
    Dwfl_Callbacks callbacks     = {};
    callbacks.find_elf           = dwfl_linux_proc_find_elf;
    callbacks.find_debuginfo     = dwfl_standard_find_debuginfo;
    callbacks.debuginfo_path     = &debugInfoPath;
    Dwfl* dwfl                   = dwfl_begin(&callbacks);
    if (!dwfl || dwfl_linux_proc_report(dwfl, getpid()) != 0 || dwfl_report_end(dwfl, nullptr, nullptr) != 0) { return; }

    const int skipDepthQty = 2;  // No need to display the bottom machinery
    int       depth        = 0;
    // Loop on stack depth
    while (unw_step(&cursor) > 0) {
        unw_get_reg(&cursor, UNW_REG_IP, &ip);

        if (depth >= skipDepthQty) {
            Dwarf_Addr   addr   = (uintptr_t)(ip - 4);
            Dwfl_Module* module = dwfl_addrmodule(dwfl, addr);
            Dwfl_Line*   line   = dwfl_getsrc(dwfl, addr);

            if (line) {
                Dwarf_Addr  addr2;
                int         lineNbr;
                int         status;
                const char* filename      = dwfl_lineinfo(line, &addr2, &lineNbr, nullptr, nullptr, nullptr);
                char*       demangledName = abi::__cxa_demangle(dwfl_module_addrname(module, addr), 0, 0, &status);
                // Filename and line first in the potentially truncated remote log (demangled function name may be long)
                snprintf(msgStr, MaxStringLength, "   #%-2d %s(%d) : %s", depth - skipDepthQty,
                         filename ? strrchr(filename, '/') + 1 : "<unknown>", filename ? lineNbr : 0,
                         status ? dwfl_module_addrname(module, addr) : demangledName);
                if (status == 0) free(demangledName);
            } else {
                snprintf(msgStr, MaxStringLength, "   #%-2d 0x%" PRIX64 " : %s", depth - skipDepthQty, ip - 4,
                         dwfl_module_addrname(module, addr));
            }

            // Store
            ssCritical("CRASH", "%s", msgStr);
        }

        // Next unwinding
        localMsgStr[0] = 0;
        unw_get_proc_name(&cursor, localMsgStr, sizeof(localMsgStr), &offset);  // Fails if there is no debug symbols
        if (!strcmp(localMsgStr, "main")) break;
        ++depth;
    }  // End of unwinding

    // End session
    dwfl_end(dwfl);
}
#endif  // if defined(__unix__) && SSLOG_STACKTRACE==1

#if defined(_MSC_VER) && SSLOG_STACKTRACE == 1
inline void
crashLogStackTrace()
{
    constexpr int MaxStringLength = 2048;
    char          msgStr[MaxStringLength];
    char          tmpStr[32];
    char          depthStr[8];

    // Get the addresses of the stacktrace
    PVOID stacktrace[64];  // 64 levels of depth should be enough for everyone
    int   foundStackDepth = gc.rtlWalkFrameChain ? gc.rtlWalkFrameChain(stacktrace, 64, 0) : 0;

    // Some required windows structures for the used APIs
    IMAGEHLP_LINE64 line;
    line.SizeOfStruct          = sizeof(IMAGEHLP_LINE64);
    DWORD         displacement = 0;
    constexpr int MaxNameSize  = 8192;
    char          symBuffer[sizeof(SYMBOL_INFO) + MaxNameSize];
    SYMBOL_INFO*  symInfo = (SYMBOL_INFO*)symBuffer;
    symInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
    symInfo->MaxNameLen   = MaxNameSize;
    HANDLE proc           = GetCurrentProcess();

#define SSLOG_CRASH_STACKTRACE_DUMP_INFO_(itemNbrStr, colorItem, colorFunc, colorNeutral)                                           \
    if (isFuncValid || isLineValid) {                                                                                               \
        snprintf(tmpStr, sizeof(tmpStr), "(%u)", isLineValid ? line.LineNumber : 0);                                                \
        snprintf(msgStr, MaxStringLength, "%s %s%s : %s", itemNbrStr, isLineValid ? strrchr(line.FileName, '\\') + 1 : "<unknown>", \
                 isLineValid ? tmpStr : "", isFuncValid ? symInfo->Name : "<unknown>");                                             \
    } else {                                                                                                                        \
        snprintf(msgStr, MaxStringLength, "%s 0x%" PRIX64, itemNbrStr, ptr);                                                        \
    }                                                                                                                               \
    ssCritical("CRASH", "%s", msgStr);

    const int skipDepthQty = 3;  // No need to display the bottom machinery
    for (int depth = skipDepthQty; depth < foundStackDepth; ++depth) {
        uint64_t ptr =
            ((uint64_t)stacktrace[depth]) - 1;  // -1 because the captured PC is already pointing on the next code line at snapshot time

        // Get the nested inline function calls, if any
        DWORD frameIdx, curContext = 0;
        int   inlineQty = SymAddrIncludeInlineTrace(proc, ptr);
        if (inlineQty > 0 && SymQueryInlineTrace(proc, ptr, 0, ptr, ptr, &curContext, &frameIdx)) {
            for (int i = 0; i < inlineQty; ++i) {
                bool isFuncValid = (SymFromInlineContext(proc, ptr, curContext, 0, symInfo) != 0);
                bool isLineValid = (SymGetLineFromInlineContext(proc, ptr, curContext, 0, &displacement, &line) != 0);
                ++curContext;
                if (gc.sink.consoleMode == ConsoleMode::Color) {
                    SSLOG_CRASH_STACKTRACE_DUMP_INFO_("inl", "\033[93m", "\033[36m", "\033[0m");
                } else {
                    SSLOG_CRASH_STACKTRACE_DUMP_INFO_("inl", "", "", "");
                }
            }
        }

        // Get the function call for this depth
        bool isFuncValid = (SymFromAddr(proc, ptr, 0, symInfo) != 0);
        bool isLineValid = (SymGetLineFromAddr64(proc, ptr - 1, &displacement, &line) != 0);
        snprintf(depthStr, sizeof(depthStr), "#%-2d", depth - skipDepthQty);
        if (gc.sink.consoleMode == ConsoleMode::Color) {
            SSLOG_CRASH_STACKTRACE_DUMP_INFO_(depthStr, "\033[93m", "\033[36m", "\033[0m");
        } else {
            SSLOG_CRASH_STACKTRACE_DUMP_INFO_(depthStr, "", "", "");
        }
    }  // End of loop on stack depth
}
#endif  // if defined(_MSC_VER) && SSLOG_STACKTRACE==1

inline void
ssCrash(const char* message)
{
    static bool alreadyCrashed = false;
    // Do not log if the crash is located inside the 'sslog' flushing thread
    if (gc.threadFlusher && (int)SSLOG_GET_SYS_THREAD_ID() == gc.flushThreadId) { gc.enabled = false; }
    if (alreadyCrashed) {
        return;  // Not good but better than looping
    }
    alreadyCrashed = true;

    // Log and display the crash message
    gc.detailsRequested.store(1);
    gc.sink.consoleLevel = Level::critical;
    ssCritical("CRASH", "%s", message);

    // Log and display the call stack
#if SSLOG_STACKTRACE == 1
    crashLogStackTrace();
#endif

    // Properly stop any recording, but without cleaning
    gc.doNotUninit = true;
    stop();

    // Stop the process
    quick_exit(1);
}

[[maybe_unused]] static void
signalHandler(int signalId)
{
    const char* sigDescr = "*Unknown*";
    switch (signalId) {
        case SIGABRT:
            sigDescr = "Abort";
            break;
        case SIGFPE:
            sigDescr = "Floating point exception";
            break;
        case SIGILL:
            sigDescr = "Illegal instruction";
            break;
        case SIGSEGV:
            sigDescr = "Segmentation fault";
            break;
        case SIGINT:
            sigDescr = "Interrupt";
            break;
        case SIGTERM:
            sigDescr = "Termination";
            break;
#if defined(__unix__)
        case SIGPIPE:
            sigDescr = "Broken pipe";
            break;
#endif
        default:
            break;
    }
    char infoStr[256];
    snprintf(infoStr, sizeof(infoStr), "[SSLOG] signal %d '%s' received", signalId, sigDescr);
    ssCrash(infoStr);
}

#if _MSC_VER
// Specific to windows, on top of the signal handler
inline LONG WINAPI
exceptionHandler(struct _EXCEPTION_POINTERS* pExcept)
{
    char         infoStr[256];
    int          tmp;
    unsigned int code = pExcept->ExceptionRecord->ExceptionCode;
#define SSLOG_LOG_EXCEPTION_(str)                                                \
    snprintf(infoStr, sizeof(infoStr), "[SSLOG] exception '%s' received.", str); \
    ssCrash(infoStr)

    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:
            tmp = (int)pExcept->ExceptionRecord->ExceptionInformation[0];
            snprintf(infoStr, sizeof(infoStr), "[SSLOG] exception 'ACCESS_VIOLATION' (%s) received.",
                     (tmp == 0) ? "read" : ((tmp == 1) ? "write" : "user-mode data execution prevention (DEP) violation"));
            ssCrash(infoStr);
            break;
        case EXCEPTION_BREAKPOINT:
            break;  // Let this one go through the handler
        case EXCEPTION_SINGLE_STEP:
            break;  // Let this one go through the handler
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            SSLOG_LOG_EXCEPTION_("ARRAY_BOUNDS_EXCEEDED");
            break;
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            SSLOG_LOG_EXCEPTION_("DATATYPE_MISALIGNMENT");
            break;
        case EXCEPTION_FLT_DENORMAL_OPERAND:
            SSLOG_LOG_EXCEPTION_("FLT_DENORMAL_OPERAND");
            break;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            SSLOG_LOG_EXCEPTION_("FLT_DIVIDE_BY_ZERO");
            break;
        case EXCEPTION_FLT_INEXACT_RESULT:
            SSLOG_LOG_EXCEPTION_("FLT_INEXACT_RESULT");
            break;
        case EXCEPTION_FLT_INVALID_OPERATION:
            SSLOG_LOG_EXCEPTION_("FLT_INVALID_OPERATION");
            break;
        case EXCEPTION_FLT_OVERFLOW:
            SSLOG_LOG_EXCEPTION_("FLT_OVERFLOW");
            break;
        case EXCEPTION_FLT_STACK_CHECK:
            SSLOG_LOG_EXCEPTION_("FLT_STACK_CHECK");
            break;
        case EXCEPTION_FLT_UNDERFLOW:
            SSLOG_LOG_EXCEPTION_("FLT_UNDERFLOW");
            break;
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            SSLOG_LOG_EXCEPTION_("ILLEGAL_INSTRUCTION");
            break;
        case EXCEPTION_IN_PAGE_ERROR:
            SSLOG_LOG_EXCEPTION_("IN_PAGE_ERROR");
            break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            SSLOG_LOG_EXCEPTION_("INT_DIVIDE_BY_ZERO");
            break;
        case EXCEPTION_INT_OVERFLOW:
            SSLOG_LOG_EXCEPTION_("INT_OVERFLOW");
            break;
        case EXCEPTION_INVALID_DISPOSITION:
            SSLOG_LOG_EXCEPTION_("INVALID_DISPOSITION");
            break;
        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
            SSLOG_LOG_EXCEPTION_("NONCONTINUABLE_EXCEPTION");
            break;
        case EXCEPTION_PRIV_INSTRUCTION:
            SSLOG_LOG_EXCEPTION_("PRIV_INSTRUCTION");
            break;
        case EXCEPTION_STACK_OVERFLOW:
            SSLOG_LOG_EXCEPTION_("STACK_OVERFLOW");
            break;
        default:
            SSLOG_LOG_EXCEPTION_("UNKNOWN");
            break;
    }
    // Go to the next handler
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif  // if _MSC_VER

// =======================================================================================================
// Public API
// =======================================================================================================

inline void
setCollector(const Collector& config)
{
    std::unique_lock<std::mutex> lkConfig(gc.threadConfigMx);
    // Set the new config in the second config bank
    gc.newCollectorCfg                   = config;
    gc.newCollectorCfg.stringBufferBytes = sslogMax(gc.newCollectorCfg.stringBufferBytes, 1024U);  // Ensure sane minimum values
    gc.newCollectorCfg.dataBufferBytes   = sslogMax(gc.newCollectorCfg.dataBufferBytes, 16384U);
    gc.threadIsConfigApplied.store(0);

    if (gc.threadFlusher == nullptr) {
        // The flush thread is not started, we can set the new config directly
        gc.initCollector();
        return;
    }

    gc.newCollectorCfgPresent.store(1);
    // Wake up the flush thread
    {
        std::lock_guard<std::mutex> lkSync(gc.threadSyncMx);
        gc.threadShallWakeUp = true;
        gc.threadSyncCv.notify_one();
    }
    // Synchronous wait for application of the config
    while (gc.threadIsConfigApplied.load() == 0) { std::this_thread::yield(); }
    gc.threadIsConfigApplied.store(0);
}

inline void
setSink(const Sink& config)
{
    std::unique_lock<std::mutex> lkConfig(gc.threadConfigMx);
    // Set the new config in the second config bank
    gc.newSinkCfg = config;
    gc.threadIsConfigApplied.store(0);

    if (gc.threadFlusher == nullptr) {
        // The flush thread is not started, we can set the new config directly
        gc.initSink();
        return;
    }

    gc.newSinkCfgPresent.store(1);
    // Wake up the flush thread
    {
        std::lock_guard<std::mutex> lkSync(gc.threadSyncMx);
        gc.threadShallWakeUp = true;
        gc.threadSyncCv.notify_one();
    }
    // Synchronous wait for application of the config
    while (gc.threadIsConfigApplied.load() == 0) { std::this_thread::yield(); }
    gc.threadIsConfigApplied.store(0);
}

inline void
forceFlush()
{
    std::unique_lock<std::mutex> lkConfig(gc.threadConfigMx);
    gc.threadIsConfigApplied.store(0);

    if (gc.threadFlusher == nullptr) {  // The flush thread is not started, nothing to do
        return;
    }

    gc.forceFlush.store(1);  // Acts like a configuration change
    // Wake up the flush thread
    {
        std::lock_guard<std::mutex> lkSync(gc.threadSyncMx);
        gc.threadShallWakeUp = true;
        gc.threadSyncCv.notify_one();
    }
    // Synchronous wait for application of the config
    while (gc.threadIsConfigApplied.load() == 0) { std::this_thread::yield(); }
    gc.threadIsConfigApplied.store(0);
}

inline Collector
getCollector()
{
    std::unique_lock<std::mutex> lkConfig(gc.threadConfigMx);
    return gc.collector;
}

inline Sink
getSink()
{
    std::unique_lock<std::mutex> lkConfig(gc.threadConfigMx);
    return gc.sink;
}

inline void
setStoragePath(const std::string& path)
{
    Sink config = getSink();
    config.path = path;
    setSink(config);
}

inline void
setStorageLevel(Level level)
{
    Sink config         = getSink();
    config.storageLevel = level;
    setSink(config);
}

inline void
setConsoleLevel(Level level)
{
    Sink config         = getSink();
    config.consoleLevel = level;
    setSink(config);
}

inline void
setConsoleFormatter(const std::string& consoleFormatter)
{
    Sink config             = getSink();
    config.consoleFormatter = consoleFormatter;
    setSink(config);
}

inline void
requestForDetails()
{
    gc.detailsRequested.store(1);
}

inline bool
start()
{
    std::unique_lock<std::mutex> lkConfig(gc.threadConfigMx);
    if (gc.threadFlusher != nullptr) {
        return false;  // Already running
    }

    // Register POSIX signals
    memset(gc.signalsOldHandlers, 0, sizeof(gc.signalsOldHandlers));
#if SSLOG_NO_CATCH_SIGNALS == 0
    gc.signalsOldHandlers[0] = std::signal(SIGABRT, signalHandler);
    gc.signalsOldHandlers[1] = std::signal(SIGFPE, signalHandler);
    gc.signalsOldHandlers[2] = std::signal(SIGILL, signalHandler);
    gc.signalsOldHandlers[3] = std::signal(SIGSEGV, signalHandler);
#if SSLOG_NO_CATCH_SIGINT == 0
    gc.signalsOldHandlers[4] = std::signal(SIGINT, signalHandler);
#endif
    gc.signalsOldHandlers[5] = std::signal(SIGTERM, signalHandler);
#if defined(__unix__)
    gc.signalsOldHandlers[6] = std::signal(SIGPIPE, signalHandler);
#endif
    gc.signalHandlersSaved = true;
#if defined(_MSC_VER)
    // Register the exception handler
    gc.exceptionHandler = AddVectoredExceptionHandler(1, exceptionHandler);
#endif  // if defined(_MSC_VER)
#endif  // if SSLOG_NO_CATCH_SIGNALS==0

#if defined(_MSC_VER) && SSLOG_STACKTRACE == 1
    // Initialize the symbol reading for the stacktrace (in case of crash)
    SymInitialize(GetCurrentProcess(), 0, true);
    SymSetOptions(SYMOPT_LOAD_LINES);
    gc.rtlWalkFrameChain = (rtlWalkFrameChain_t)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlWalkFrameChain");
    assert(gc.rtlWalkFrameChain);
#endif  // if defined(_MSC_VER) && SSLOG_STACKTRACE==1

    // Reinitialize the contexts
    gc.init();

    // Create the flushing thread and wait for its readiness
    {
        gc.threadIsStarted = false;
        gc.threadFlusher   = new std::thread(flushTask);
        std::unique_lock<std::mutex> lkInit(gc.threadInitMx);
        gc.threadInitCv.wait(lkInit, [&] { return gc.threadIsStarted; });
    }

    // Init success
    return true;
}

inline void
stop()
{
    std::unique_lock<std::mutex> lkConfig(gc.threadConfigMx);
    if (gc.threadFlusher == nullptr) { return; }

    // Unregister signals
#if SSLOG_NO_CATCH_SIGNALS == 0
    if (gc.signalHandlersSaved) {
        gc.signalHandlersSaved = false;
        std::signal(SIGABRT, gc.signalsOldHandlers[0]);
        std::signal(SIGFPE, gc.signalsOldHandlers[1]);
        std::signal(SIGILL, gc.signalsOldHandlers[2]);
        std::signal(SIGSEGV, gc.signalsOldHandlers[3]);
#if SSLOG_NO_CATCH_SIGINT == 0
        std::signal(SIGINT, gc.signalsOldHandlers[4]);
#endif
        std::signal(SIGTERM, gc.signalsOldHandlers[5]);
#if defined(__unix__)
        std::signal(SIGPIPE, gc.signalsOldHandlers[6]);
#endif
#if defined(_MSC_VER)
        RemoveVectoredExceptionHandler(gc.exceptionHandler);
#endif  // if defined(_MSC_VER)
    }
#endif  // if SSLOG_NO_CATCH_SIGNALS==0

    // Stop the data collection thread
    gc.enabled = false;
    {
        // Notify end of collection thread and wake it up
        std::lock_guard<std::mutex> lkSync(gc.threadSyncMx);
        gc.threadFlusherFlagStop.store(1);
        gc.threadSyncCv.notify_one();
    }
    if (gc.doNotUninit) {
        // Wait for the TX thread to flush the last data (unless it is the crashing thread)
        if (gc.threadFlusher && gc.threadFlusher->joinable() && (int)SSLOG_GET_SYS_THREAD_ID() != gc.flushThreadId) {
            gc.threadFlusher->join();
        }
        // No cleaning, so stop here
        return;
    }
    if (gc.threadFlusher && gc.threadFlusher->joinable()) gc.threadFlusher->join();
    delete gc.threadFlusher;
    gc.threadFlusher = 0;
}

inline Stats
getStats()
{
    return gc.stats;
}

// =======================================================================================================
// Automatic instantiation
// =======================================================================================================

#ifndef SSLOG_NO_BOOTSTRAP  // Undocumented flag. Use with caution

struct Bootstrap {
    Bootstrap()
    {
#if SSLOG_NO_AUTOSTART == 0
        start();
#endif
    }
    ~Bootstrap() { stop(); }
    Bootstrap(Bootstrap const&)            = delete;
    Bootstrap(Bootstrap&&)                 = delete;
    Bootstrap& operator=(Bootstrap const&) = delete;
    Bootstrap& operator=(Bootstrap&&)      = delete;
};

inline Bootstrap boostrap;

#endif  // ifndef SSLOG_NO_BOOTSTRAP

}  // namespace priv

#endif  // ifndef SSLOG_DISABLE

}  // namespace sslog

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
