#! /usr/bin/env python3

# System import
import sys
import time

# Local import
from run_tests import *  # Decorators, LOG, CHECK, KPI
from test_base import *  # Test common helpers

# These tests does some performance measurement

# C++ program to evaluate performances
PERF_CODE = r"""#include <cstdlib>
#include "sslog.h"

int main(int argc, char** argv)
{
    sslog::Collector collector;
    collector.dataBufferBytes = 100000000;  // Large buffers to avoid waiting
    ssSetCollector(collector);

    sslog::Sink sink;
    sink.path         = "sslog";
    sink.storageLevel = sslog::Level::info;
    sink.consoleLevel = sslog::Level::off;
    ssSetSink(sink);

    volatile int number = atoi(argv[1]);
#define LOG1()   %s
#define LOG8()   LOG1() LOG1() LOG1() LOG1() LOG1() LOG1() LOG1() LOG1()
#define LOG64()  LOG8() LOG8() LOG8() LOG8() LOG8() LOG8() LOG8() LOG8()

    for(int i=0; i<50000; ++i) { // 50000 times 1024 instances
         LOG64() LOG64() LOG64() LOG64() LOG64() LOG64() LOG64() LOG64()
         LOG64() LOG64() LOG64() LOG64() LOG64() LOG64() LOG64() LOG64()
    }
    ssStop();
    return 0;
}
"""

# C++ program to evaluate the cost of the header instrumentation library
HEADER_CODE = r"""#include <cstdlib>
%s

int main(int argc, char** argv) {
    return 0;
}
"""


def _evaluate_perf_program(eval_content, flags, loop=3, code=PERF_CODE):
    LOG("Experiment with evaluation '%s' and flags '%s'" % (eval_content, " ".join(flags)))

    # Create the source file
    fh = open("test_performance.cpp", "w")
    fh.write(code % eval_content)
    fh.close()

    # Measure the build time
    build_time_sec = 1000.0
    for i in range(loop):
        start_sec = time.time()
        if sys.platform == "win32":
            run_cmd(["cl.exe", "test_performance.cpp", "/std:c++17", "-I", "..\\..\\lib\\include", "/EHs", "/Fea.exe"] + flags)
        else:
            run_cmd(["g++", "test_performance.cpp", "-std=c++17", "-I", "../../lib/include"] + flags)
        build_time_sec = min(build_time_sec, time.time() - start_sec)
    if sys.platform == "win32":
        prog_name = "a.exe"
    else:
        prog_name = "./a.out"
        run_cmd(["strip", prog_name])

    # Measure the program size
    program_size = os.stat(prog_name).st_size

    # Measure the execution time
    exec_time_sec = 1000.0
    for i in range(loop):
        start_sec = time.time()
        run_cmd([prog_name, "14"])
        exec_time_sec = min(exec_time_sec, time.time() - start_sec)

    # Return the performances
    LOG("    build time=%.2f, program size=%d bytes, exec time=%.2f s" % (build_time_sec, program_size, exec_time_sec))
    return build_time_sec, program_size, exec_time_sec


@declare_test("performance")
def measure_log_compactness():
    """Measure log compactness"""

    build_target("testsslog", "", compilation_flags=["-O2"])
    build_target("sscat", "", compilation_flags=["-O2"])
    LOG("Measure the log sizes")
    run_cmd(["./bin/testsslog", "mix", "-t", "4", "-l", "10000"])  # 4 threads x 10000 loops x 25 logs = 1 million logs
    res = run_cmd(["./bin/sscat", "sslogDb"])
    referenceSize = len(res.stdout)
    logSize = os.path.getsize("./sslogDb/data000000.sslog")
    try:
        run_cmd(["zstd", "./sslogDb/data000000.sslog", "-5"], silent=True)
        logZstSize = os.path.getsize("./sslogDb/data000000.sslog.zst")
        KPI("sslog - compactness - sslog+zstd -5 vs text", "%.1f %%" % (100.*logZstSize/referenceSize))
    except:
        pass  # No Zstd installed, skip this KPI
    KPI("sslog - compactness - full text", "100%% (1e6 logs / %.1f MB)" % (1e-6*referenceSize))
    KPI("sslog - compactness - sslog         vs text", "%.1f %%" % (100.*logSize/referenceSize))


