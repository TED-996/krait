import krait

class HttpController(object):
    def __init__(self):
        super(HttpController, self).__init__()

        self.page_url = "/http"
        self.request_str = "{} {}{} {}\n{}\n\n{}".format(
            krait.request.http_method,
            krait.request.url,
            "" if len(krait.request.query) == 0 else "?" + "&".
                join(["{}={}".format(key, value) for key, value in krait.request.query.iteritems()]),
            krait.request.http_version,
            "\n".join(["{}: {}".format(key, value) for key, value in krait.request.headers.iteritems()]),
            krait.request.body
        )

    def get_view(self):
        return ".view/http.html"
