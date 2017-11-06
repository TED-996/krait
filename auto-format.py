#!/usr/bin/python2
import subprocess
import os
import glob


def main():
    directory = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                             "src")
    files = glob.glob(directory + os.path.sep + "*.cpp") + glob.glob(directory + os.path.sep + "*.h")
    if not files:
        raise RuntimeError("No .cpp/.h files found in src.")
    subprocess.check_call(["clang-format.exe", "-i"] + files, shell=True)


if __name__ == '__main__':
    main()
