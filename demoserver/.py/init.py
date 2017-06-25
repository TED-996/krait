import sqlite3
import os
import atexit
import sys

from krait import config

config.routes = [
    config.Route("POST", url="/comment"),
    config.Route("WEBSOCKET", url="/ws_socket"),
    config.Route()
]


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
