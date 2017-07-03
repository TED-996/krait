import fabric.api as fab
import os

from test import run_test
from test import server_setup


def test():
    run_test.run_tests()


def test_ds():
    run_test.run_tests(["demoserver"])


def test_build():
    run_test.run_tests(["build"])


def test_docs():
    run_test.run_tests(["docs"])


def deploy():
    server_setup.upload()


def build():
    server_setup.build()


def commit():
    raise NotImplemented("commit()")
