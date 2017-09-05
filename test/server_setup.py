import fabric.api as fab
import fabric.contrib.project as fab_project

import posixpath as rpath
import os

host = "localhost"
port = 8090
remote_path = "~/vs_projects/krait"

built = False


def setup(skip_build=False):
    if not skip_build:
        build()


def build():
    print "Building."

    global built
    if built:
        return

    with fab.cd(remote_path):
        fab.run("cmake .")
        fab.run("mkdir -p build")
        fab.run("make -j5")

    built = True


def start_demoserver():
    # Remove build from demoserver tests. Run it manually.
    # build()
    stop_demoserver()

    with fab.cd(remote_path):
        fab.sudo("build/krait-cmdr start -p {} {}".format(port, rpath.join(remote_path, "demoserver")))

    return "http://{}:{}".format(host, port)


def stop_demoserver(force=False):
    with fab.cd(remote_path), fab.settings(warn_only=(not force)):
        result = fab.sudo("build/krait-cmdr stop")

    if not force and result.failed:
        print "Server already stopped, not a problem."


def watch():
    with fab.cd(remote_path):
        fab.sudo("build/krait-cmdr watch")


def upload():
    excludes = [".idea", ".git", "bin", "obj", ".gitignore", "LICENSE", "krait.vcxproj.user", "krait.vcxproj"]
    local_path = to_rsync_local_path(os.path.abspath(os.path.dirname(os.path.dirname(__file__))))
    fab_project.rsync_project(remote_path, local_path + "/", exclude=excludes,
                              ssh_opts="-o StrictHostKeyChecking=no")


def to_rsync_local_path(path):
    if os.name != "nt":
        return path

    path = os.path.abspath(path)
    drive, path = os.path.splitdrive(path)
    drive = drive[:1].lower()

    if path.endswith("/") or path.endswith("\\"):
        path = path[:-1]

    folders = []
    while True:
        path, folder = os.path.split(path)

        if folder != "":
            folders.append(folder)
        else:
            break

    folders.reverse()

    return rpath.join("/cygdrive", drive, *folders)  # Don't know how to run rsync NOT on cygwin yet.