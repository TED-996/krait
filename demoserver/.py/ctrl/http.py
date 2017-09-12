import krait


# This decorator routes requests to "/http" AND "/http+<anything>" to this controller.
# This module MUST be imported, otherwise routing to this page won't work.
@krait.mvc.route_ctrl_decorator(url="/http")
@krait.mvc.route_ctrl_decorator(regex="/http\\+.*")
class HttpController(krait.mvc.CtrlBase):
    def __init__(self):
        super(HttpController, self).__init__()

        self.page_url = "/http"
        self.request_str = str(krait.request).replace("\r\n", "\n")

    def get_view(self):
        return ".view/http.html"
