import os
import krait


# Configure some globals that can be accessed from anywhere in the program.

# These are used in .view/header.html
project_name = "Krait Demo"
header_items = [
    ("Home", "/"),
    ("DB Access", "/db"),
    ("Your Request", "/http"),
    ("Websockets", "/ws")
]

# This is used in .py/ctrl/db.py
sqlite_db = os.path.join(krait.site_root, ".private", "db")