import krait
import sqlite3

post_form = krait.request.get_post_form()
name = post_form["name"]
message = post_form["text"]

krait.response = krait.Response("HTTP/1.1", 302, [("Location", "/db")], "")

conn = sqlite3.connect(sqlite_db)
c = conn.cursor()

c.execute("insert into messages values(?, ?)", (name, message))

conn.commit()
conn.close()
