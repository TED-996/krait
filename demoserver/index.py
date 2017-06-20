import logging
logging.warn("before imports")

import ctrl_index

logging.warn("imported ctrl_index")

import krait

logging.warn("imported krait")

krait.mvc.set_init_ctrl(ctrl_index.IndexController())

logging.warn("init ctrl set")
logging.warn("filename would be {}".format(krait.get_full_path(krait.mvc.init_ctrl.get_view())))
logging.warn("krait site root is {!r}".format(krait.site_root))