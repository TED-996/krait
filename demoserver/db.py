import krait
from ctrl import db

krait.mvc.set_init_ctrl(db.DbController(sqlite_db))
