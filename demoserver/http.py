import ctrl_http
import mvc
import krait


mvc.set_init_ctrl(ctrl_http.HttpController(krait.request))
