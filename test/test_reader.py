#! /usr/bin/env python3

# System import
import sys
import time

# Library unter test import
import sslogread

# Local import
from run_tests import *  # Decorators, LOG, CHECK, KPI
from test_base import *      # Test common helpers

# These tests check the behavior of the 'sslog' Python reader library
# This also indirectly checks the underlying C++ reader library, as the
# Python module is a wrapper.


@prepare_suite("reader")
def prepare_build():
    # Builds the reference log
    build_target("testsslog", "")
    # Run the 'mix' sub program with 3 threads (each outputing 25 logs)
    run_cmd(["./bin/testsslog", "mix", "-t", "3"])
    # Logs are ready in './sslogDb'!


@declare_test("reader")
def test_reader_individual_filters():
    """Test the reader filters one by one"""

    def checkLogQty(testedName, testedDict, expectedQty):
        res = session.query(testedDict)
        CHECK(len(res) == expectedQty, "Filtering logs on %s: %d logs expected, %d seen" % (testedName, expectedQty, len(res)))
        return res

    session = sslogread.load("./sslogDb")

    LOG("Checking all unique types of filters")
    checkLogQty("nothing", {}, 3*25)

    res = checkLogQty("level_min", {'level_min': "info"}, 3*17)
    CHECK(not [l for l in res if l.level not in ["info", "warn", "error", "critical"]], "Check filtered content")
    res = checkLogQty("level_min", {'level_max': "debug"}, 3*8)
    CHECK(not [l for l in res if l.level not in ["trace", "debug"]], "Check filtered content")

    res = checkLogQty("category (exact)", {'category': "sslog-test"}, 3*9)
    CHECK(not [l for l in res if l.category != "sslog-test"], "Check filtered content")
    res = checkLogQty("category (pattern1)", {'category': "/mod*"}, 3*16)
    CHECK(not [l for l in res if not l.category.startswith("/mod")], "Check filtered content")
    res = checkLogQty("category (pattern2)", {'category': "*state"}, 3*8)
    CHECK(not [l for l in res if not l.category.endswith("state")], "Check filtered content")
    res = checkLogQty("category (pattern3)", {'category': "*1/*"}, 3*4)
    CHECK(not [l for l in res if not "1/" in l.category], "Check filtered content")

    res = checkLogQty("no category (exact)", {'no_category': "sslog-test"}, 3*(25-9))
    CHECK(not [l for l in res if l.category == "sslog-test"], "Check filtered content")
    res = checkLogQty("no category (pattern1)", {'no_category': "/mod*"}, 3*(25-16))
    CHECK(not [l for l in res if l.category.startswith("/mod")], "Check filtered content")
    res = checkLogQty("no category (pattern2)", {'no_category': "*state"}, 3*(25-8))
    CHECK(not [l for l in res if l.category.endswith("state")], "Check filtered content")
    res = checkLogQty("no category (pattern3)", {'no_category': "*1/*"}, 3*(25-4))
    CHECK(not [l for l in res if "1/" in l.category], "Check filtered content")

    res = checkLogQty("thread (exact)", {'thread': "main"}, 25)
    CHECK(not [l for l in res if l.thread != "main"], "Check filtered content")
    res = checkLogQty("thread (pattern1)", {'thread': "se*"}, 25)
    CHECK(not [l for l in res if not l.thread.startswith("se")], "Check filtered content")
    res = checkLogQty("thread (pattern2)", {'thread': "*rd"}, 25)
    CHECK(not [l for l in res if not l.thread.endswith("rd")], "Check filtered content")
    res = checkLogQty("thread (pattern3)", {'thread': "*d*"}, 50)
    CHECK(not [l for l in res if not "d" in l.thread], "Check filtered content")

    res = checkLogQty("no thread (exact)", {'no_thread': "main"}, 50)
    CHECK(not [l for l in res if l.thread == "main"], "Check filtered content")
    res = checkLogQty("no thread (pattern1)", {'no_thread': "se*"}, 50)
    CHECK(not [l for l in res if l.thread.startswith("se")], "Check filtered content")
    res = checkLogQty("no thread (pattern2)", {'no_thread': "*rd"}, 50)
    CHECK(not [l for l in res if l.thread.endswith("rd")], "Check filtered content")
    res = checkLogQty("no thread (pattern3)", {'no_thread': "*d*"}, 25)
    CHECK(not [l for l in res if "d" in l.thread], "Check filtered content")

    res = checkLogQty("format (exact)", {'format': "Start of a batch of 25 mixed logs"}, 3*1)
    CHECK(not [l for l in res if l.format != "Start of a batch of 25 mixed logs"], "Check filtered content")
    res = checkLogQty("format (pattern1)", {'format': "Lorem*"}, 3*8)
    CHECK(not [l for l in res if not l.format.startswith("Lorem")], "Check filtered content")
    res = checkLogQty("format (pattern2)", {'format': "* lazy dog"}, 3*8)
    CHECK(not [l for l in res if not l.format.endswith(" lazy dog")], "Check filtered content")
    res = checkLogQty("format (pattern3)", {'format': "*ic*"}, 3*8)
    CHECK(not [l for l in res if not "ic" in l.format], "Check filtered content")

    res = checkLogQty("no format (exact)", {'no_format': "Start of a batch of 25 mixed logs"}, 3*(25-1))
    CHECK(not [l for l in res if l.format == "Start of a batch of 25 mixed logs"], "Check filtered content")
    res = checkLogQty("no format (pattern1)", {'no_format': "Lorem*"}, 3*(25-8))
    CHECK(not [l for l in res if l.format.startswith("Lorem")], "Check filtered content")
    res = checkLogQty("no format (pattern2)", {'no_format': "* lazy dog"}, 3*(25-8))
    CHECK(not [l for l in res if l.format.endswith(" lazy dog")], "Check filtered content")
    res = checkLogQty("no format (pattern3)", {'no_format': "*ic*"}, 3*(25-8))
    CHECK(not [l for l in res if "ic" in l.format], "Check filtered content")

    res = checkLogQty("buffer presence", {'buffer_size_min': 1}, 3*8)
    CHECK(not [l for l in res if l.buffer == None or len(l.buffer) == 0], "Check filtered content")
    res = checkLogQty("buffer size min", {'buffer_size_min': 9}, 3*4)
    CHECK(not [l for l in res if len(l.buffer) < 9], "Check filtered content")
    res = checkLogQty("buffer size max", {'buffer_size_max': 12}, 3*(25-4))
    CHECK(not [l for l in res if l.buffer != None and len(l.buffer) > 12], "Check filtered content")
    res = checkLogQty("buffer size min and max", {'buffer_size_min': 1, 'buffer_size_max': 12}, 3*4)
    CHECK(not [l for l in res if len(l.buffer) > 12], "Check filtered content")

    res = checkLogQty("argument presence", {'arguments': ["voltage"]}, 3*8)
    CHECK(not [l for l in res if not [a for a in l.arguments if a[0] == "voltage"]], "Check filtered content")  # Tuple (name, unit, value)
    res = checkLogQty("argument value '='", {'arguments': ["voltage=11."]}, 3*1)
    CHECK(not [l for l in res if not [a for a in l.arguments if a[0] == "voltage" and a[2] == 11.]], "Check filtered content")
    res = checkLogQty("argument value '=='", {'arguments': ["voltage==11."]}, 3*1)
    CHECK(not [l for l in res if not [a for a in l.arguments if a[0] == "voltage" and a[2] == 11.]], "Check filtered content")
    res = checkLogQty("argument value '>'", {'arguments': ["voltage>11."]}, 3*2)
    CHECK(not [l for l in res if not [a for a in l.arguments if a[0] == "voltage" and a[2] > 11.]], "Check filtered content")
    res = checkLogQty("argument value '>='", {'arguments': ["voltage>=11."]}, 3*3)
    CHECK(not [l for l in res if not [a for a in l.arguments if a[0] == "voltage" and a[2] >= 11.]], "Check filtered content")
    res = checkLogQty("argument value '<'", {'arguments': ["voltage<11."]}, 3*5)
    CHECK(not [l for l in res if not [a for a in l.arguments if a[0] == "voltage" and a[2] < 11.]], "Check filtered content")
    res = checkLogQty("argument value '<='", {'arguments': ["voltage<=11."]}, 3*6)
    CHECK(not [l for l in res if not [a for a in l.arguments if a[0] == "voltage" and a[2] <= 11.]], "Check filtered content")


