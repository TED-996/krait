import sqlite3
import os
import atexit
import sys

project_name = "Krait Demo"
header_items = [
    ("Home", "/"),
    ("DB Access", "/db"),
    ("Your Request", "/http"),
    ("Websockets", "/ws")
]
sqlite_db = os.path.join(project_dir, ".private", "db")


def exit_function():
    print "atexit test: Demoserver process shutting down."
    sys.stdout.flush()

atexit.register(exit_function)
print "atexit registered"
