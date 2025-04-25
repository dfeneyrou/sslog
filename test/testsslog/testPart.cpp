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

// This file is part of the test program of 'sslog'

#include "testPart.h"

#include <math.h>

#include <thread>

#include "sslog.h"

void
performance0Task(int loopQty)
{
    for (int i = 0; i < loopQty; ++i) { ssInfo("Benchmark", "Simple log message without parameter"); }
}

void
performance1Task(int loopQty)
{
    for (int i = 0; i < loopQty; ++i) { ssInfo("Benchmark", "Simple log message with 1 parameters param1=%d", i); }
}

void
performance2Task(int loopQty)
{
    for (int i = 0; i < loopQty; ++i) { ssInfo("Benchmark", "Simple log message with 2 parameters param1=%d and %f", i, 14.f); }
}

void
performance3Task(int loopQty)
{
    for (int i = 0; i < loopQty; ++i) { ssInfo("Benchmark", "Simple log message with 3 parameters param1=%d and %f %d", i, 14.f, 2 * i); }
}

void
performance4Task(int loopQty)
{
    for (int i = 0; i < loopQty; ++i) {
        ssInfo("Benchmark", "Simple log message with 4 parameters param1=%d and %f %d %d", i, 14.f, 2 * i, -i);
    }
}

void
testPerformance(int threadQty, int loopQty)
{
    sslog::Collector collectorConfig;
    collectorConfig.dataBufferBytes = 100000000;  // Large buffers to avoid waiting
    ssSetCollector(collectorConfig);
    collectorConfig = ssGetCollector();

    sslog::Sink sinkConfig;
    sinkConfig.path         = "sslogDb";
    sinkConfig.storageLevel = sslog::Level::debug;
    sinkConfig.consoleLevel = sslog::Level::off;
    ssSetSink(sinkConfig);
    sinkConfig = ssGetSink();

    sslog::Stats             s;
    double                   dataBufferUsageRatio = 0., stringBufferUsageRatio = 0.;
    uint64_t                 startCollectNs = 0, endCollectNs = 0, endSendingNs = 0;
    std::error_code          ec;
    std::vector<std::thread> threads;
    threads.reserve((uint32_t)threadQty);
    loopQty *= 1000000;
    ssStop();
    printf("                    Collection      Collection+Write\n");

#define RUN_PERF_TEST(perfTask, paramQtyStr)                                                                                              \
    SSLOG_FS_NAMESPACE::remove_all(sinkConfig.path, ec);                                                                                  \
    ssStart();                                                                                                                            \
    startCollectNs = GET_TIME(nanoseconds);                                                                                               \
    for (int threadNbr = 0; threadNbr < threadQty; ++threadNbr) { threads.push_back(std::thread(perfTask, loopQty / threadQty)); }        \
    for (std::thread & t : threads) t.join();                                                                                             \
    endCollectNs = GET_TIME(nanoseconds);                                                                                                 \
    ssStop();                                                                                                                             \
    endSendingNs = GET_TIME(nanoseconds);                                                                                                 \
    threads.clear();                                                                                                                      \
                                                                                                                                          \
    s = ssGetStats();                                                                                                                     \
    dataBufferUsageRatio =                                                                                                                \
        100. * (double)s.maxUsageDataBufferBytes / (double)((collectorConfig.dataBufferBytes > 0) ? collectorConfig.dataBufferBytes : 1); \
    stringBufferUsageRatio = 100. * (double)s.maxUsageStringBufferBytes /                                                                 \
                             (double)((collectorConfig.stringBufferBytes > 0) ? collectorConfig.stringBufferBytes : 1);                   \
    printf("Performance of a loop of logs with " paramQtyStr " parameter on %d threads:\n", threadQty);                                   \
    printf(                                                                                                                               \
        "  peak rate      :    %-6.1f Ml/s       %-6.1f Ml/s  (million logs/s)\n"                                                         \
        "  duration / log :    %-6.1f ns         %-6.1f ns    per log\n"                                                                  \
        "  bytes / log    :    %4.1f\n"                                                                                                   \
        "  total duration :    %-6.2f ms         %-6.2f ms    for %d logs\n"                                                              \
        "  data   buffer  :    %-5.1f%% of max = %-6d KB in %d logs (%d loss)\n"                                                          \
        "  string buffer  :    %-5.1f%% of max = %-6d KB in %d strings\n\n",                                                              \
        1e3 * loopQty / (double)(endCollectNs - startCollectNs), 1e3 * loopQty / (double)(endSendingNs - startCollectNs),                 \
        (double)(endCollectNs - startCollectNs) / loopQty, (double)(endSendingNs - startCollectNs) / loopQty,                             \
        (double)(s.storedBytes) / (double)loopQty, (double)(endCollectNs - startCollectNs) / 1000000.,                                    \
        (double)(endSendingNs - startCollectNs) / 1000000., (int)loopQty, dataBufferUsageRatio, collectorConfig.dataBufferBytes / 1000,   \
        s.storedLogs, s.droppedLogs, stringBufferUsageRatio, collectorConfig.stringBufferBytes / 1000, s.storedStrings)

    RUN_PERF_TEST(performance0Task, "0");
    RUN_PERF_TEST(performance1Task, "1");
    RUN_PERF_TEST(performance2Task, "2");
    RUN_PERF_TEST(performance3Task, "3");
    RUN_PERF_TEST(performance4Task, "4");
}

