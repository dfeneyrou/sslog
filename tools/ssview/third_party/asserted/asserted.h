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

// 'asserted' is a library making a trade-off between weight and features.
// It rather places itself on the light side while bringing much needed features and ease C++ development.
//
// Features:
// - Context display via variable content
// - Stacktrace dump (requires libdw and libunwind on Linux)
// - Signals override to catch crashes
// - Compile-time enabling of group of assertions
// - No "unused variable" when assertions are disabled
// - Single header
// - Linux & Windows support

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)  // Disable Windows's secure API warnings
#endif

//-----------------------------------------------------------------------------
// Library configuration
//-----------------------------------------------------------------------------

// The 3 main switches are:
// - ASSERTED_NO_ASSERT  set to 1 to disable the assertions at compile time. They are enabled by default.
// - ASSERTED_NO_SIGNALS set to 1 to disable the signal catching. Catching is enabled by default.
// - ASSERTED_WITH_STACKTRACE set to 1 to enable the stacktrace dump. Disabled by default, due to dependencies

// Disable assertions
#ifndef ASSERTED_NO_ASSERT
#define ASSERTED_NO_ASSERT 0
#endif

// Disable installing signal handlers (ABRT, FPE, ILL, SEGV, TERM and INT (if ASSERTED_NO_SIGINT is 0) )
#ifndef ASSERTED_NO_SIGNALS
#define ASSERTED_NO_SIGNALS 0
#endif

// Disable catching the SIGINT signal (N/A if ASSERTED_NO_SIGNALS==1). Signal is enabled by default
#ifndef ASSERTED_NO_SIGINT
#define ASSERTED_NO_SIGINT 0
#endif

// Display messages using terminal colors. Enabled by default.
#if ASSERTED_NO_COLOR == 1
#define ASSERTED_COLOR_RED      ""
#define ASSERTED_COLOR_BLUE     ""
#define ASSERTED_COLOR_NUMBER   ""
#define ASSERTED_COLOR_FUNCTION ""
#define ASSERTED_COLOR_RESET    ""
#else
#define ASSERTED_COLOR_RED      "\033[31m"
#define ASSERTED_COLOR_BLUE     "\033[34m"
#define ASSERTED_COLOR_NUMBER   "\033[93m"
#define ASSERTED_COLOR_FUNCTION "\033[36m"
#define ASSERTED_COLOR_RESET    "\033[0m"
#endif

// Stacktrace logging when a crash occurs
//   - On linux, stack trace logging is disabled by default as it requires libunwind.so (stack unwinding) and libdw.so
//    from elfutils (elf and DWARF infos reading). Install with: "apt install libunwind-dev libdw-dev"
//   - On Windows, stacktrace logging is enabled by default as base system libraries cover the requirements.
//   Note: The executable shall contain debug information, else the stacktrace will just be a list of addresses.
//         If the non-stripped executable is available, addresses can be manually decoded with 'addr2line' or equivalent.
#if defined(_MSC_VER) && !defined(ASSERTED_WITH_STACKTRACE)
#define ASSERTED_WITH_STACKTRACE 1
#endif
#ifndef ASSERTED_WITH_STACKTRACE
#define ASSERTED_WITH_STACKTRACE 0
#endif

// Exit function when a crash occurs, called after displaying the crash information. Default is a call to quick_exit().
#ifndef ASSERTED_CRASH_EXIT_FUNC
#define ASSERTED_CRASH_EXIT_FUNC() quick_exit(1)
#endif

// Error display function. On Windows, it shall probably be redirected on a MessageBox
#ifndef ASSERTED_MESSAGE
#define ASSERTED_MESSAGE(msg, isLastFromCrash) fprintf(stderr, "%s", msg)
#endif

// Library version
#define ASSERTED_VERSION     "0.1.0"
#define ASSERTED_VERSION_NUM 100  // Monotonic number. 100 per version component. Official releases are multiple of 100

#define ASSERTED_IS_ENABLED     (ASSERTED_NO_SIGNALS == 0 || ASSERTED_NO_ASSERT == 0)
#define ASSERTED_CRASH_MSG_SIZE 2048

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#if ASSERTED_IS_ENABLED

// Windows base header (hard to avoid this include...)
#if defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers. If it is a problem, just comment it
#include <windows.h>

#include <cstdint>
#endif

#include <cstdio>   // For snprintf etc...
#include <cstdlib>  // For abort(), quick_exit...
#include <cstring>  // For memset

#if ASSERTED_NO_ASSERT == 0
#include <string>  // For the std::string display handler
#endif

#if ASSERTED_NO_SIGNALS == 0
#include <csignal>  // For raising signals in crash handler
#endif

#if ASSERTED_WITH_STACKTRACE == 1

#if defined(__unix__)
#define UNW_LOCAL_ONLY
#include <cxxabi.h>            // For demangling names
#include <elfutils/libdwfl.h>  // Elf reading (need package libdw-dev)
#include <libunwind.h>         // Stack unwinding (need package libunwind-dev)
#include <unistd.h>            // For getpid()
#endif                         // if defined(__unix__)

#if defined(_MSC_VER)
#include <dbghelp.h>         // For the symbol decoding
#include <errhandlingapi.h>  // For the HW exceptions
#pragma comment(lib, "DbgHelp.lib")
#endif  // if defined(_MSC_VER)

