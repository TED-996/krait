import sys
import os

config_dir = os.path.join(project_dir, ".config")

sys.path.append(config_dir)


import urllib


class Request:
    def __init__(self, http_method, url, query_string, http_version, headers, body):
        self.http_method = http_method
        self.url = url
        self.query = Request._get_query(query_string)
        self.http_version = http_version
        self.headers = headers
        self.body = body
        print query_string
        print self.query
    
    def get_post_form(self):
        return Request._get_query(self.body)
    
    @staticmethod
    def _get_query(query_string):
        if query_string is None or query_string == "":
            return dict()
        
        result=dict()
        for field in query_string.split('&'):
            items = field.split('=')
            if len(items) == 1:
                result[urllib.unquote_plus(items[0])] = ""
            else:
                result[urllib.unquote_plus(items[0])] = urllib.unquote_plus(items[1])
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


main_script_path = os.path.join(config_dir, "init.py")
if os.path.exists(main_script_path):
    execfile(main_script_path)

