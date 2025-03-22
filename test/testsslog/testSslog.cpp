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

// This file is a test program with multiple purposes:
//  - show an example of C++ instrumentation
//  - have a way to measure speed performance in a specific case
//  - be a part of sslog internal tests, by using all instrumentation APIs and features

#ifdef _WIN32
#include "windows.h"
#endif  // _WIN32

#include <cstring>
#include <mutex>
#include <thread>

#include "sslog.h"
#include "testPart.h"

// Instrumentation group to test the group API
#ifndef SSLOG_GROUP_TESTGROUP
#define SSLOG_GROUP_TESTGROUP 1
#endif

std::mutex globalSharedMx;

// ==============================
// Event collection program
// ==============================

void
allApisTask(int threadNbr, int loopQty)
{
    for (int loopNbr = 1; loopNbr <= loopQty; ++loopNbr) {
        sslog::Collector collectorConfig = ssGetCollector();
        ssSetCollector(collectorConfig);
        sslog::Sink sinkConfig = ssGetSink();
        ssSetSink(sinkConfig);
        ssSetStorageLevel(sslog::Level::trace);
        ssSetConsoleLevel(sslog::Level::trace);
        ssSetConsoleFormatter("[%L] [%Y-%m-%dT%H:%M:%S.%f%z] [%c] [thread %t] %v%Q");
        ssRequestForDetails();
        [[maybe_unused]] sslog::Stats s = ssGetStats();
        ssStop();
        ssStart();
        char threadName[64];
        snprintf(threadName, sizeof(threadName), "T%d", threadNbr);
        ssSetThreadName(threadName);

        if (ssIsEnabled(sslog::Level::info)) {
            ssTrace("category", "Text");
            ssDebug("category", "Text %d %f %s", loopNbr, 2. * loopNbr, "string");
            ssInfo("category", "Text %d %f %s", loopNbr, 2. * loopNbr, "string");
            ssWarn("category", "Text %d %f %s", loopNbr, 2. * loopNbr, "string");
            ssError("category", "Text %d %f %s", loopNbr, 2. * loopNbr, "string");
            ssCritical("category", "Text %d %f %s", loopNbr, 2. * loopNbr, "string");
            std::vector<uint8_t> buffer{1, 2, 3, 4, 5, 6, 7, 8};
            ssTraceBuffer("category", buffer.data(), buffer.size(), "Text Buffer");
            ssDebugBuffer("category", buffer.data(), buffer.size(), "Text Buffer %d %f %s", loopNbr, 2. * loopNbr, "string");
            ssInfoBuffer("category", buffer.data(), buffer.size(), "Text Buffer %d %f %s", loopNbr, 2. * loopNbr, "string");
            ssWarnBuffer("category", buffer.data(), buffer.size(), "Text Buffer %d %f %s", loopNbr, 2. * loopNbr, "string");
            ssErrorBuffer("category", buffer.data(), buffer.size(), "Text Buffer %d %f %s", loopNbr, 2. * loopNbr, "string");
            ssCriticalBuffer("category", buffer.data(), buffer.size(), "Text Buffer %d %f %s", loopNbr, 2. * loopNbr, "string");
        }

        if (ssgIsEnabled(TESTGROUP, sslog::Level::info)) {
            ssgTrace(TESTGROUP, "category", "Text");
            ssgDebug(TESTGROUP, "category", "Text %d %f %s", loopNbr, 2. * loopNbr, "string");
            ssgInfo(TESTGROUP, "category", "Text %d %f %s", loopNbr, 2. * loopNbr, "string");
            ssgWarn(TESTGROUP, "category", "Text %d %f %s", loopNbr, 2. * loopNbr, "string");
            ssgError(TESTGROUP, "category", "Text %d %f %s", loopNbr, 2. * loopNbr, "string");
            ssgCritical(TESTGROUP, "category", "Text %d %f %s", loopNbr, 2. * loopNbr, "string");
            std::vector<uint8_t> buffer{9, 10, 11, 12, 13, 14, 15, 16};
            ssgTraceBuffer(TESTGROUP, "category", buffer.data(), buffer.size(), "Text Buffer");
            ssgDebugBuffer(TESTGROUP, "category", buffer.data(), buffer.size(), "Text Buffer %d %f %s", loopNbr, 2. * loopNbr, "string");
            ssgInfoBuffer(TESTGROUP, "category", buffer.data(), buffer.size(), "Text Buffer %d %f %s", loopNbr, 2. * loopNbr, "string");
            ssgWarnBuffer(TESTGROUP, "category", buffer.data(), buffer.size(), "Text Buffer %d %f %s", loopNbr, 2. * loopNbr, "string");
            ssgErrorBuffer(TESTGROUP, "category", buffer.data(), buffer.size(), "Text Buffer %d %f %s", loopNbr, 2. * loopNbr, "string");
            ssgCriticalBuffer(TESTGROUP, "category", buffer.data(), buffer.size(), "Text Buffer %d %f %s", loopNbr, 2. * loopNbr, "string");
        }
    }
}