#endif  // if ASSERTED_WITH_STACKTRACE==1

#endif  // if ASSERTED_IS_ENABLED

// This line below is unfortunately the only way found to remove the zero-arguments-variadic-macro and
// the prohibited-anonymous-structs warnings with GCC when the build is using the option -Wpedantic
#ifndef _MSC_VER
// #pragma GCC system_header
#endif

//-----------------------------------------------------------------------------
// Public assertion API. Macro-based due to compile-time removal cosntraint
//-----------------------------------------------------------------------------

// Macros to handle disabled assertions. Up to 10 parameters
#define ASSERTED_PRIV_UNUSED0()
#define ASSERTED_PRIV_UNUSED1(a)                             (void)(a)
#define ASSERTED_PRIV_UNUSED2(a, b)                          (void)(a), ASSERTED_PRIV_UNUSED1(b)
#define ASSERTED_PRIV_UNUSED3(a, b, c)                       (void)(a), ASSERTED_PRIV_UNUSED2(b, c)
#define ASSERTED_PRIV_UNUSED4(a, b, c, d)                    (void)(a), ASSERTED_PRIV_UNUSED3(b, c, d)
#define ASSERTED_PRIV_UNUSED5(a, b, c, d, e)                 (void)(a), ASSERTED_PRIV_UNUSED4(b, c, d, e)
#define ASSERTED_PRIV_UNUSED6(a, b, c, d, e, f)              (void)(a), ASSERTED_PRIV_UNUSED5(b, c, d, e, f)
#define ASSERTED_PRIV_UNUSED7(a, b, c, d, e, f, g)           (void)(a), ASSERTED_PRIV_UNUSED6(b, c, d, e, f, g)
#define ASSERTED_PRIV_UNUSED8(a, b, c, d, e, f, g, h)        (void)(a), ASSERTED_PRIV_UNUSED7(b, c, d, e, f, g, h)
#define ASSERTED_PRIV_UNUSED9(a, b, c, d, e, f, g, h, i)     (void)(a), ASSERTED_PRIV_UNUSED8(b, c, d, e, f, g, h, i)
#define ASSERTED_PRIV_UNUSED10(a, b, c, d, e, f, g, h, i, j) (void)(a), ASSERTED_PRIV_UNUSED9(b, c, d, e, f, g, h, i, j)

#define ASSERTED_PRIV_VA_NUM_ARGS_IMPL(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N
#define ASSERTED_PRIV_VA_NUM_ARGS(...)                                                      ASSERTED_PRIV_VA_NUM_ARGS_IMPL(100, ##__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define ASSERTED_PRIV_ALL_UNUSED_IMPL_(nargs)                                               ASSERTED_PRIV_UNUSED##nargs
#define ASSERTED_PRIV_ALL_UNUSED_IMPL(nargs)                                                ASSERTED_PRIV_ALL_UNUSED_IMPL_(nargs)
#define ASSERTED_PRIV_ALL_UNUSED(...)                                                       ASSERTED_PRIV_ALL_UNUSED_IMPL(ASSERTED_PRIV_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)

// These assertions allow to easily dump the values which contribute to the condition.
// Ex: asserted(a<b);                                         // Standard form
//     asserted(a<b, "A shall always be less than b");        // Documented form
//     asserted(a<b, a, b);                                   // Extended form showing the values of 'a' and 'b' when assertion is failed
//     asserted(a<b, "A shall always be less than b", a, b);  // Displays up to 9 parameters... Ought to be enough for anybody (tm)
#if ASSERTED_NO_ASSERT == 0

// Macro to stringify the additional parameters of the enhanced assertions. Up to 10 parameters
// Note: in C99 and C++11, zero parameter is a problem, hence the forced & dummy first parameter "".
#define ASSERTED_PRIV_ASSERT_PARAM0()                           nullptr, 0
#define ASSERTED_PRIV_ASSERT_PARAM1(v1)                         nullptr, 0
#define ASSERTED_PRIV_ASSERT_PARAM2(v1, v2)                     nullptr, 0, #v2, v2
#define ASSERTED_PRIV_ASSERT_PARAM3(v1, v2, v3)                 nullptr, 0, #v2, v2, #v3, v3
#define ASSERTED_PRIV_ASSERT_PARAM4(v1, v2, v3, v4)             nullptr, 0, #v2, v2, #v3, v3, #v4, v4
#define ASSERTED_PRIV_ASSERT_PARAM5(v1, v2, v3, v4, v5)         nullptr, 0, #v2, v2, #v3, v3, #v4, v4, #v5, v5
#define ASSERTED_PRIV_ASSERT_PARAM6(v1, v2, v3, v4, v5, v6)     nullptr, 0, #v2, v2, #v3, v3, #v4, v4, #v5, v5, #v6, v6
#define ASSERTED_PRIV_ASSERT_PARAM7(v1, v2, v3, v4, v5, v6, v7) nullptr, 0, #v2, v2, #v3, v3, #v4, v4, #v5, v5, #v6, v6, #v7, v7
#define ASSERTED_PRIV_ASSERT_PARAM8(v1, v2, v3, v4, v5, v6, v7, v8) \
    nullptr, 0, #v2, v2, #v3, v3, #v4, v4, #v5, v5, #v6, v6, #v7, v7, #v8, v8
