import krait
import sqlite3
import datetime

from ctrl import db

post_form = krait.request.get_post_form()
name = post_form["name"]
message = post_form["text"]

krait.response = krait.ResponseRedirect("/db")

conn = sqlite3.connect(db.sqlite_db)
c = conn.cursor()

c.execute("insert into messages values(?, ?)", (name, message))

conn.commit()
conn.close()

new_cookie = krait.cookie.Cookie("comment_count", str(int(krait.cookie.get_cookie("comment_count", "0")) + 1))
new_cookie.set_expires(datetime.datetime.utcnow() + datetime.timedelta(minutes=1))
new_cookie.set_http_only(True)

krait.cookie.set_cookie(new_cookie)
