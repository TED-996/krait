import krait


class WsPageController(krait.mvc.CtrlBase):
    def __init__(self):
        self.page_url = "/ws"

    def get_view(self):
        return ".view/ws.html"
