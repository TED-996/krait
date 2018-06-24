import urllib

class Response(object):
    """
    Represents an HTTP response.
    Set :obj:`krait.response` with an instance of this variable (or its subclasses) to override the HTTP response.

    Args:
        http_version (str): 'HTTP/1.1', no other values are supported.
        status_code (int): The HTTP status code (for example, 200 or 404).
        headers (list of (str, str)): The response headers.
        body: The response body.

    Attributes:
        http_version (str): The HTTP version of the response.
        status_code (int): The HTTP status code.
        headers (list of (str, str)): The response headers.
        body: The response body.
    """

    _status_reasons = {
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

        if self.http_version != "HTTP/1.1":
            raise ValueError("Only HTTP/1.1 is supported.")

    def __str__(self):
        """
        Convert the Response object according to the HTTP standard.

        Returns:
            str: The HTTP data for the response.
        """

        return "{} {} {}\r\n{}\r\n{}".format(
            self.http_version,
            self.status_code,
            self._status_reasons.get(self.status_code, "Unknown"),
            "".join(["{}: {}\r\n".format(name, value) for name, value in self.headers]),
            self.body
        )


class ResponseNotFound(Response):
    """
    Response returning a 404 Not Found

    Args:
        headers (list of (str, str), optional): Extra headers to send with the response.
    """

    def __init__(self, headers=None):
        super(ResponseNotFound, self).__init__("HTTP/1.1", 404, headers or [],
                                               "<html><head><title>404 Not Found</title></head>"
                                               "<body><h1>404 Not Found</h1></body></html>")


class ResponseRedirect(Response):
    """
    Response returning a 302 Found redirect.

    Args:
        destination (str): The URL on which to redirect the client.
        headers (list of (str, str), optional): Extra headers to send with the response.
    """

    def __init__(self, destination, headers=None):
        headers = headers or []
        headers.append(("Location", destination))
        super(ResponseRedirect, self).__init__("HTTP/1.1", 302, headers, "")


class ResponseBadRequest(Response):
    """
    Response returning 400 Bad Request

    Args:
        headers (list of (str, str), optional): Extra headers to send with the response.
    """

    def __init__(self, headers=None):
        super(ResponseBadRequest, self).__init__("HTTP/1.1", 400, headers or [],
                                                 "<html><head><title>400 Bad Request</title></head>"
                                                 "<body><h1>404 Bad Request</h1></body></html>")
