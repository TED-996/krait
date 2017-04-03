import sys
import os

config_dir = os.path.join(project_dir, ".config")
krait_py_dir = os.path.join(root_dir, "py")
site_py_dir = os.path.join(project_dir, ".py")
site_ctrl_dir = os.path.join(project_dir, ".ctrl")

# sys.path.append(config_dir)
sys.path.append(krait_py_dir)
sys.path.append(site_py_dir)
sys.path.append(site_ctrl_dir)

import krait
krait.site_root = project_dir

import mvc

# If this changes from None, this is the response to send to the client.
response = None

# If this is None, get from source extension.
# If this is "ext/.{extension}", read from mime.types;
#   (since this may change, use krait.get_content_type_ext_marker(ext)
# otherwise this is the content-type to send to the client.
content_type = None

def set_content_type(raw=None, ext=None):
    if raw is not None:
        content_type = raw
    elif ext is not None:
        content_type = "ext/{}".format(ext)
    else:
        content_type = None


main_script_path = os.path.join(config_dir, "init.py")
if os.path.exists(main_script_path):
    execfile(main_script_path)

