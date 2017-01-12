import sys
import os


class Request:
	def __init__(self, http_method, url, http_version, headers, body):
		self.http_method = http_method
		self.url = url
		self.http_version = http_version
		self.headers = headers
		self.body = body


class IteratorWrapper:
	def __init__(self, collection):
		self.iterator = iter(collection)
		self.over = False
		self.value = self.next()
	
	def next(self):
		if self.over:
			return None
		try:
			self.value = self.iterator.next()
			return self.value
		except StopIteration:
			self.over = True
			return None


def main_setup():
    global project_dir
    global root_dir

    main_script_path = os.path.join(project_dir, "init.py")
    if os.path.exists(main_script_path):
        execfile(main_script_path)

main_setup()
