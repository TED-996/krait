import urllib
import collections


class Request:
    MultipartFormData = collections.namedtuple("MultipartFormData", ["data", "name", "filename", "content_type"])

    def __init__(self, http_method, url, query_string, http_version, headers, body):
        self.http_method = http_method
        self.url = url
        self.query = Request._get_query(query_string)
        self.http_version = http_version
        self.headers = headers
        self.body = body
        print query_string
        print self.query

    '''
    Returns a dict in the format {"name": "value", ... }
    '''
    def get_post_form(self):
        return Request._get_query(self.body)

    '''
    Returns an array of named tuples with the following fields:
        * data: string, field data
        * name: string, field name
        * filename: string, filename of form part
        * content_type: content type of form part
    '''
    def get_multipart_form(self):
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
            print "Error: asked for multipart form, but the request's boundary is missing (full value '{}')"\
                .format(content_type_multipart)
            return None

        boundary = boundary_field[9:]
        if len(boundary) >= 2 and boundary[0] == '"' and boundary[-1] == '"':
            boundary = boundary[1:-1]
        boundary_next = "\r\n" + boundary

        result = []
        found_idx = 0 if self.body.startswith(boundary) else self.body.find(boundary_next)
        while found_idx != -1 and self.body[found_idx + len(boundary_next):found_idx + len(boundary_next) + 2] != "--":
            end_idx = self.body.find(boundary_next, found_idx + len(boundary))
            result.append(self._get_multipart_item(found_idx + 2, end_idx))
            found_idx = end_idx

        return result

    def _get_multipart_item(self, start_idx, end_idx):
        extra_headers_end = self.body.find("\r\n\r\n", start_idx)
        extra_headers = self.body[self.body.find("\r\n", start_idx) + 2:extra_headers_end]
        data_start = extra_headers_end + 4

        data = self.body[data_start:end_idx]
        name = None
        filename = None
        content_type = "text/plain"
        for header_line in extra_headers.split("\r\n"):
            if header_line == "" or header_line.isspace():
                continue

            colon_idx = header_line.index(':')
            header_name = header_line[:colon_idx].strip().lower()
            header_value = header_line[colon_idx + 1:].strip()

            if header_name == "content-type":
                content_type = header_value
            elif header_name == "content-disposition":
                disp_items = header_value.split(";")
                if disp_items[0].strip() != "form-data":
                    continue  # next header line

                for item in disp_items:
                    item_strip = item.strip()
                    if item_strip == "form-data":
                        continue  # next item

                    if item_strip.startswith("name="):
                        name = item_strip[5:]
                        if len(name) >= 2 and name[0] == '"' and name[-1] == '"':
                            name = name[1:-1]
                    elif item_strip.startswith("filename="):
                        filename = item_strip[9:]
                        if len(filename) >= 2 and filename[0] == '"' and filename[-1] == '"':
                            filename = filename[1:-1]

        # after processing all headers
        return Request.MultipartFormData(data, name, filename, content_type)

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
