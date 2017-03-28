import sys
import os

config_dir = os.path.join(project_dir, ".config")
krait_py_dir = os.path.join(root_dir, "py")
site_py_dir = os.path.join(project_dir, ".py")

# sys.path.append(config_dir)
sys.path.append(krait_py_dir)
sys.path.append(site_py_dir)

print sys.path

import krait
krait.site_root = project_dir

import mvc

response = None

main_script_path = os.path.join(config_dir, "init.py")
if os.path.exists(main_script_path):
    execfile(main_script_path)

