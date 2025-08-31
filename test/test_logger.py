#! /usr/bin/env python3

# System import
import os
import sys
import time
import glob

# Local import
from run_tests import *  # Decorators, LOG, CHECK, KPI
from test_base import *  # Test common helpers

# These tests check the logger behavior


def compile_and_run_fileinfos(descr, code, doCompress=False):
    LOG("Check logger behavior '%s' " % descr)

    # Create the source file
    fh = open("test_behavior.cpp", "w")
    fh.write(code)
    fh.close()

    # Build it
    if sys.platform == "win32":
        prog_name = "a.exe"
        run_cmd(["cl.exe", "test_behavior.cpp", "/std:c++17", "-I", "..\\..\\lib\\include", "/EHs", "/Fea.exe"])
    else:
        prog_name = "./a.out"
        run_cmd(["g++", "test_behavior.cpp", "-std=c++17", "-g", "-I", "../../lib/include"])

    # Execute
    run_cmd([prog_name])

    # Compression
    if doCompress:
        run_cmd["zstd", "-r", "--rm", "-1", "sslogDb"]

    # Return data file information
    fileList = glob.glob("sslogDb/data*")
    fileList.sort()
    dataInfos = []
    for f in fileList:
        size = os.path.getsize(f)
        number = int(f.replace("sslogDb/data", "").replace("sslogDb\\data", "").replace(".sslog", "").replace(".dtl", "").replace(".zst", ""))
        dataInfos.append({"filename": f, "size": size, "number": number, "details": (".dtl" in f)})

    return dataInfos


FILE_MGT_BASE_CODE = r"""
#define SSLOG_VIRTUAL_TIME_FOR_TEST_ONLY 1
#include "sslog.h"
int main()
{
    // Small buffers to flush often
    sslog::Collector collector;
    collector.dataBufferBytes = 2000;
    ssSetCollector(collector);

    // Sink configuration with dynamic part
    sslog::Sink sink;
    sink.path         = "sslogDb";
    sink.consoleLevel = sslog::Level::off;
    sink.storageLevel = sslog::Level::info;
    %s
    ssSetSink(sink);

    // 1KB buffers are logged to create large log files
    std::vector<uint8_t> buffer(1024);
    for(int i=0; i<1024; ++i) buffer[i] = (uint8_t)i;

    for(int i=0; i<2000; ++i) {
         ssInfoBuffer("category", buffer.data(), buffer.size(), "text");

         // Regular logging of 200 logs (~200 KB) per second
         sslog::priv::testIncrementVirtualTimeMs(5);

         // Force flush after each log to have consistent results
         // Required because the threading time is not virtualized
         sslog::priv::forceFlush();
    }
    return 0;
}
"""


@declare_test("logger")
def check_file_split_and_quantity():
    """Check file split and controlling quantity"""

    dataInfos = compile_and_run_fileinfos("file unsplit", FILE_MGT_BASE_CODE % "")
    CHECK(len(dataInfos) == 1, "Files are unsplit by default (%d files seen)" % len(dataInfos))

    dataInfos = compile_and_run_fileinfos("file split by size", FILE_MGT_BASE_CODE % "sink.splitFileMaxBytes = 10000;")
    CHECK(len(dataInfos) > 20, "Files are split by size (%d files seen)" % len(dataInfos))

    dataInfos = compile_and_run_fileinfos("file split by size and limited quantity", FILE_MGT_BASE_CODE %
                                          "sink.splitFileMaxBytes = 10000; sink.fileMaxQty = 10;")
    CHECK(len(dataInfos) == 10, "Files are split by size and quantity is limited to 10 (%d files seen)" % len(dataInfos))

    dataInfos = compile_and_run_fileinfos("file split by duration", FILE_MGT_BASE_CODE % "sink.splitFileMaxDurationSec = 2;")
    CHECK(len(dataInfos) == 5, "Files are split by duration (%d files seen)" % len(dataInfos))

    dataInfos = compile_and_run_fileinfos("file split by duration and limited quantity", FILE_MGT_BASE_CODE %
                                          "sink.splitFileMaxDurationSec = 1; sink.fileMaxQty = 5;")
    CHECK(len(dataInfos) == 5, "Files are split by duration and quantity is limited to 5 (%d files seen)" % len(dataInfos))

    dataInfos = compile_and_run_fileinfos("file split by duration and limited by age", FILE_MGT_BASE_CODE %
                                          "sink.splitFileMaxDurationSec = 1; sink.fileMaxFileAgeSec = 3;")
    CHECK(len(dataInfos) == 3+1, "Files are split by duration and age is limited to 3s (%d files seen)" % len(dataInfos))

    dataInfos = compile_and_run_fileinfos("file split by duration and no logging at all", FILE_MGT_BASE_CODE %
                                          "sink.splitFileMaxDurationSec = 1; sink.storageLevel = sslog::Level::critical;")
    CHECK(len(dataInfos) == 0, "No logging means no data file (%d files seen)" % len(dataInfos))


