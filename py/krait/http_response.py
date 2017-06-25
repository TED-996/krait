import urllib

class Response(object):
    """
    Wraps a HTTP response.
    """

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
        headers.append(("Location", urllib.quote_plus(destination)))
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
