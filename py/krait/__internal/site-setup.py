import sys
import os


global project_dir


site_py_dir = os.path.join(project_dir, ".py")

sys.path.append(site_py_dir)

import krait
from krait.__internal import compile_imports
krait.site_root = project_dir
compile_imports.pre_clean()
compile_imports.register()

main_script_path = os.path.join(site_py_dir, "init.py")
if os.path.exists(main_script_path):
    execfile(main_script_path)

from krait.__internal import mvc_compiler
mvc_compiler.compile_routes(krait.config.routes)