import sys
import os


class Request:
	def __init__(self, http_method, url, http_version, headers, body):
		self.http_method = http_method
		self.url = url
		self.http_version = http_version
		self.headers = headers
		self.body = body


def main_setup():
    global project_dir
    global root_dir

    main_script_path = os.path.join(project_dir, "init.py")
    if os.path.exists(main_script_path):
        execfile(main_script_path)

main_setup()
