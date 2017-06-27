import time
import random


class IndexController(object):
    def __init__(self):
        super(IndexController, self).__init__()

        self.page_url = "/"
        self.alerts_exist = random.randrange(2)
        self.suffixes = {
            1: "st",
            2: "nd",
            3: "rd"
        }
        self.time_str = time.strftime("%x")
        self.subh_left = [
            ("New car smell", "Krait is still new!"),
            ("Support budding developers", "there's lots of love here"),
            ("Bug hunting!", "Spot a bug? Let me know!")
        ]
        self.subh_right = [
            ("Python!", "Why choose PHP when you can choose Python?"),
            ("Seriously, Python.", "it's just an awesome language"),
            ("Python is a selling point", "Look at it!")
        ]

    def get_view(self):
        return ".view/index.html"
