import ctrl_http
import krait


krait.mvc.set_init_ctrl(ctrl_http.HttpController(krait.request))
