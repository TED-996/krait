import threading


class WebsocketsCtrlBase(object):
    """
    Base class responsible for handling Websockets communication.
    Subclass it to add behavior.
    """
    def __init__(self, use_in_message_queue):
        self._out_message_queue = []
        self._in_message_queue = [] if use_in_message_queue else None

        self._in_message_lock = threading.Lock()
        self._out_message_lock = threading.Lock()
        # noinspection PyArgumentList
        self._thread = threading.Thread(target=self.on_thread_start)

    def on_start(self):
        """
        Called by Krait when the controller is ready to start. This is running on the main thread.
        Do not perform expensive initialization here.
        """
        self._thread.start()

    # noinspection PyMethodMayBeStatic
    def on_thread_start(self):
        """
        The thread's target.
        Started by the controller's on_start() method.
        Override this to add behavior to your controller.
        """
        pass

    def on_in_message(self, message):
        """
        Called by Krait (indirectly) when a new message arrives from the client.
        By default adds the message to the message queue.
        Override this if the controller is not configured to use messages queues.
        :param message: the new message
        """
        if self._in_message_queue is not None:
            self._in_message_queue.append(message)

    def on_in_message_internal(self, message):
        """
        Called by Krait (directly) when a new message arrives from the client.
        This only acquires the lock, then calls on_in_message(message).
        :param message: the new message
        """
        with self._in_message_lock:
            self.on_in_message(message)

    def pop_in_message(self):
        """
        Returns the first message in the input queue, popping it, if it exists, or None otherwise.
        Call this to get messages from the client.
        Do not call this if the controller doesn't use input message queues.
        :return: The value of the first message, or None
        """
        if self._in_message_queue is None:
            raise RuntimeError("WebsocketsCtrlBase: Input message queueing disabled, but tried calling pop_in_message.")

        with self._in_message_lock:
            if len(self._in_message_queue) == 0:
                return None
            else:
                return self._in_message_queue.pop(0)

    def push_out_message(self, message):
        """
        Call this to send a message. This adds it to the queue; Krait will watch the message queue and send it.
        :param message: The message to send.
        """
        with self._out_message_lock:
            self._out_message_queue.append(message)

    def pop_out_message(self):
        """
        Returns the first message in the output queue, popping it, if it exists, or None otherwise.
        Called by Krait to check on messages to send.
        :return: The value of the first message, or None
        """
        with self._out_message_lock:
            if len(self._out_message_queue) == 0:
                return None
            else:
                return self._out_message_queue.pop(0)


websockets_ctrl = None


def set_websockets_ctrl(ctrl):
    global websockets_ctrl
    websockets_ctrl = ctrl
