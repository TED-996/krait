import ctrl_db
import mvc

mvc.set_init_ctrl(ctrl_db.DbController(sqlite_db))
