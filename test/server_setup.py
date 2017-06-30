import fabric.api as fab
import posixpath as rpath

host = "localhost"
port = 8090
remote_path = "~/vs_projects/krait"

built = False


def setup():
    build()


def build():
    global built
    if built:
        return

    with fab.cd(remote_path):
        fab.run("cmake .")
        fab.run("make -j5")

    built = True


def start_demoserver():
    build()
    stop_demoserver()

    with fab.cd(remote_path):
        fab.run("build/krait-cmdr start -p {} {}".format(port, rpath.join(remote_path, "demoserver")))

    return "http://{}:{}".format(host, port)


def stop_demoserver(force=False):
    with fab.cd(remote_path), fab.settings(warn_only=(not force)):
        result = fab.run("build/krait-cmdr stop")

    if not force and result.failed:
        print "Server already stopped, not a problem."
