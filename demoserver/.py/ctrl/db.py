import sqlite3
import krait

import global_config


# This decorator routes requests to "/db" to this controller.
# This module MUST be imported, otherwise routing to this page won't work.
@krait.mvc.route_ctrl_decorator(url="/db")
class DbController(krait.mvc.CtrlBase):
    def __init__(self):
        super(DbController, self).__init__()

        self.page_url = "/db"
        self.messages = None
        self.load_db(global_config.sqlite_db)
        self.nr_comments = int(krait.cookie.get_cookie("comment_count", 0))

    def load_db(self, db_filename):
        conn = sqlite3.connect(db_filename)
        c = conn.cursor()

        c.execute("select * from messages")
        self.messages = c.fetchall()

        conn.close()

    def get_view(self):
        return ".view/db.html"
