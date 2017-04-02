import sqlite3


class DbController(object):
    def __init__(self, db_filename):
        super(DbController, self).__init__()

        self.page_url = "/db.html"
        self.messages = None
        self.load_db(db_filename)

    def load_db(self, db_filename):
        conn = sqlite3.connect(db_filename)
        c = conn.cursor()

        c.execute("select * from messages")
        self.messages = c.fetchall()

        conn.close()

    def get_view(self):
        return ".view/db.html"
