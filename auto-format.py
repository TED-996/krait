#!/usr/bin/python2
from __future__ import print_function
import subprocess
import os
import sys
import glob


def main():
    git_dir = os.path.abspath(os.path.dirname(__file__))
    directory = os.path.join(git_dir,
                             "src")

    args = parse_args()
    if args["is_from_git"]:
        files = get_git_files(directory, git_dir)
        add_to_git = True
    else:
        files = get_glob_files(directory)
        add_to_git = False

    if files:
        subprocess.check_output(["clang-format.exe", "-i"] + files, shell=True)

        if add_to_git:
            add_files(files, git_dir)

        print("auto-format.py: Code formatted.")
    else:
        print("auto-format.py: No code to format.")


def parse_args():
    def print_usage(do_exit=False):
        print("Usage: auto-format.py [--from-git]")
        if do_exit:
            sys.exit(1)

    is_from_git = False
    for arg in sys.argv[1:]:
        if arg == "--from-git":
            is_from_git = True
        else:
            print("Unrecognized argument {}".format(is_from_git))
            print_usage(do_exit=True)

    return {
        "is_from_git": is_from_git
    }


def get_git_files(directory, git_directory):
    output = subprocess.check_output(["git", "diff", "--name-only", "--cached"],
                                     cwd=git_directory)
    lines = [os.path.normpath(os.path.join(git_directory, l.strip()))
             for l in output.splitlines() if l and not l.isspace()]
    return [f for f in lines if
            (f.endswith(".cpp") or f.endswith(".h")) and ".." not in os.path.relpath(f, directory)]


def get_glob_files(directory):
    return glob.glob(directory + os.path.sep + "*.cpp") + glob.glob(directory + os.path.sep + "*.h")


def add_files(files, directory):
    subprocess.check_output(["git", "add"] + files, cwd=directory)


if __name__ == '__main__':
    main()
