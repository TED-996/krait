import sys
import os
import urllib


class Request:
    def __init__(self, http_method, url, http_version, headers, body):
        self.http_method = http_method
        self.url = url
        self.http_version = http_version
        self.headers = headers
        self.body = body
    
    def get_post_form(self):
        result = dict()
        fields = self.body.split('&')
        for field in fields:
            items = field.split('=')
            if len(items) == 1:
                result[urllib.unquote(items[0])] = ""
            else:
                result[urllib.unquote(items[0])] = urllib.unquote(items[1])
        return result


class Response:
    status_reasons = {
        200: "OK",
        302: "Found",
        304: "Not Modified",
        400: "Bad Request",
        401: "Unauthorized",
        403: "Forbidden",
        404: "Not Found",
        500: "Internal Server Error"
    }
    
    def __init__(self, http_version, status_code, headers, body):
        self.http_version = http_version
        self.status_code = status_code
        self.headers = headers
        self.body = body
    
    def __str__(self):
        return "{} {} {}\r\n{}\r\n{}".format(
            self.http_version,
            self.status_code,
            self.status_reasons.get(self.status_code, "Unknown"),
            "".join(["{}: {}\r\n".format(name, value) for name, value in self.headers]),
            self.body
        )


response = None


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



main_script_path = os.path.join(project_dir, ".config", "init.py")
if os.path.exists(main_script_path):
    execfile(main_script_path)

