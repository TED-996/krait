import krait
import sqlite3
import cookie
import datetime

import krait_utils
reload(krait_utils)
print "reloaded"


post_form = krait.request.get_post_form()
name = post_form["name"]
message = post_form["text"]

krait.response = krait.Response("HTTP/1.1", 302, [("Location", "/db")], "")

conn = sqlite3.connect(sqlite_db)
c = conn.cursor()

c.execute("insert into messages values(?, ?)", (name, message))

conn.commit()
conn.close()

new_cookie = cookie.Cookie("comment_count", int(cookie.get_cookie("comment_count", "0")) + 1)
new_cookie.set_expires(datetime.datetime.utcnow() + datetime.timedelta(minutes=1))
new_cookie.set_http_only(True)

cookie.set_cookie(new_cookie)
