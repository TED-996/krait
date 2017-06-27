import time
import random
import string

from krait import websockets


class WsPingpongController(websockets.WebsocketsCtrlBase):
    def __init__(self):
        super(WsPingpongController, self).__init__(True)

    def on_thread_start(self):
        rand_chars = string.ascii_letters + string.digits
        while not self.should_stop():
            in_msg = self.pop_in_message()

            if in_msg is not None and not in_msg.startswith("PONG"):
                self.push_out_message("PONG: " + in_msg)

            if random.randrange(0, 50) == 0:
                ping_str = "".join(random.choice(rand_chars) for _ in range(random.randrange(5, 10)))

                self.push_out_message("PING: " + ping_str)

            time.sleep(0.1)

        print "Closing..."
