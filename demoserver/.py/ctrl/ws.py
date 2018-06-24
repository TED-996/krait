import krait


# This decorator routes requests to "/ws" to this controller.
# This module MUST be imported, otherwise routing to this page won't work.
@krait.mvc.route_ctrl_decorator(url="/ws")
class WsPageController(krait.mvc.CtrlBase):
    def __init__(self):
        self.page_url = "/ws"

    def get_view(self):
        return ".view/ws.html"