def compile_and_run_stdout(descr, code):
    LOG("Check logger behavior '%s' " % descr)

    # Create the source file
    fh = open("test_behavior.cpp", "w")
    fh.write(code)
    fh.close()

    # Build it
    if sys.platform == "win32":
        prog_name = "a.exe"
        run_cmd(["cl.exe", "test_behavior.cpp", "/std:c++17", "-I", "..\\..\\lib\\include", "/EHs", "/Fea.exe"])
    else:
        prog_name = "./a.out"
        run_cmd(["g++", "test_behavior.cpp", "-std=c++17", "-g", "-I", "../../lib/include"])

    # Execute
    res = run_cmd([prog_name])
    return dict([(w.split(" ")[0], int(w.split(" ")[1])) for w in res.stdout.split("\n") if w])


SATURATION_POLICY_BASE_CODE = r"""
#define SSLOG_VIRTUAL_TIME_FOR_TEST_ONLY 1
#include "sslog.h"
int main()
{
    // Small buffers to saturate easily
    sslog::Collector collector;
    collector.dataBufferBytes = 16384;
    collector.stringBufferBytes = 1024;
    %s
    ssSetCollector(collector);

    // Sink configuration with dynamic part
    sslog::Sink sink;
    sink.path         = "sslogDb";
    sink.consoleLevel = sslog::Level::off;
    sink.storageLevel = sslog::Level::info;
    ssSetSink(sink);

    // 1KB buffers are logged to create large log files
    std::vector<uint8_t> buffer(1024);
    for(int i=0; i<1024; ++i) buffer[i] = (uint8_t)i;

    // Big strings are logged to saturate the buffer. They should be truncated
    std::string longValue(%d, 'A');

    for(int i=0; i<2000; ++i) {
         ssInfoBuffer("category", buffer.data(), buffer.size(), "text %%s", longValue.c_str());

         // Regular logging of 200 logs (~200 KB) per second
         sslog::priv::testIncrementVirtualTimeMs(5);

         // Force flush to have consistent results
         // Required because the threading time is not virtualized
         if((i%%20)==0) { sslog::priv::forceFlush(); }
    }

    sslog::Stats s = ssGetStats();
    printf("storedLogs %%d\n", s.storedLogs);
    printf("storedBytes %%d\n", s.storedBytes);
    printf("droppedLogs %%d\n", s.droppedLogs);
    printf("delayedLogs %%d\n", s.delayedLogs);
    printf("delayedStrings %%d\n", s.delayedStrings);
    return 0;
}
"""


@declare_test("logger")
def check_saturation_policies():
    """Check the saturation policies"""

    stats = compile_and_run_stdout("Data saturation with wait policy", SATURATION_POLICY_BASE_CODE %
                                   ("collector.dataSaturationPolicy = sslog::SaturationPolicy::Wait;", 100))
    CHECK(stats["droppedLogs"] == 0, "No dropped logs (%d seen)" % stats["droppedLogs"])
    CHECK(stats["delayedLogs"] > 0, "Delayed logs (%d seen)" % stats["delayedLogs"])
    CHECK(stats["delayedStrings"] == 0, "No delayed strings (%d seen)" % stats["delayedStrings"])

    stats = compile_and_run_stdout("Data saturation with drop policy", SATURATION_POLICY_BASE_CODE %
                                   ("collector.dataSaturationPolicy = sslog::SaturationPolicy::Drop;", 100))
    CHECK(stats["droppedLogs"] > 0, "Dropped logs (%d seen)" % stats["droppedLogs"])
    CHECK(stats["delayedLogs"] == 0, "No delayed logs (%d seen)" % stats["delayedLogs"])
    CHECK(stats["delayedStrings"] == 0, "No delayed strings (%d seen)" % stats["delayedStrings"])

    stats = compile_and_run_stdout("String saturation", SATURATION_POLICY_BASE_CODE % ("", 2000))
    CHECK(stats["droppedLogs"] == 0, "No dropped logs (%d seen)" % stats["droppedLogs"])
    CHECK(stats["delayedLogs"] > 0, "Delayed logs (%d seen)" % stats["delayedLogs"])
    CHECK(stats["delayedStrings"] == 1, "Delayed strings (%d seen)" % stats["delayedStrings"])


