import sqlite3
import os

project_name = "Krait Demo"
header_items = [
    ("Home", "/"),
    ("DB Access", "/db.html"),
    ("Your Request", "/http.html")
]
sqlite_db = os.path.join(project_dir, ".private", "db")