// =========================
// Main
// =========================

void
displayUsage(const char* programPath)
{
    printf("\nUsage: %s <parameter> [options]\n", programPath);
    printf("  'sslog' C++ logger test program\n");
    printf("\n");
    printf("  Parameter:\n");
    printf("    'api' : Use the entirety of the logger API (testing)\n");
    printf("    'mix' : Use a mix of different logs to create a more realistic logging session (compactness performance)\n");
    printf("    'perf': Use repeating logs in loop (speed performance)\n");
    printf("\n");
    printf("  Options to configure the program behavior:\n");
    printf("    '-t <1-9>      : Defines the quantity of threads\n");
    printf("    '-l <integer>' : Run time length multiplier (default is 1)\n");
}

int
main(int argc, char** argv)
{
    // Check the type of execution
    bool doDisplayUsage = (argc <= 1 || (strcmp(argv[1], "perf") != 0 && strcmp(argv[1], "api") != 0 && strcmp(argv[1], "mix") != 0));

    // Get the options
    int threadQty = 1;
    int loopQty   = 1;
    int argCount  = 2;
    while (!doDisplayUsage && argCount < argc) {
        const char* w = argv[argCount];
        if (strcmp(w, "-t") == 0 && argCount + 1 < argc) {
            threadQty = strtol(argv[++argCount], nullptr, 10);
        } else if (strcmp(w, "-l") == 0 && argCount + 1 < argc) {
            loopQty = strtol(argv[++argCount], nullptr, 10);
        } else {
            printf("Error: unknown argument '%s'\n", w);
            doDisplayUsage = true;
        }
        ++argCount;
    }
    if (threadQty <= 0 || loopQty <= 0) doDisplayUsage = true;

    // Some display
    if (doDisplayUsage) {
        displayUsage(argv[0]);
        return 1;
    }

    printf("Thread qty: %d\n", threadQty);
    printf("Loop quantity: %d\n", loopQty);
    ssSetStorageLevel(sslog::Level::trace);
    ssSetStoragePath("sslogDb");
    ssSetConsoleLevel(sslog::Level::off);
    sslog::Collector collector;
    collector.dataBufferBytes = 100000000;  // Large buffers to avoid waiting
    ssSetCollector(collector);

    std::vector<std::thread> threads;

    if (strcmp(argv[1], "api") == 0) {
        for (int threadNbr = 0; threadNbr < threadQty; ++threadNbr) { threads.push_back(std::thread(allApisTask, threadNbr, loopQty)); }
        for (std::thread& t : threads) t.join();
    }

    if (strcmp(argv[1], "mix") == 0) {  // To stimulate filters when reading
        for (int threadNbr = 0; threadNbr < threadQty; ++threadNbr) { threads.push_back(std::thread(mixOfLogTask, threadNbr, loopQty)); }
        for (std::thread& t : threads) t.join();
    }

    if (strcmp(argv[1], "perf") == 0) { testPerformance(threadQty, loopQty); }

    return 0;
}