REQUEST_FOR_DETAILS_BASE_CODE = r"""
#define SSLOG_VIRTUAL_TIME_FOR_TEST_ONLY 1
#include "sslog.h"
int main()
{
    // Small buffers to flush often
    sslog::Collector collector;
    collector.dataBufferBytes = 2000;
    ssSetCollector(collector);

    // Sink configuration with dynamic part
    sslog::Sink sink;
    sink.path         = "sslogDb";
    sink.consoleLevel = sslog::Level::off;
    sink.storageLevel = sslog::Level::info;
    sink.detailsLevel = sslog::Level::trace;
    sink.splitFileMaxDurationSec = 1;
    %s
    ssSetSink(sink);

    for(int i=0; i<2000; ++i) {

         ssInfo("category", "text");
         ssDebug("category", "Detailed text");

         // Request for details in the file N (200 logs/s)
         if(i==200*%d + 100) {
           ssRequestForDetails();
         }

         // Regular logging of 200 logs per second
         sslog::priv::testIncrementVirtualTimeMs(5);

         // Force flush after each log to have consistent results
         // Required because the threading time is not virtualized
         sslog::priv::forceFlush();
    }
    return 0;
}
"""


@declare_test("logger")
def check_request_for_details():
    """Check the request for details feature"""

    dataInfos = compile_and_run_fileinfos("request for details with zero range around", REQUEST_FOR_DETAILS_BASE_CODE %
                                          ("sink.detailsBeforeAfterMinSec = 0;", 5))
    CHECK(len(dataInfos) > 7, "Files are split by duration (%d files seen)" % len(dataInfos))
    CHECK(len([d for d in dataInfos if d["details"]]) == 1, "1 data file with details is present")

    dataInfos = compile_and_run_fileinfos("request for details with 1s range around", REQUEST_FOR_DETAILS_BASE_CODE % ("sink.detailsBeforeAfterMinSec = 1;", 5))
    CHECK(len(dataInfos) > 7, "Files are split by duration (%d files seen)" % len(dataInfos))
    CHECK(len([d for d in dataInfos if d["details"]]) == 3, "1+2 data files with details are present")

    dataInfos = compile_and_run_fileinfos("request for details with 2s range around", REQUEST_FOR_DETAILS_BASE_CODE % ("sink.detailsBeforeAfterMinSec = 2;", 5))
    CHECK(len(dataInfos) > 7, "Files are split by duration (%d files seen)" % len(dataInfos))
    CHECK(len([d for d in dataInfos if d["details"]]) == 5, "1+2*2 data files with details are present")

    dataInfos = compile_and_run_fileinfos("request for details with infinite range around", REQUEST_FOR_DETAILS_BASE_CODE %
                                          ("sink.detailsBeforeAfterMinSec = 10;", 5))
    CHECK(len(dataInfos) > 7, "Files are split by duration (%d files seen)" % len(dataInfos))
    CHECK(len([d for d in dataInfos if d["details"]]) == len(dataInfos) // 2, "all data files with details are present")

    dataInfos = compile_and_run_fileinfos("request for details with zero range around and no standard log", REQUEST_FOR_DETAILS_BASE_CODE %
                                          ("sink.storageLevel = sslog::Level::off; sink.detailsBeforeAfterMinSec = 0;", 5))
    CHECK(len(dataInfos) == 1, "Files are split by duration (%d files seen)" % len(dataInfos))
    CHECK(len([d for d in dataInfos if d["details"]]) == 1, "the data file is a detailed one")

    dataInfos = compile_and_run_fileinfos("request for details with infinite range around and no standard log", REQUEST_FOR_DETAILS_BASE_CODE %
                                          ("sink.storageLevel = sslog::Level::off; sink.detailsBeforeAfterMinSec = 10;", 5))
    CHECK(len(dataInfos) > 7, "Files are split by duration (%d files seen)" % len(dataInfos))
    CHECK(len([d for d in dataInfos if d["details"]]) == len(dataInfos), "all data files are detailed ones")


LIVE_NOTIFICATION_BASE_CODE = r"""
#define SSLOG_VIRTUAL_TIME_FOR_TEST_ONLY 1
#include "sslog.h"
int main()
{
    // Sink configuration with dynamic part
    sslog::Sink sink;
    sink.path         = "sslogDb";
    sink.consoleLevel = sslog::Level::off;
    sink.storageLevel = sslog::Level::off;
    sink.detailsLevel = sslog::Level::off;
    sink.liveNotifLevel = sslog::Level::error;
    uint32_t counter = 0;
    sink.liveNotifCbk = [&counter](uint64_t timestampUtcNs, uint32_t level, const char* threadName, const char* category,
                                   const char* logContent, const uint8_t* binaryBuffer, uint32_t binaryBufferSize)
                                    { ++counter; };
    ssSetSink(sink);

    for(int i=0; i<2000; ++i) {

         ssInfo("category", "text");

         // Error level message sent each 100 loop
         if((i%100)==0) {
           ssError("category", "Important processing %d", i);
         }

         // Regular logging of 200 logs per second
         sslog::priv::testIncrementVirtualTimeMs(5);

         // Force flush after each log to have consistent results
         // Required because the threading time is not virtualized
         sslog::priv::forceFlush();
    }
    printf("counter %d\n", counter);
    return 0;
}
"""


@declare_test("logger")
def check_live_notifications():
    """Check the live notifications"""
    stats = compile_and_run_stdout("Live notification", LIVE_NOTIFICATION_BASE_CODE)
    CHECK(stats["counter"] == 20, "Received notifications is %d" % stats["counter"])


LOG_GROUP_BASE_CODE = r"""
#define SSLOG_VIRTUAL_TIME_FOR_TEST_ONLY 1
#include "sslog.h"

#define SSLOG_GROUP_MY_TEST %d

int main()
{
    // Sink configuration with dynamic part
    sslog::Sink sink;
    sink.path         = "sslogDb";
    sink.consoleLevel = sslog::Level::off;
    sink.storageLevel = sslog::Level::debug;
    ssSetSink(sink);

    for(int i=0; i<1000; ++i) {
         ssInfo("category", "text");
         ssgInfo(MY_TEST, "compile-time category", "text");
    }

    sslog::Stats s = ssGetStats();
    printf("storedLogs %%d\n", s.storedLogs);
    return 0;
}
"""


@declare_test("logger")
def check_log_groups():
    """Check compile-time log groups"""

    stats = compile_and_run_stdout("Log group enabled", LOG_GROUP_BASE_CODE % 1)
    CHECK(stats["storedLogs"] == 1000+1000, "Collected log quantity is %d" % stats["storedLogs"])

    stats = compile_and_run_stdout("Log group disabled", LOG_GROUP_BASE_CODE % 0)
    CHECK(stats["storedLogs"] == 1000, "Collected log quantity is %d" % stats["storedLogs"])


FILE_CLEANING_CODE = r"""
#define SSLOG_VIRTUAL_TIME_FOR_TEST_ONLY 1
#include "sslog.h"
int main()
{
    // Small buffers to flush often
    sslog::Collector collector;
    collector.dataBufferBytes = 2000;
    ssSetCollector(collector);

    // Sink configuration with dynamic part
    sslog::Sink sink;
    sink.path         = "sslogDb";
    sink.consoleLevel = sslog::Level::off;
    sink.storageLevel = sslog::Level::info;
    sink.detailsLevel = sslog::Level::trace;
    %s
    ssSetSink(sink);

    // Initial log
    ssInfo("category", "initial log");

    // Only detailed logging during 60s
    for(int i=0; i<600; ++i) {
         ssTrace("details", "middle log");
         sslog::priv::testIncrementVirtualTimeMs(100);
         sslog::priv::forceFlush();
    }

    // The final log
    ssInfo("category", "final log");
    return 0;
}
"""


@declare_test("logger")
def check_empty_file_cleaning():
    """Check empty file cleaning"""

    dataInfos = compile_and_run_fileinfos("file split by duration", FILE_CLEANING_CODE % "sink.splitFileMaxDurationSec = 2;")
    CHECK(len(dataInfos) == 2, "Only non-empty files are kept (%d files seen)" % len(dataInfos))
