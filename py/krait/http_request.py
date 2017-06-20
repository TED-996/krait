import collections
import urllib

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
