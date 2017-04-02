class HttpController(object):
    def __init__(self, request):
        super(HttpController, self).__init__()

        self.page_url = "/http.html"
        self.request_str = "{} {}{} {}\n{}\n\n{}".format(
            request.http_method,
            request.url,
            "" if len(request.query) == 0 else "?" + "&".
                join(["{}={}".format(key, value) for key, value in request.query.iteritems()]),
            request.http_version,
            "\n".join(["{}: {}".format(key, value) for key, value in request.headers.iteritems()]),
            request.body
        )

    def get_view(self):
        return ".view/http.html"
