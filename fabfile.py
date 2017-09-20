import fabric.api as fab
import os
from fabric.network import ssh

from test import run_test
from test import server_setup


# ssh.util.log_to_file("paramiko.log", 10)


def add_key(filename):
    if os.path.exists(filename):
        fab.env.key_filename = filename
    else:
        print "Private key file {} does not exist, you will probably be prompted for a password."\
            .format(filename)
        print "Add a file {} with the private key in OpenSSH format. It will not be committed."\
            .format(filename)
        print "Due to a bug, this may still not work."


# add_key(os.path.join(os.path.dirname(__file__), "fab-key.nocommit"))


def test():
    deploy()
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


def start(port_spec):
    http_port_str, _, https_port_str = port_spec.partition("/")

    server_setup.http_port = None if not http_port_str else int(http_port_str)
    server_setup.https_port = None if not https_port_str else int(https_port_str)

    server_setup.start_demoserver()


def stop():
    server_setup.stop_demoserver()


def watch():
    server_setup.watch()


def commit():
    raise NotImplemented("commit()")