@declare_test("performance")
def measure_event_size_and_timing():
    """Measure code size, build time and execution times"""

    # Cost of including the header
    inc0_built_time_sec, inc0_program_size, inc0_exec_time_sec = _evaluate_perf_program("", [], code=HEADER_CODE)
    inc1_built_time_sec, inc1_program_size, inc1_exec_time_sec = _evaluate_perf_program('#include "sslog.h"', ["-DSSLOG_DISABLE=1"], code=HEADER_CODE)
    inc2_built_time_sec, inc2_program_size, inc2_exec_time_sec = _evaluate_perf_program('#include "sslog.h"', [], code=HEADER_CODE)

    # Fully disabled
    (noSsO_build_time_sec, noSsO_program_size, noSsO_exec_time_sec) = _evaluate_perf_program("", ["-DSSLOG_DISABLE=1", "-O2"])

    # Optimized log measures
    refO_build_time_sec, refO_program_size, refO_exec_time_sec = _evaluate_perf_program("", ["-O2"])
    logO1_build_time_sec, logO1_program_size, logO1_exec_time_sec = _evaluate_perf_program("ssInfo(\"benchmark\", \"text\");", ["-O2"])
    logSk_build_time_sec, logSk_program_size, logSk_exec_time_sec = _evaluate_perf_program("ssDebug(\"benchmark\", \"text\");", ["-O2"])
    logO2_build_time_sec, logO2_program_size, logO2_exec_time_sec = _evaluate_perf_program(
        "ssInfo(\"benchmark\", \"text and 1 numeric arg %d\", number);", ["-O2"])
    logO3_build_time_sec, logO3_program_size, logO3_exec_time_sec = _evaluate_perf_program(
        "ssInfo(\"benchmark\", \"text and 1 string arg %s\", \"argument\");", ["-O2"])
    logO4_build_time_sec, logO4_program_size, logO4_exec_time_sec = _evaluate_perf_program(
        "ssInfo(\"benchmark\", \"text and 4 numeric args %d %d %d %d\", number, 2*number, 4*number, 8*number);", ["-O2"])

    # Debug event measures (for build timings only)
    debugOpt = "-Od" if sys.platform == "win32" else "-O0"
    refg_build_time_sec, refg_program_size, refg_exec_time_sec = _evaluate_perf_program("", [debugOpt])
    (logg_build_time_sec, small_program_size, small_exec_time_sec) = _evaluate_perf_program("ssInfo(\"benchmark\", \"text\");", [debugOpt])

    # Log the KPIs
    KPI("sslog - Log compilation speed (-O2)", "%3d log/s" % (1024.0 / (logO1_build_time_sec - refO_build_time_sec)))
    KPI("sslog - Log compilation speed (%s)" % debugOpt, "%3d log/s" % (1024.0 / (logg_build_time_sec - refg_build_time_sec)))

    KPI("sslog - Log code size 0 param (-O2)", "%d bytes/log" % int((logO1_program_size - refO_program_size) / 1024))
    KPI("sslog - Log code size 1 param int (-O2)", "%d bytes/log" % int((logO2_program_size - refO_program_size) / 1024))
    KPI("sslog - Log code size 1 param string (-O2)", "%d bytes/log" % int((logO3_program_size - refO_program_size) / 1024))
    KPI("sslog - Log code size 4 params int (-O2)", "%d bytes/log" % int((logO4_program_size - refO_program_size) / 1024))

    KPI("sslog - Logging runtime skipped log", "%.1f ns or %.2f Mlog/s" % (1e9 * (logSk_exec_time_sec - refO_exec_time_sec) / (1024 * 50000.0),
                                                                           (1024 * 50000.0) / (1e6 * (logSk_exec_time_sec - refO_exec_time_sec))))
    KPI("sslog - Logging runtime 0 param", "%.1f ns or %.2f Mlog/s" % (1e9 * (logO1_exec_time_sec - refO_exec_time_sec) / (1024 * 50000.0),
                                                                       (1024 * 50000.0) / (1e6 * (logO1_exec_time_sec - refO_exec_time_sec))))
    KPI("sslog - Logging runtime 1 param int", "%.1f ns or %.2f Mlog/s" % (1e9 * (logO2_exec_time_sec - refO_exec_time_sec) / (1024 * 50000.0),
                                                                           (1024 * 50000.0) / (1e6 * (logO2_exec_time_sec - refO_exec_time_sec))))
    KPI("sslog - Logging runtime 1 param string", "%.1f ns or %.2f Mlog/s" % (1e9 * (logO3_exec_time_sec - refO_exec_time_sec) / (1024 * 50000.0),
                                                                              (1024 * 50000.0) / (1e6 * (logO3_exec_time_sec - refO_exec_time_sec))))
    KPI("sslog - Logging runtime 4 params int", "%.1f ns or %.2f Mlog/s" % (1e9 * (logO4_exec_time_sec - refO_exec_time_sec) / (1024 * 50000.0),
                                                                            (1024 * 50000.0) / (1e6 * (logO4_exec_time_sec - refO_exec_time_sec))))

    KPI("sslog - Header include - SSLOG_DISABLE=1", "%.3f s" % max(0, inc1_built_time_sec - inc0_built_time_sec))
    KPI("sslog - Header include", "%.3f s" % (inc2_built_time_sec - inc0_built_time_sec))

    KPI("sslog - Library code size (-O2)", "%d bytes" % int(refO_program_size - noSsO_program_size),)
