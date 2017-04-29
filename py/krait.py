import urllib
import collections
import os

"""
The site_root (directly from the krait argument)
"""
site_root = ""


class Request(object):
    """
    A named tuple to hold a multipart form entry.
    Fields:
        * data: the entry data
        * name: the entry name in the multipart header
        * filename: the entry filename in the multipart header
        * content_type: the entry content_type in the multipart header
    """
    MultipartFormData = collections.namedtuple("MultipartFormData", ["data", "name", "filename", "content_type"])

    def __init__(self, http_method, url, query_string, http_version, headers, body):
        """
        :param http_method: the HTTP verb in the request. Values: 'GET', 'POST', etc.
        :param url: The URL of the requests, without the query
        :param query_string: The URL query as a string
        :param http_version: the HTTP version; only 'HTTP/1.1' is supported
        :param headers: The headers of the request, as a dict
        :param body: The body of the request
        """
        self.http_method = http_method
        self.url = url
        self.query = Request._get_query(query_string)
        self.http_version = http_version
        self.headers = headers
        self.body = body

    def get_post_form(self):
        """
        Extracts the HTTP form from a POST request
        :return: A dict with the HTTP form data
        """
        return Request._get_query(self.body)

    def get_multipart_form(self):
        """
        Extracts multipart form data from the request body.
        :return: a list of named tuples with (data, name, filename, content_type)
        """
        content_type_multipart = self.headers.get("content-type")
        if content_type_multipart is None:
            print "Error: asked for multipart form, but the request's content-type is missing."
            return None

        value_fields = content_type_multipart.split(';')
        if value_fields[0].strip() != 'multipart/form-data' or len(value_fields) != 2:
            print "Error: asked for multipart form, but the request's content-type is " + content_type_multipart
            return None
        boundary_field = value_fields[1].strip()
        if not boundary_field.startswith("boundary="):
            print "Error: asked for multipart form, but the request's boundary is missing (full value '{}')" \
                .format(content_type_multipart)
            return None

        boundary = boundary_field[9:]
        if len(boundary) >= 2 and boundary[0] == '"' and boundary[-1] == '"':
            boundary = boundary[1:-1]
        boundary = "--" + boundary
        boundary_next = "\r\n" + boundary
        # print "found  multipart form data with boundary", boundary

        result = []
        found_idx = 0 if self.body.startswith(boundary) else self.body.find(boundary_next)
        while found_idx != -1 and self.body[found_idx + len(boundary_next):found_idx + len(boundary_next) + 2] != "--":
            end_idx = self.body.find(boundary_next, found_idx + len(boundary))
            result.append(self._get_multipart_item(found_idx + 2, end_idx))
            found_idx = end_idx

        return result

    def _get_multipart_item(self, start_idx, end_idx):
        """
        Extracts a multipart entry from the request body
        :param start_idx: The start index in the body of the multipart entry
        :param end_idx: The end index in the body of the multipart entry
        :return: An object of type Request.MultipartFormData with the extracted information
        """
        part_headers = self.body.find("\r\n\r\n", start_idx)
        part_headers = self.body[self.body.find("\r\n", start_idx) + 2:part_headers]
        data_start = part_headers + 4

        part_data = self.body[data_start:end_idx]
        part_name = None
        part_filename = None
        part_content_type = "text/plain"
        for header_line in part_headers.split("\r\n"):
            if header_line == "" or header_line.isspace():
                continue

            colon_idx = header_line.index(':')
            header_name = header_line[:colon_idx].strip().lower()
            header_value = header_line[colon_idx + 1:].strip()

            if header_name == "content-type":
                part_content_type = header_value
            elif header_name == "content-disposition":
                disp_items = header_value.split(";")
                if disp_items[0].strip() != "form-data":
                    continue  # next header line

                for item in disp_items:
                    item_strip = item.strip()
                    if item_strip == "form-data":
                        continue  # next item

                    if item_strip.startswith("name="):
                        part_name = item_strip[5:]
                        if len(part_name) >= 2 and part_name[0] == '"' and part_name[-1] == '"':
                            part_name = part_name[1:-1]
                    elif item_strip.startswith("filename="):
                        part_filename = item_strip[9:]
                        if len(part_filename) >= 2 and part_filename[0] == '"' and part_filename[-1] == '"':
                            part_filename = part_filename[1:-1]

        # after processing all headers
        return Request.MultipartFormData(part_data, part_name, part_filename, part_content_type)

    @staticmethod
    def _get_query(query_string):
        """
        Extracts a dict from a string in the URL query format
        :param query_string: a string in the URL query format
        :return: a dict with the keys and values from the input
        """
        if query_string is None or query_string == "":
            return dict()

        result = dict()
        for field in query_string.split('&'):
            items = field.split('=')
            if len(items) == 1:
                result[urllib.unquote_plus(items[0])] = ""
            else:
                result[urllib.unquote_plus(items[0])] = urllib.unquote_plus(items[1])
        return result

    def __str__(self):
        """
        Convert a Request to a string according to the HTTP standard
        :return: The HTTP data for the request
        """
        return "{} {}{} {}\r\n{}\r\n{}".format(
            self.http_method,
            self.url,
            ("?" + "&".join(
                ["{}={}".format(k, v) for k, v in self.query.iteritems()])) if self.query is not dict() else "",
            self.http_version,
            "".join(["{}: {}\r\n".format(k, v) for k, v in self.headers.iteritems()]),
            self.body
        )


