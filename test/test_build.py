#! /usr/bin/env python3

# Local import
from run_tests import *  # Decorators, LOG, CHECK, KPI
from test_base import *  # Test common helpers


# Standard case
@declare_test("build")
def test_build_standard():
    """Standard"""
    build_target("testsslog", "")


# Disabled case
@declare_test("build")
def test_build_disabled():
    """SSLOG_DISABLE=1"""
    build_target("testsslog", test_build_disabled.__doc__)


# Not catching SIG_INT (check)
@declare_test("build")
def test_build_nosigint():
    """SSLOG_NO_CATCH_SIGINT=1"""
    build_target("testsslog", test_build_nosigint.__doc__)


# No signals (check)
@declare_test("build")
def test_build_nosignals():
    """SSLOG_NO_CATCH_SIGNALS=1"""
    build_target("testsslog", test_build_nosignals.__doc__)


# No printf check (check)
@declare_test("build")
def test_build_noprintfcheck():
    """SSLOG_NO_PRINTF_FORMAT_CHECK=1"""
    build_target("testsslog", test_build_noprintfcheck.__doc__)
