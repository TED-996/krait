import krait

class HttpController(object):
    def __init__(self):
        super(HttpController, self).__init__()

        self.page_url = "/http"
        self.request_str = str(krait.request).replace("\r\n", "\n");

    def get_view(self):
        return ".view/http.html"
