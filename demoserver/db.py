import ctrl_db
import krait

krait.mvc.set_init_ctrl(ctrl_db.DbController(sqlite_db))
