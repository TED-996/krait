import sys
import os

def main_setup():
    global project_dir
    global root_dir

    main_script_path = os.path.join(project_dir, "init.py")
    if os.path.exists(main_script_path):
        execfile(main_script_path)
    
main_setup()