void
mixOfLogTask(int threadNbr, int loopQty)
{
    constexpr int      MaxThreadNames              = 5;
    static const char* threadNames[MaxThreadNames] = {"main", "second", "third", "fourth", "fifth"};
    if (threadNbr < MaxThreadNames) {
        ssSetThreadName(threadNames[threadNbr]);
    } else {
        char threadName[64];
        snprintf(threadName, sizeof(threadName), "T%d", threadNbr);
        ssSetThreadName(threadName);
    }

    std::vector<uint8_t> buffer1{1, 2, 3, 4, 5, 6, 7, 8};
    std::vector<uint8_t> buffer2{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    for (int loopNbr = 0; loopNbr < loopQty; ++loopNbr) {
        std::vector<uint8_t> buffer{
            (uint8_t)(loopNbr + 1), (uint8_t)(threadNbr + loopNbr + 2), (uint8_t)(loopNbr + 3), (uint8_t)(threadNbr + loopNbr + 4),
            (uint8_t)(loopNbr + 5), (uint8_t)(threadNbr + loopNbr + 6), (uint8_t)(loopNbr + 7), (uint8_t)(threadNbr + loopNbr + 8)};
        ssError("sslog-test", "Start of a batch of 25 mixed logs");
        ssTrace("/module1/stats", "The quick brown fox name_%s jumps over the lazy dog", "Renard");
        ssTrace("sslog-test", "Small log number 1");
        ssWarn("/module1/state", "Lorem ipsum dolor sit amet voltage=%3.1f_V intensity=%dmA, consectetur adipiscing", 3.3, 530);
        ssWarn("sslog-test", "Small log number 2");
        ssInfo("/module1/state", "The quick brown fox name_%s jumps over the lazy dog", "Mr Tod");
        ssDebug("/module1/stats", "Lorem ipsum dolor sit amet voltage=%3.1f_V intensity=%dmA, consectetur adipiscing", 12., 200);
        ssInfo("sslog-test", "Small log number 3");
        ssError("/module2/stats", "The quick brown fox name_%s jumps over the lazy dog", "Basil");
        ssWarn("/module2/state", "Lorem ipsum dolor sit amet voltage=%3.1f_V intensity=%dmA, consectetur adipiscing", 4.4, 640);
        ssDebug("sslog-test", "Small log number 4");
        ssInfo("/module2/state", "The quick brown fox name_%s jumps over the lazy dog", "Roxy");
        ssCritical("/module2/stats", "Lorem ipsum dolor sit amet voltage=%3.1f_V intensity=%dmA, consectetur adipiscing", 11., 104);
        ssError("sslog-test", "Small log number 5");
        ssTraceBuffer("/module3/stats", buffer1.data(), buffer1.size(), "The quick brown fox name_%s jumps over the lazy dog", "Flash");
        ssDebugBuffer("/module3/state", buffer2.data(), buffer2.size(),
                      "Lorem ipsum dolor sit amet voltage=%3.1f_V intensity=%dmA, consectetur adipiscing", 3.5, 550);
        ssWarn("sslog-test", "Small log number 6");
        ssInfoBuffer("/module3/state", buffer1.data(), buffer1.size(), "The quick brown fox name_%s jumps over the lazy dog", "Finn");
        ssDebugBuffer("/module3/stats", buffer2.data(), buffer2.size(),
                      "Lorem ipsum dolor sit amet voltage=%3.1f_V intensity=%dmA, consectetur adipiscing", 14., 420);
        ssInfo("sslog-test", "Small log number 7");
        ssErrorBuffer("/module4/stats", buffer1.data(), buffer1.size(), "The quick brown fox name_%s jumps over the lazy dog", "Happy");
        ssWarnBuffer("/module4/state", buffer2.data(), buffer2.size(),
                     "Lorem ipsum dolor sit amet voltage=%3.1f_V intensity=%dmA, consectetur adipiscing", 3.1, 510);
        ssCritical("sslog-test", "Small log number 8");
        ssTraceBuffer("/module4/state", buffer1.data(), buffer1.size(), "The quick brown fox name_%s jumps over the lazy dog", "Jamie");
        ssCriticalBuffer("/module4/stats", buffer2.data(), buffer2.size(),
                         "Lorem ipsum dolor sit amet voltage=%3.1f_V intensity=%dmA, consectetur adipiscing", 4., 40);
    }
}