#define ASSERTED_PRIV_ASSERT_PARAM9(v1, v2, v3, v4, v5, v6, v7, v8, v9) \
    nullptr, 0, #v2, v2, #v3, v3, #v4, v4, #v5, v5, #v6, v6, #v7, v7, #v8, v8, #v9, v9
#define ASSERTED_PRIV_ASSERT_PARAM10(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10) \
    nullptr, 0, #v2, v2, #v3, v3, #v4, v4, #v5, v5, #v6, v6, #v7, v7, #v8, v8, #v9, v9, #v10, v10

#define asserted(cond_, ...)                                                  \
    if (ASSERTED_UNLIKELY(!(cond_)))                                          \
    assertedPriv::failedAssert(__FILE__, __LINE__, ASSERTED_FUNCTION, #cond_, \
                               ASSERTED_PRIV_CALL_OVERLOAD(ASSERTED_PRIV_ASSERT_PARAM, "", ##__VA_ARGS__))
#define group_asserted(group_, cond_, ...) \
    ASSERTED_PRIV_IF(ASSERTED_IS_COMPILE_TIME_ENABLED_(group_), asserted(cond_, ##__VA_ARGS__), do {} while (0))

#ifdef NDEBUG
#define debug_asserted(...) ASSERTED_PRIV_ALL_UNUSED(__VA_ARGS__)
#else
#define debug_asserted(...) asserted(__VA_ARGS__)
#endif

#else  // if ASSERTED_NO_ASSERT == 0

#define asserted(...)              ASSERTED_PRIV_ALL_UNUSED(__VA_ARGS__)
#define group_asserted(group, ...) ASSERTED_PRIV_ALL_UNUSED(__VA_ARGS__)
#define debug_asserted(...)        ASSERTED_PRIV_ALL_UNUSED(__VA_ARGS__)

#endif  // if ASSERTED_NO_ASSERT==0

//-----------------------------------------------------------------------------
// Macro helpers
//-----------------------------------------------------------------------------

// Optimization of the branching
#if defined(__GNUC__) || defined(__clang__)
#define ASSERTED_LIKELY(x)   (__builtin_expect(!!(x), 1))
#define ASSERTED_UNLIKELY(x) (__builtin_expect(!!(x), 0))
#define ASSERTED_NOINLINE    __attribute__((noinline))
#define ASSERTED_NORETURN    __attribute__((__noreturn__))
#else
#define ASSERTED_LIKELY(x)   (x)
#define ASSERTED_UNLIKELY(x) (x)
#define ASSERTED_NOINLINE
#define ASSERTED_NORETURN
#endif

// Best possible function name for assertions
#if defined(__GNUC__) || defined(__clang__)
#define ASSERTED_FUNCTION __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define ASSERTED_FUNCTION __FUNCSIG__
#else
#define ASSERTED_FUNCTION __func__
#endif

// Conditional inclusion macro trick
#define ASSERTED_PRIV_IF(cond, foo1, foo2)       ASSERTED_PRIV_IF_IMPL(cond, foo1, foo2)
#define ASSERTED_PRIV_IF_IMPL(cond, foo1, foo2)  ASSERTED_PRIV_IF_IMPL2(cond, foo1, foo2)
#define ASSERTED_PRIV_IF_IMPL2(cond, foo1, foo2) ASSERTED_PRIV_IF_##cond(foo1, foo2)
#define ASSERTED_PRIV_IF_0(foo1, foo2)           foo2
#define ASSERTED_PRIV_IF_1(foo1, foo2)           foo1

// Variadic macro trick (from 0 up to 10 arguments)
#define ASSERTED_PRIV_EXPAND(x)                                         x
#define ASSERTED_PRIV_PREFIX(...)                                       0, ##__VA_ARGS__
#define ASSERTED_PRIV_LASTOF12(a, b, c, d, e, f, g, h, i, j, k, l, ...) l
#define ASSERTED_PRIV_SUB_NBARG(...)                                    ASSERTED_PRIV_EXPAND(ASSERTED_PRIV_LASTOF12(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#define ASSERTED_PRIV_NBARG(...)                                        ASSERTED_PRIV_SUB_NBARG(ASSERTED_PRIV_PREFIX(__VA_ARGS__))
#define ASSERTED_PRIV_GLUE(x, y)                                        x y
#define ASSERTED_PRIV_OVERLOAD_MACRO2(name, count)                      name##count
#define ASSERTED_PRIV_OVERLOAD_MACRO1(name, count)                      ASSERTED_PRIV_OVERLOAD_MACRO2(name, count)
#define ASSERTED_PRIV_OVERLOAD_MACRO(name, count)                       ASSERTED_PRIV_OVERLOAD_MACRO1(name, count)
#define ASSERTED_PRIV_CALL_OVERLOAD(name, ...) \
    ASSERTED_PRIV_GLUE(ASSERTED_PRIV_OVERLOAD_MACRO(name, ASSERTED_PRIV_NBARG(__VA_ARGS__)), (__VA_ARGS__))

#define ASSERTED_IS_COMPILE_TIME_ENABLED_(group_) ASSERTED_GROUP_##group_

//-----------------------------------------------------------------------------
// Private implementation
//-----------------------------------------------------------------------------

#if ASSERTED_IS_ENABLED

namespace assertedPriv
{

// Break inside this function in debugger
void ASSERTED_NORETURN
assertedCrash(const char* message);

#if ASSERTED_NO_ASSERT == 0

template<typename T>
inline void
printParamType_(char* infoStr, int& offset, const char* name, T param)
{
    offset +=
        snprintf(infoStr + offset, (size_t)(ASSERTED_CRASH_MSG_SIZE - offset), "    %s is not a numeric or string type (enum?)\n", name);
    if (offset > ASSERTED_CRASH_MSG_SIZE - 1) offset = ASSERTED_CRASH_MSG_SIZE - 1;
    (void)(param);
}
template<typename T>
inline void
printParamType_(char* infoStr, int& offset, const char* name, T* param)
{
    offset += snprintf(infoStr + offset, (size_t)(ASSERTED_CRASH_MSG_SIZE - offset),
                       "    %-7s %-20s => " ASSERTED_COLOR_NUMBER "%p\n" ASSERTED_COLOR_RESET, "pointer", name, (void*)param);
    if (offset > ASSERTED_CRASH_MSG_SIZE - 1) offset = ASSERTED_CRASH_MSG_SIZE - 1;
}
template<>
inline void
printParamType_<std::string>(char* infoStr, int& offset, const char* name, std::string param)
{
    offset += snprintf(infoStr + offset, (size_t)(ASSERTED_CRASH_MSG_SIZE - offset),
                       "    %-7s %-20s => " ASSERTED_COLOR_NUMBER "%s\n" ASSERTED_COLOR_RESET, "string", name, param.c_str());
    if (offset > ASSERTED_CRASH_MSG_SIZE - 1) offset = ASSERTED_CRASH_MSG_SIZE - 1;
}
template<>
inline void
printParamType_<bool>(char* infoStr, int& offset, const char* name, bool param)
{
    offset += snprintf(infoStr + offset, (size_t)(ASSERTED_CRASH_MSG_SIZE - offset),
                       "    %-7s %-20s => " ASSERTED_COLOR_NUMBER "%s\n" ASSERTED_COLOR_RESET, "bool", name, param ? "true" : "false");
    if (offset > ASSERTED_CRASH_MSG_SIZE - 1) offset = ASSERTED_CRASH_MSG_SIZE - 1;
}
template<>
inline void
printParamType_<char>(char* infoStr, int& offset, const char* name, char* param)
{
    offset += snprintf(infoStr + offset, (size_t)(ASSERTED_CRASH_MSG_SIZE - offset),
                       "    %-28s => " ASSERTED_COLOR_NUMBER "%s\n" ASSERTED_COLOR_RESET, "message", param);
    if (offset > ASSERTED_CRASH_MSG_SIZE - 1) offset = ASSERTED_CRASH_MSG_SIZE - 1;
    (void)(name);
}
template<>
inline void
printParamType_<const char>(char* infoStr, int& offset, const char* name, const char* param)
{
    offset += snprintf(infoStr + offset, (size_t)(ASSERTED_CRASH_MSG_SIZE - offset),
                       "    %-28s => " ASSERTED_COLOR_NUMBER "%s\n" ASSERTED_COLOR_RESET, "message", param);
    if (offset > ASSERTED_CRASH_MSG_SIZE - 1) offset = ASSERTED_CRASH_MSG_SIZE - 1;
    (void)(name);
}
#define ASSERTED_DECLARE_ASSERT_TYPE(type_, code_, display_)                                                                         \
    template<>                                                                                                                       \
    inline void printParamType_<type_>(char* infoStr, int& offset, const char* name, type_ param)                                    \
    {                                                                                                                                \
        offset += snprintf(infoStr + offset, (size_t)(ASSERTED_CRASH_MSG_SIZE - offset),                                             \
                           "    %-7s %-20s => " ASSERTED_COLOR_NUMBER "%" #code_ "\n" ASSERTED_COLOR_RESET, #display_, name, param); \
        if (offset > ASSERTED_CRASH_MSG_SIZE - 1) offset = ASSERTED_CRASH_MSG_SIZE - 1;                                              \
    }

ASSERTED_DECLARE_ASSERT_TYPE(char, d, s8)
ASSERTED_DECLARE_ASSERT_TYPE(unsigned char, u, u8)
ASSERTED_DECLARE_ASSERT_TYPE(short int, d, s16)
ASSERTED_DECLARE_ASSERT_TYPE(unsigned short, u, u16)
ASSERTED_DECLARE_ASSERT_TYPE(int, d, int)
ASSERTED_DECLARE_ASSERT_TYPE(unsigned int, u, u32)
ASSERTED_DECLARE_ASSERT_TYPE(long, ld, s64)
ASSERTED_DECLARE_ASSERT_TYPE(unsigned long, lu, u64)
ASSERTED_DECLARE_ASSERT_TYPE(long long, lld, s64)
ASSERTED_DECLARE_ASSERT_TYPE(unsigned long long, llu, u64)
ASSERTED_DECLARE_ASSERT_TYPE(float, f, float)
ASSERTED_DECLARE_ASSERT_TYPE(double, lf, double)

template<typename T>
inline void
printParams_(bool isFirst, char* infoStr, int& offset, const char* name, T param)
{
    if (isFirst) {
        offset +=
            snprintf(infoStr + offset, (size_t)(ASSERTED_CRASH_MSG_SIZE - offset), ASSERTED_COLOR_BLUE "  Context:\n" ASSERTED_COLOR_RESET);
        if (offset > ASSERTED_CRASH_MSG_SIZE - 1) offset = ASSERTED_CRASH_MSG_SIZE - 1;
    }
    if (name) { printParamType_(infoStr, offset, name, param); }
}
template<typename T, typename... Args>
inline void
printParams_(bool isFirst, char* infoStr, int& offset, const char* name, T value, Args... args)
{
    if (isFirst) {
        offset +=
            snprintf(infoStr + offset, (size_t)(ASSERTED_CRASH_MSG_SIZE - offset), ASSERTED_COLOR_BLUE "  Context:\n" ASSERTED_COLOR_RESET);
        if (offset > ASSERTED_CRASH_MSG_SIZE - 1) offset = ASSERTED_CRASH_MSG_SIZE - 1;
    }
    if (name) { printParamType_(infoStr, offset, name, value); }
    printParams_(false, infoStr, offset, args...);
}

// Variadic template based assertion display
template<typename... Args>
void ASSERTED_NOINLINE ASSERTED_NORETURN
failedAssert(const char* filename, int lineNbr, const char* function, const char* condition, Args... args)
{
    char infoStr[ASSERTED_CRASH_MSG_SIZE];
    int  offset =
        snprintf(infoStr, sizeof(infoStr),
                 ASSERTED_COLOR_RED "[ASSERTED] Assertion failed: " ASSERTED_COLOR_NUMBER "%s\n" ASSERTED_COLOR_BLUE
                                    "  Where:\n" ASSERTED_COLOR_RESET "    Function => " ASSERTED_COLOR_FUNCTION "%s\n" ASSERTED_COLOR_RESET
                                    "    File     => " ASSERTED_COLOR_FUNCTION "%s:" ASSERTED_COLOR_NUMBER "%d\n" ASSERTED_COLOR_RESET,
                 condition, function, filename, lineNbr);
    if (offset > ASSERTED_CRASH_MSG_SIZE - 1) offset = ASSERTED_CRASH_MSG_SIZE - 1;
    printParams_(true, infoStr, offset, args...);  // Recursive display of provided items
    assertedCrash(infoStr);
}

#endif  // if ASSERTED_NO_ASSERT==0

#if defined(_MSC_VER) && ASSERTED_WITH_STACKTRACE == 1
extern "C" {
typedef unsigned long(__stdcall* rtlWalkFrameChain_t)(void**, unsigned long, unsigned long);
}
#endif

typedef void (*assertedSignalHandler_t)(int);  // @REMOVE?

inline struct GlobalContext {
    bool                    signalHandlersSaved   = false;
    assertedSignalHandler_t signalsOldHandlers[7] = {0};
#if defined(_MSC_VER)
    PVOID exceptionHandler = 0;
#if ASSERTED_WITH_STACKTRACE == 1
    rtlWalkFrameChain_t rtlWalkFrameChain = 0;
#endif
#endif  // if defined(_MSC_VER)
} gc;

#if ASSERTED_WITH_STACKTRACE == 1

#if defined(__unix__)
// Linux version
inline void
crashLogStackTrace(void)
{
    // Initialize libunwind
    unw_context_t uc;
    unw_getcontext(&uc);
    unw_cursor_t cursor;
    unw_init_local(&cursor, &uc);
    unw_word_t offset;
    unw_word_t ip;

    // Initialize DWARF reading
    char*          debugInfoPath = NULL;
    Dwfl_Callbacks callbacks     = {};
    callbacks.find_elf           = dwfl_linux_proc_find_elf;
    callbacks.find_debuginfo     = dwfl_standard_find_debuginfo;
    callbacks.debuginfo_path     = &debugInfoPath;
    Dwfl* dwfl                   = dwfl_begin(&callbacks);
    if (!dwfl || dwfl_linux_proc_report(dwfl, getpid()) != 0 || dwfl_report_end(dwfl, NULL, NULL) != 0) { return; }

    constexpr int MaxStackLines = 48;
    struct StackLine {
        char filenameAndLineNbr[128];
        char functionName[256];
        int  firstPartSize;
    };
    StackLine lines[MaxStackLines];
    int       lineQty          = 0;
    int       maxFirstPartSize = 0;
    char      tmpStr[128];

    const int skipDepthQty = 2;  // No need to display the bottom machinery
    int       depth        = 0;
    // Loop on stack depth
    while (unw_step(&cursor) > 0) {
        unw_get_reg(&cursor, UNW_REG_IP, &ip);

        if (depth >= skipDepthQty && lineQty < MaxStackLines) {
            Dwarf_Addr   addr   = (uintptr_t)(ip - 4);
            Dwfl_Module* module = dwfl_addrmodule(dwfl, addr);
            Dwfl_Line*   line   = dwfl_getsrc(dwfl, addr);
            StackLine&   l      = lines[lineQty++];

            if (line) {
                Dwarf_Addr  addr2;
                int         lineNbr;
                int         status;
                const char* filename      = dwfl_lineinfo(line, &addr2, &lineNbr, NULL, NULL, NULL);
                char*       demangledName = abi::__cxa_demangle(dwfl_module_addrname(module, addr), 0, 0, &status);
                snprintf(l.filenameAndLineNbr, sizeof(l.filenameAndLineNbr),
                         "    #" ASSERTED_COLOR_NUMBER "%-2d " ASSERTED_COLOR_RESET "%s:" ASSERTED_COLOR_NUMBER "%d ", depth - skipDepthQty,
                         filename ? strrchr(filename, '/') + 1 : "<unknown>", filename ? lineNbr : 0);
                snprintf(l.functionName, sizeof(l.functionName), ASSERTED_COLOR_FUNCTION "%s\n" ASSERTED_COLOR_RESET,
                         status ? dwfl_module_addrname(module, addr) : demangledName);
                if (status == 0) free(demangledName);
            } else {
                snprintf(l.filenameAndLineNbr, sizeof(l.filenameAndLineNbr),
                         "    #" ASSERTED_COLOR_NUMBER "%-2d " ASSERTED_COLOR_RESET "0x%" PRIX64 ASSERTED_COLOR_NUMBER,
                         depth - skipDepthQty, ip - 4);
                snprintf(l.functionName, sizeof(l.functionName), ASSERTED_COLOR_FUNCTION "%s\n" ASSERTED_COLOR_RESET,
                         dwfl_module_addrname(module, addr));
            }
            l.firstPartSize = strlen(l.filenameAndLineNbr);
            if (l.firstPartSize > maxFirstPartSize) maxFirstPartSize = l.firstPartSize;
        }

        // Next unwinding
        tmpStr[0] = 0;
        unw_get_proc_name(&cursor, tmpStr, sizeof(tmpStr), &offset);  // Fails if there is no debug symbols
        if (!strcmp(tmpStr, "main")) break;
        ++depth;
    }  // End of unwinding

    // Display the stack trace. Thanks to this 2-pass, the functions are aligned.
    for (int lineNbr = 0; lineNbr < lineQty; ++lineNbr) {
        StackLine& l = lines[lineNbr];
        ASSERTED_MESSAGE(l.filenameAndLineNbr, false);
        snprintf(tmpStr, sizeof(tmpStr), "%*s", maxFirstPartSize - l.firstPartSize, "");
        ASSERTED_MESSAGE(tmpStr, false);
        ASSERTED_MESSAGE(l.functionName, false);
    }

    // End session
    ASSERTED_MESSAGE("\n", false);
    dwfl_end(dwfl);
}
#endif  // if defined(__unix__)

#if defined(_MSC_VER)
// Windows version
inline void
crashLogStackTrace(void)
{
    char tmpStr[128];
    char depthStr[8];

    // Get the addresses of the stacktrace
    constexpr int MaxStackLines = 64;  // 64 levels of depth should be enough for everyone
    struct StackLine {
        char filenameAndLineNbr[128];
        char functionName[256];
        int  firstPartSize;
    };
    StackLine lines[MaxStackLines];
    int       lineQty          = 0;
    int       maxFirstPartSize = 0;
    PVOID     stacktrace[MaxStackLines];
    int       foundStackDepth = gc.rtlWalkFrameChain ? gc.rtlWalkFrameChain(stacktrace, 64, 0) : 0;

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

#define ASSERTED_CRASH_STACKTRACE_DUMP_INFO_(itemNbrStr)                                                                          \
    if (isFuncValid || isLineValid) {                                                                                             \
        snprintf(tmpStr, sizeof(tmpStr), ":%u", isLineValid ? line.LineNumber : 0);                                               \
        snprintf(l.filenameAndLineNbr, sizeof(l.filenameAndLineNbr),                                                              \
                 "     " ASSERTED_COLOR_NUMBER "%s " ASSERTED_COLOR_RESET "%s" ASSERTED_COLOR_NUMBER "%s ", itemNbrStr,           \
                 isLineValid ? strrchr(line.FileName, '\\') + 1 : "<unknown>", isLineValid ? tmpStr : "");                        \
        snprintf(l.functionName, sizeof(l.functionName), ASSERTED_COLOR_FUNCTION "%s" ASSERTED_COLOR_RESET "\n",                  \
                 isFuncValid ? symInfo->Name : "<unknown>");                                                                      \
    } else {                                                                                                                      \
        snprintf(l.filenameAndLineNbr, sizeof(l.filenameAndLineNbr),                                                              \
                 "     " ASSERTED_COLOR_NUMBER "%s" ASSERTED_COLOR_FUNCTION " 0x%" PRIX64 ASSERTED_COLOR_RESET, itemNbrStr, ptr); \
        l.functionName[0] = '\n';                                                                                                 \
        l.functionName[1] = 0;                                                                                                    \
    }                                                                                                                             \
    l.firstPartSize = strlen(l.filenameAndLineNbr);                                                                               \
    if (l.firstPartSize > maxFirstPartSize) maxFirstPartSize = l.firstPartSize;

    constexpr int skipDepthQty = 3;  // No need to display the bottom machinery
    for (int depth = skipDepthQty; depth < foundStackDepth; ++depth) {
        // -1 because the captured PC is already pointing on the next code line at snapshot time
        uint64_t ptr = ((uint64_t)stacktrace[depth]) - 1;

        // Get the nested inline function calls, if any
        DWORD frameIdx, curContext = 0;
        int   inlineQty = SymAddrIncludeInlineTrace(proc, ptr);
        if (inlineQty > 0 && SymQueryInlineTrace(proc, ptr, 0, ptr, ptr, &curContext, &frameIdx)) {
            for (int i = 0; i < inlineQty; ++i) {
                bool isFuncValid = (SymFromInlineContext(proc, ptr, curContext, 0, symInfo) != 0);
                bool isLineValid = (SymGetLineFromInlineContext(proc, ptr, curContext, 0, &displacement, &line) != 0);
                ++curContext;
                if (lineQty < MaxStackLines) {
                    StackLine& l = lines[lineQty++];
                    ASSERTED_CRASH_STACKTRACE_DUMP_INFO_("inl");
                }
            }
        }

        // Get the function call for this depth
        if (lineQty < MaxStackLines) {
            StackLine& l           = lines[lineQty++];
            bool       isFuncValid = (SymFromAddr(proc, ptr, 0, symInfo) != 0);
            bool       isLineValid = (SymGetLineFromAddr64(proc, ptr - 1, &displacement, &line) != 0);
            snprintf(depthStr, sizeof(depthStr), "#%-2d", depth - skipDepthQty);
            ASSERTED_CRASH_STACKTRACE_DUMP_INFO_(depthStr);
        }
    }  // End of loop on stack depth

    // Display the stack trace. Thanks to this 2-pass, the functions are aligned.
    for (int lineNbr = 0; lineNbr < lineQty; ++lineNbr) {
        StackLine& l = lines[lineNbr];
        ASSERTED_MESSAGE(l.filenameAndLineNbr, false);
        snprintf(tmpStr, sizeof(tmpStr), "%*s", maxFirstPartSize - l.firstPartSize, "");
        ASSERTED_MESSAGE(tmpStr, false);
        ASSERTED_MESSAGE(l.functionName, false);
    }
}
#endif  // if defined(_MSC_VER)

#endif  // if ASSERTED_WITH_STACKTRACE == 1

inline void
assertedCrash(const char* message)
{
    // Log and display the crash message
    ASSERTED_MESSAGE(message, false);

    // Log and display the call stack
#if ASSERTED_WITH_STACKTRACE == 1
    ASSERTED_MESSAGE(ASSERTED_COLOR_BLUE "  Stacktrace:\n" ASSERTED_COLOR_RESET, false);  // Standard
    assertedPriv::crashLogStackTrace();
#endif
    ASSERTED_MESSAGE("\n", true);  // End of full crash display

    // Stop the process
    ASSERTED_CRASH_EXIT_FUNC();
}

#if ASSERTED_NO_SIGNALS == 0

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
    snprintf(infoStr, sizeof(infoStr), ASSERTED_COLOR_RED "[ASSERTED] signal %d '%s' received\n" ASSERTED_COLOR_RESET, signalId, sigDescr);
    assertedCrash(infoStr);
}

#if _MSC_VER
// Specific to windows, on top of the signal handler
static LONG WINAPI
exceptionHandler(struct _EXCEPTION_POINTERS* pExcept)
{
    char         infoStr[256];
    int          tmp;
    unsigned int code = pExcept->ExceptionRecord->ExceptionCode;
#define ASSERTED_LOG_EXCEPTION_(str)                                                                                          \
    snprintf(infoStr, sizeof(infoStr), ASSERTED_COLOR_RED "[ASSERTED] exception '%s' received.\n" ASSERTED_COLOR_RESET, str); \
    assertedCrash(infoStr)

    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:
            tmp = (int)pExcept->ExceptionRecord->ExceptionInformation[0];
            snprintf(infoStr, sizeof(infoStr),
                     ASSERTED_COLOR_RED "[ASSERTED] exception 'ACCESS_VIOLATION' (%s) received." ASSERTED_COLOR_RESET,
                     (tmp == 0) ? "read" : ((tmp == 1) ? "write" : "user-mode data execution prevention (DEP) violation"));
            assertedCrash(infoStr);
            break;
        case EXCEPTION_BREAKPOINT:
            break;  // Let this one go through the handler
        case EXCEPTION_SINGLE_STEP:
            break;  // Let this one go through the handler
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            ASSERTED_LOG_EXCEPTION_("ARRAY_BOUNDS_EXCEEDED");
            break;
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            ASSERTED_LOG_EXCEPTION_("DATATYPE_MISALIGNMENT");
            break;
        case EXCEPTION_FLT_DENORMAL_OPERAND:
            ASSERTED_LOG_EXCEPTION_("FLT_DENORMAL_OPERAND");
            break;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            ASSERTED_LOG_EXCEPTION_("FLT_DIVIDE_BY_ZERO");
            break;
        case EXCEPTION_FLT_INEXACT_RESULT:
            ASSERTED_LOG_EXCEPTION_("FLT_INEXACT_RESULT");
            break;
        case EXCEPTION_FLT_INVALID_OPERATION:
            ASSERTED_LOG_EXCEPTION_("FLT_INVALID_OPERATION");
            break;
        case EXCEPTION_FLT_OVERFLOW:
            ASSERTED_LOG_EXCEPTION_("FLT_OVERFLOW");
            break;
        case EXCEPTION_FLT_STACK_CHECK:
            ASSERTED_LOG_EXCEPTION_("FLT_STACK_CHECK");
            break;
        case EXCEPTION_FLT_UNDERFLOW:
            ASSERTED_LOG_EXCEPTION_("FLT_UNDERFLOW");
            break;
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            ASSERTED_LOG_EXCEPTION_("ILLEGAL_INSTRUCTION");
            break;
        case EXCEPTION_IN_PAGE_ERROR:
            ASSERTED_LOG_EXCEPTION_("IN_PAGE_ERROR");
            break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            ASSERTED_LOG_EXCEPTION_("INT_DIVIDE_BY_ZERO");
            break;
        case EXCEPTION_INT_OVERFLOW:
            ASSERTED_LOG_EXCEPTION_("INT_OVERFLOW");
            break;
        case EXCEPTION_INVALID_DISPOSITION:
            ASSERTED_LOG_EXCEPTION_("INVALID_DISPOSITION");
            break;
        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
            ASSERTED_LOG_EXCEPTION_("NONCONTINUABLE_EXCEPTION");
            break;
        case EXCEPTION_PRIV_INSTRUCTION:
            ASSERTED_LOG_EXCEPTION_("PRIV_INSTRUCTION");
            break;
        case EXCEPTION_STACK_OVERFLOW:
            ASSERTED_LOG_EXCEPTION_("STACK_OVERFLOW");
            break;
        default:
            ASSERTED_LOG_EXCEPTION_("UNKNOWN");
            break;
    }
    // Go to the next handler
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif  // if _MSC_VER

#endif  // if ASSERTED_NO_SIGNALS == 0

// =======================================================================================================
// Automatic instantiation
// =======================================================================================================

struct Bootstrap {
    Bootstrap()
    {
        auto& ic = assertedPriv::gc;

        // Register POSIX signals
        memset(ic.signalsOldHandlers, 0, sizeof(ic.signalsOldHandlers));
#if ASSERTED_NO_SIGNALS == 0
        ic.signalsOldHandlers[0] = std::signal(SIGABRT, assertedPriv::signalHandler);
        ic.signalsOldHandlers[1] = std::signal(SIGFPE, assertedPriv::signalHandler);
        ic.signalsOldHandlers[2] = std::signal(SIGILL, assertedPriv::signalHandler);
        ic.signalsOldHandlers[3] = std::signal(SIGSEGV, assertedPriv::signalHandler);
#if ASSERTED_NO_SIGINT == 0
        ic.signalsOldHandlers[4] = std::signal(SIGINT, assertedPriv::signalHandler);
#endif
        ic.signalsOldHandlers[5] = std::signal(SIGTERM, assertedPriv::signalHandler);
#if defined(__unix__)
        ic.signalsOldHandlers[6] = std::signal(SIGPIPE, assertedPriv::signalHandler);
#endif
        ic.signalHandlersSaved = true;
#if defined(_MSC_VER)
        // Register the exception handler
        ic.exceptionHandler = AddVectoredExceptionHandler(1, assertedPriv::exceptionHandler);
#endif  // if defined(_MSC_VER)
#endif  // if ASSERTED_NO_SIGNALS==0

#if defined(_MSC_VER) && ASSERTED_WITH_STACKTRACE == 1
        // Initialize the symbol reading for the stacktrace (in case of crash)
        SymInitialize(GetCurrentProcess(), 0, true);
        SymSetOptions(SYMOPT_LOAD_LINES);
        ic.rtlWalkFrameChain = (assertedPriv::rtlWalkFrameChain_t)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlWalkFrameChain");
        asserted(ic.rtlWalkFrameChain);
#endif  // if defined(_MSC_VER) && ASSERTED_WITH_STACKTRACE==1
    }

    ~Bootstrap()
    {
        // Unregister signals
#if ASSERTED_NO_SIGNALS == 0
        auto& ic = assertedPriv::gc;
        if (ic.signalHandlersSaved) {
            std::signal(SIGABRT, ic.signalsOldHandlers[0]);
            std::signal(SIGFPE, ic.signalsOldHandlers[1]);
            std::signal(SIGILL, ic.signalsOldHandlers[2]);
            std::signal(SIGSEGV, ic.signalsOldHandlers[3]);
#if ASSERTED_NO_SIGINT == 0
            std::signal(SIGINT, ic.signalsOldHandlers[4]);
#endif
            std::signal(SIGTERM, ic.signalsOldHandlers[5]);
#if defined(__unix__)
            std::signal(SIGPIPE, ic.signalsOldHandlers[6]);
#endif
#if defined(_MSC_VER)
            RemoveVectoredExceptionHandler(ic.exceptionHandler);
#endif  // if defined(_MSC_VER)
        }
#endif  // if ASSERTED_NO_SIGNALS==0
    }

    Bootstrap(Bootstrap const&)            = delete;
    Bootstrap(Bootstrap&&)                 = delete;
    Bootstrap& operator=(Bootstrap const&) = delete;
    Bootstrap& operator=(Bootstrap&&)      = delete;
};

inline Bootstrap boostrap;

}  // namespace assertedPriv

#endif  // if ASSERTED_IS_ENABLED

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