@declare_test("reader")
def test_reader_multiple_filters():
    """Test the reader multiple filters"""

    session = sslogread.load("./sslogDb")

    expectedQty = 3*2
    res = session.query({'buffer_size_min': 1, "category": "/module4/state"})
    CHECK(len(res) == expectedQty, "Filtering on combined criteria: %d logs expected, %d seen" % (expectedQty, len(res)))
    CHECK(not [l for l in res if l.buffer == None or len(l.buffer) == 0 or l.category != "/module4/state"], "Check filtered content")

    expectedQty = 3*(8+2)
    res = session.query({'buffer_size_min': 1}, {"category": "/module1/state"})
    CHECK(len(res) == expectedQty, "Filtering with union of filters: %d logs expected, %d seen" % (expectedQty, len(res)))
    CHECK(not [l for l in res if (l.buffer == None or len(l.buffer) == 0) and l.category != "/module1/state"], "Check filtered content")


@declare_test("reader")
def test_reader_string_access():
    """Test the reader string access"""

    def checkStringQty(testedName, expectedQty, **kwargs):
        res = session.get_strings(**kwargs)
        CHECK(len(res) == expectedQty, "Filtering strings on %s: %d strings expected, %d seen" % (testedName, expectedQty, len(res)))
        return res

    session = sslogread.load("./sslogDb")

    # Unique flag
    checkStringQty("nothing", 31)
    checkStringQty("category", 9, in_category=True)
    checkStringQty("thread", 3, in_thread=True)
    checkStringQty("format", 11, in_format=True)
    checkStringQty("argument name", 3, in_arg_name=True)
    checkStringQty("argument value", 8, in_arg_value=True)
    checkStringQty("argument unit", 2, in_arg_unit=True)

    # Combined flags
    checkStringQty("category and thread", 12, in_category=True, in_thread=True)
    checkStringQty("format and arg name", 14, in_format=True, in_arg_name=True)
    checkStringQty("arg_value and arg unit", 10, in_arg_value=True, in_arg_unit=True)

    # Flags & pattern
    checkStringQty("category and pattern", 8, in_category=True, pattern="*stat*")
    checkStringQty("arg_name and pattern", 1, in_arg_name=True, pattern="volt*")
    checkStringQty("arg_unit and pattern", 1, in_arg_unit=True, pattern="*A")
