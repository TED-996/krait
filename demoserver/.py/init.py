import sqlite3  # Import some modules at setup time (for performance)
import os
import atexit
import sys

import krait
from krait import config
from krait import mvc

from ctrl import db

# ==========================================
# Standard configuration (routing & caching)
# ==========================================


# First, configure the routes
# MVC routes are configured by the krait.mvc.route_ctrl_decorator decorators on the controller classes.
# Automatically import these controllers so that they add themselves as routes.
mvc.import_ctrls_from("ctrl")

config.routes.extend([
    # MVC routes have already been added.
    # Add special URLs
    config.Route("POST", url="/comment"),  # POSTs must be explicitly routed
    config.Route("WEBSOCKET", url="/ws_socket"),  # Websocket requests must also be explicitly routed.
    config.Route()  # This final route is a default route: all GETs will resolve to files with the same name.
])

# Configure the caching. Caching uses regexes, so some characters have a special meaning (notably, ``.``)

# Allow JS, CSS and .map (if they exist) files to be cached by any clients.
config.cache_public = [".*\.js", ".*\.css", ".*\.map"]

# config.cache_no_store = ["*.py"]  # Redundant. Python scripts are assumed to be dynamic.

# Allow JS and CSS resources to be cached for a longer time.
config.cache_long_term = [".*\.js", ".*\.css"]

# Choose the caching max age (in seconds)
config.cache_max_age_default = 6 * 60  # 6 minutes
config.cache_max_age_long_term = 24 * 60 * 60  # 24 hours

# Finally, configure SSL

# Set SSL certificate
config.ssl_certificate_path = os.path.join(krait.site_root, ".private", "ssl", "cert.pem")
# Set SSL private key
config.ssl_private_key_path = os.path.join(krait.site_root, ".private", "ssl", "key.pem")
# Then the SSL key passphrase. We read this from a file, as this file (init.py) may be committed.
with open(os.path.join(krait.site_root, ".private", "ssl", "pk_passphrase")) as f:
    config.ssl_key_passphrase = f.read()


# ==============================================
# Non-standard configuration (globals & cleanup)
# ==============================================

# Configure globals used in the program
project_name = "Krait Demo"
header_items = [
    ("Home", "/"),
    ("DB Access", "/db"),
    ("Your Request", "/http"),
    ("Websockets", "/ws")
]
db.sqlite_db = os.path.join(krait.site_root, ".private", "db")


# Choose a function to be run when a process shuts down (either the server, or a worker) and register it.
def exit_function():
    print "atexit test: Demoserver process shutting down."
    sys.stdout.flush()

atexit.register(exit_function)
print "atexit registered"

krait._emit("test");
krait._emit_raw("test raw");

print "wtf"