class Response(object):
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
        """
        :param http_version: 'HTTP/1.1', no other values are supported.
        :param status_code: The HTTP status code (for example, 200 or 404)
        :param headers: Response headers, as a tuple array
        :param body:
        """
        self.http_version = http_version
        self.status_code = status_code
        self.headers = headers
        self.body = body

    def __str__(self):
        """
        Convert the Response object according to the HTTP standard
        :return: The HTTP data for the response
        """
        return "{} {} {}\r\n{}\r\n{}".format(
            self.http_version,
            self.status_code,
            self.status_reasons.get(self.status_code, "Unknown"),
            "".join(["{}: {}\r\n".format(name, value) for name, value in self.headers]),
            self.body
        )


class ResponseNotFound(Response):
    """
    Response returning 404 Not Found
    """
    def __init__(self, headers=None):
        """
        :param headers: extra headers to send with the response, as a tuple list
        """
        super(ResponseNotFound, self).__init__("HTTP/1.1", 404, headers or [],
                                               "<html><head><title>404 Not Found</title></head>"
                                               "<body><h1>404 Not Found</h1></body></html>")


class ResponseRedirect(Response):
    """
    Response returning a 302 Found redirect
    """
    def __init__(self, destination, headers=None):
        """
        :param destination: the URL to which to redirect the client
        :param headers: extra headers to send with the response, as a tuple list
        """
        headers = headers or []
        headers.append(("Location", destination))
        super(ResponseRedirect, self).__init__("HTTP/1.1", 302, headers, "")


class ResponseBadRequest(Response):
    """
    Response returning 400 Bad Request
    """
    def __init__(self, headers=None):
        """
        :param headers: extra headers to send with the response, as a tuple list
        """
        super(ResponseBadRequest, self).__init__("HTTP/1.1", 400, headers or [],
                                               "<html><head><title>400 Bad Request</title></head>"
                                               "<body><h1>404 Bad Request</h1></body></html>")


class IteratorWrapper(object):
    """
    Wraps an iterator in a way useful to the classic for(init; condition; increment) paradigm
    """
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


def get_full_path(filename):
    """
    Converts a filename relative to the site-root to its full path
    Note that this is not necessarily absolute, but is derived from the krait argument.
    :param filename: a filename relative to the site-root (the krait argument)
    :return: os.path.join(site_root, filename)
    """
    global site_root
    return os.path.join(site_root, filename)


# An object of type krait.Request, set before any responses are handled
request = None
# An object of type krait.Response (or its subclasses); leave None for usual behavior, set to override.
response = None


# If this is None, get from source extension.
# If this is "ext/.{extension}", read from mime.types;
#   (since this may change, use krait.set_content_type(ext="your-ext")
# otherwise this is the content-type to send to the client.
content_type = None

# Extra response headers to set without overriding the entire response, as a dict
extra_headers = None


def set_content_type(raw=None, ext=None):
    """
    Sets the content-type to a custom value
    Use when the routable page's extension is not the extension of the final content.
    :param raw: full content-type (e.g 'application/json')
    :param ext: extension from which to derive the content-type (e.g. 'json')
    """
    global content_type
    if raw is not None:
        content_type = raw
    elif ext is not None:
        content_type = "ext/{}".format(ext)
    else:
        content_type = None
