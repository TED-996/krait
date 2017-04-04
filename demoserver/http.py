import ctrl_http
import mvc


mvc.set_init_ctrl(ctrl_http.HttpController(request))
