import krait
from ctrl import http

krait.mvc.set_init_ctrl(http.HttpController(krait.request))
