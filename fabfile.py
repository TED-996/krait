import fabric.api as fab
from test import run_test


def test():
    run_test.run_tests()


def deploy():
    raise NotImplemented("deploy()")


def commit():
    raise NotImplemented("commit()")
