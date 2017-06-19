import threading

class WebsocketsRequest(object):
    """
    Represents a Websockets request.
    Contains additional information extracted from a Upgrade: websocket request.
    """
    def __init__(self, http_request):
        self.protocols = WebsocketsRequest.extract_protocols(http_request)

    @staticmethod
    def extract_protocols(http_request):
        if http_request.headers.get("upgrade") == "websocket":
            protocols = http_request.headers.get("sec-websocket-protocol")
            if protocols is not None:
                return protocols.split(", ")
            else:
                return None
        else:
            return None


class WebsocketsResponse(object):
    def __init__(self, controller, protocol=None):
        self.protocol = protocol
        self.controller = controller


request = None
response = None


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
        self._exit_event = threading.Event()
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

    def on_stop(self):
        """
        Sets a flag that tells the controller thread to shut down.
        Called by Krait when the WebSockets connection is closing.
        """
        self._exit_event.set()

    def should_stop(self):
        """
        Returns True if the shutdown event has been set, or False otherwise.
        Call this periodically from the controller thread to check if you should shut down.
        """
        return self._exit_event.is_set()

    def wait_stopped(self, timeout=None):
        """
        Joins the controller thread, with an optional timeout.
        Called by Krait until the thread has shut down.
        :return: True if the thread has shut down, or False otherwise.
        """
        if not self._thread.is_alive():
            return True

        self._thread.join(float(timeout))

        return not self._thread.is_alive()
