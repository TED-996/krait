import krait
import mvc


class WsPageController(mvc.CtrlBase):
    def __init__(self):
        self.page_url = "/ws"

    def get_view(self):
        return ".view/ws.html"
