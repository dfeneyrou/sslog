#! /usr/bin/env python3

# System import
import sys
import time

# Local import
from run_tests import *  # Decorators, LOG, CHECK, KPI
from test_base import *      # Test common helpers

# These tests does some performance measurement

# It requires the package spdlog-dev (On linux: apt install libspdlog-dev)

# C++ program to evaluate performances
PERF_CODE = r"""
#include <cstdlib>

#ifndef NO_SPDLOG
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#endif

int main(int argc, char** argv)
{
#ifndef NO_SPDLOG
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("sslog.txt");
    sink->set_level(spdlog::level::info);
    spdlog::init_thread_pool(65536, 1);
    auto logger = std::make_shared<spdlog::async_logger>("category", sink, spdlog::thread_pool(), spdlog::async_overflow_policy::block);
#endif

    volatile int number = atoi(argv[1]);
#define LOG1()   %s
#define LOG8()   LOG1() LOG1() LOG1() LOG1() LOG1() LOG1() LOG1() LOG1()
#define LOG64()  LOG8() LOG8() LOG8() LOG8() LOG8() LOG8() LOG8() LOG8()

    for(int i=0; i<1000; ++i) { // 1000 times 1024 instances
         LOG64() LOG64() LOG64() LOG64() LOG64() LOG64() LOG64() LOG64()
         LOG64() LOG64() LOG64() LOG64() LOG64() LOG64() LOG64() LOG64()
    }
#ifndef NO_SPDLOG
    spdlog::shutdown();
#endif
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


def _evaluate_perf_program(eval_content, flags, loop=3, code=PERF_CODE, silent=False):
    if not silent:
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
            run_cmd(["cl.exe", "test_performance.cpp", "-I", "..\\..\\lib\\include", "/EHs", "/Fea.exe"] + flags, silent=silent)
        else:
            run_cmd(["g++", "test_performance.cpp", "-I", "../../lib/include", "-lpthread", "-lspdlog", "-lfmt"] + flags, silent=silent)
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


@declare_test("performance_spdlog")
def measure_event_size_and_timing():
    """Measure code size, build time and execution times for spdlog"""

    # Detect is spdlog is installed
    try:
        _evaluate_perf_program('#include <spdlog/spdlog.h>', [], code=HEADER_CODE, silent=True)
    except:
        # No SPDLOG installed, skip this test
        return

    # Cost of including the header
    inc0_built_time_sec, inc0_program_size, inc0_exec_time_sec = _evaluate_perf_program("", [], code=HEADER_CODE)
    inc1_built_time_sec, inc1_program_size, inc1_exec_time_sec = _evaluate_perf_program('#include <spdlog/spdlog.h>', [], code=HEADER_CODE)

    # Fully disabled
    (noLogO_build_time_sec, noLogO_program_size, noLogO_exec_time_sec) = _evaluate_perf_program("", ["-DNO_SPDLOG=1", "-O2"])

    # Optimized log measures
    refO_build_time_sec, refO_program_size, refO_exec_time_sec = _evaluate_perf_program("", ["-O2"])
    logO1_build_time_sec, logO1_program_size, logO1_exec_time_sec = _evaluate_perf_program("logger->info(\"text\");", ["-O2"])
    logSk_build_time_sec, logSk_program_size, logSk_exec_time_sec = _evaluate_perf_program("logger->debug(\"text\");", ["-O2"])
    logO2_build_time_sec, logO2_program_size, logO2_exec_time_sec = _evaluate_perf_program(
        "logger->info(\"text and 1 numeric arg {}\", number);", ["-O2"])
    logO3_build_time_sec, logO3_program_size, logO3_exec_time_sec = _evaluate_perf_program(
        "logger->info(\"text and 1 string arg {}\", \"argument\");", ["-O2"])
    logO4_build_time_sec, logO4_program_size, logO4_exec_time_sec = _evaluate_perf_program(
        "logger->info(\"text and 4 numeric args {} {} {} {}\", number, 2*number, 4*number, 8*number);", ["-O2"])

    # Debug event measures (for build timings only)
    debugOpt = "-Od" if sys.platform == "win32" else "-O0"
    refg_build_time_sec, refg_program_size, refg_exec_time_sec = _evaluate_perf_program("", [debugOpt])
    (logg_build_time_sec, small_program_size, small_exec_time_sec) = _evaluate_perf_program("logger->info(\"text\");", [debugOpt])

    # Log the KPIs
    KPI("spdlog - Log compilation speed (-O2)", "%3d log/s" % (1024.0 / (logO1_build_time_sec - refO_build_time_sec)))
    KPI("spdlog - Log compilation speed (%s)" % debugOpt, "%3d log/s" % (1024.0 / (logg_build_time_sec - refg_build_time_sec)))

    KPI("spdlog - Log code size 0 param (-O2)", "%d bytes/log" % int((logO1_program_size - refO_program_size) / 1024))
    KPI("spdlog - Log code size 1 param int (-O2)", "%d bytes/log" % int((logO2_program_size - refO_program_size) / 1024))
    KPI("spdlog - Log code size 1 param string (-O2)", "%d bytes/log" % int((logO3_program_size - refO_program_size) / 1024))
    KPI("spdlog - Log code size 4 params int (-O2)", "%d bytes/log" % int((logO4_program_size - refO_program_size) / 1024))

    KPI("spdlog - Logging runtime skipped log", "%.1f ns or %.2f Mlog/s" % (1e9 * (logSk_exec_time_sec - refO_exec_time_sec) / (1024 * 1000.0),
                                                                            (1024 * 1000.0) / (1e6 * (logSk_exec_time_sec - refO_exec_time_sec))))
    KPI("spdlog - Logging runtime 0 param", "%.1f ns or %.2f Mlog/s" % (1e9 * (logO1_exec_time_sec - refO_exec_time_sec) / (1024 * 1000.0),
                                                                        (1024 * 1000.0) / (1e6 * (logO1_exec_time_sec - refO_exec_time_sec))))
    KPI("spdlog - Logging runtime 1 param int", "%.1f ns or %.2f Mlog/s" % (1e9 * (logO2_exec_time_sec - refO_exec_time_sec) / (1024 * 1000.0),
                                                                            (1024 * 1000.0) / (1e6 * (logO2_exec_time_sec - refO_exec_time_sec))))
    KPI("spdlog - Logging runtime 1 param string", "%.1f ns or %.2f Mlog/s" % (1e9 * (logO3_exec_time_sec - refO_exec_time_sec) / (1024 * 1000.0),
                                                                               (1024 * 1000.0) / (1e6 * (logO3_exec_time_sec - refO_exec_time_sec))))
    KPI("spdlog - Logging runtime 4 params int", "%.1f ns or %.2f Mlog/s" % (1e9 * (logO4_exec_time_sec - refO_exec_time_sec) / (1024 * 1000.0),
                                                                             (1024 * 1000.0) / (1e6 * (logO4_exec_time_sec - refO_exec_time_sec))))

    KPI("spdlog - Header include", "%.3f s" % (inc1_built_time_sec - inc0_built_time_sec))

    KPI("spdlog - Library code size (-O2)", "%d bytes" % int(refO_program_size - noLogO_program_size))
