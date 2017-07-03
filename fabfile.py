import fabric.api as fab
from test import run_test


def test():
    run_test.run_tests()


def test_ds():
    run_test.run_tests(["demoserver"])


def test_build():
    run_test.run_tests(["build"])


def test_docs():
    run_test.run_tests(["docs"])


def deploy():
    raise NotImplemented("deploy()")


def commit():
    raise NotImplemented("commit()")
