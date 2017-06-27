"""
This module interfaces Krait's WebSocket support with your Python backend.
WebSocket controllers are implemented as two threads, one running the networking in C++ (the main thread),
and the other running the behaviour in Python (the handler thread). Your backend implements the second thread,
by subclassing :class:`WebsocketsCtrlBase` and overriding the relevant methods.

The WebSocket protocol requires three components:

1. A page contains JavaScript code that requests a WebSocket connection to a specific URL, by requesting
   a WebSocket upgrade (change of protocols) through an otherwise normal GET request. This doesn't have to be
   Javascript running in a browser, but usually is.
2. A server-side script to handle a GET request on that URL and accept or deny the WebSocket upgrade
3. A WebSocket controller that runs in a separate thread, handles incoming messages and sends messages itself.

See the websockets tutorial (TODO) for a more detailed explanation.

Usage
=====

1. Create a page with the relevant JavaScript that makes a WebSocket upgrade request to a separate URL,
   then communicates with the server on the new channel.
2. Create a WebSockets controller by subclassing :class:`WebsocketsCtrlBase` and overriding, at least,
   :obj:`WebsocketsCtrlBase.on_thread_start`. This method should contain a loop that runs while
   :obj:`WebsocketsCtrlBase.should_stop` on ``self`` is false.
3. Create the route target Python script at the URL requested by the JavaScript client. Inspect
   :obj:`krait.websockets.request`, and if it is a valid WebSocket request (not ``None`` and :obj:`protocols`
   contains the expected subprotocol), set :obj:`krait.websockets.response` to a new :class:`WebsocketsResponse`
   with a new instance of your controller and, optionally, the subprotocol that you chose.

Reference
=========
"""
import threading

class WebsocketsRequest(object):
    """
    Represents a Websockets request.
    Contains additional information extracted from a ``Upgrade: websocket`` request.

    Args:
        http_request (:class:`krait.Request`): The HTTP Upgrade request.

    Attributes:
        protocols (list of str, optional): The list of protocols options, or None if no options are specified.
    """

    def __init__(self, http_request):
        self.protocols = WebsocketsRequest.extract_protocols(http_request)

    @staticmethod
    def extract_protocols(http_request):
        """
        Extract the Websocket protocol options from an HTTP request.

        Args:
            http_request (:class:`krait.Request`): The HTTP Upgrade request.

        Returns:
            list of str: The list of protocol options specified by the request, or None if no options are specified.
        """

        if http_request.headers.get("upgrade") == "websocket":
            protocols = http_request.headers.get("sec-websocket-protocol")
            if protocols is not None:
                return protocols.split(", ")
            else:
                return None
        else:
            return None


class WebsocketsResponse(object):
    """
    Represents a Websockets response. This class is used to tell Krait what protocol has been chosen
    and what controller will handle the Websockets connection.

    Args:
        controller (:class:`WebsocketsCtrlBase`): The controller to be used to serve this Websockets connection.
        protocol (str, optional): The chosen subprotocol, or None for no protocol.

    Attributes:
        controller (:class:`WebsocketsCtrlBase`): The controller to be used to serve this Websockets connection.
        protocol (str, optional): The chosen subprotocol, or None for no protocol.
    """

    def __init__(self, controller, protocol=None):
        self.protocol = protocol
        self.controller = controller


request = None
"""
:class:`WebsocketsRequest`:
The additional Websockets information sent with the HTTP Upgrade request by the client.
If this is None, this is not a HTTP ``Upgrade: websocket`` request.
"""

response = None
"""
:class:`WebsocketsResponse`:
The response set by the backend, containing information to be sent back to the client,
and the handler for the request.
If this is None, the Websockets upgrade was denied. In this case, :obj:`krait.response` SHOULD be
a 400 Bad Request or similar.
"""


class WebsocketsCtrlBase(object):
    """
    Base class responsible for handling Websockets communication. Subclass it to add behavior.
    Passed as an argument to :class:`WebsocketsResponse` and called by Krait to communicate
    with the Websockets client.
    Do not access its (private) attributes directly, use the provided methods.
    These ensure thread safety, and a stable API.

    Args:
        use_in_message_queue (bool): True to use a message queue for incoming messages.
            Otherwise, the backend would handle these messages on an event model,
            by overriding :obj:`on_in_message`.
    """

    def __init__(self, use_in_message_queue=True):
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
        The handler thread's target.
        Started by the controller's on_start() method.
        Override this to add behavior to your controller.
        """
        pass

    def on_in_message(self, message):
        """
        Called by Krait (indirectly) when a new message arrives from the client.
        By default adds the message to the message queue.
        Override this if the controller is not configured to use messages queues.

        Args:
            message (str): The new message.
        """
        if self._in_message_queue is not None:
            self._in_message_queue.append(message)

    def on_in_message_internal(self, message):
        """
        Called by Krait (directly) when a new message arrives from the client.
        This only acquires the lock, then calls :obj:`on_in_message`.

        Args:
            message (str): The new message;
        """
        with self._in_message_lock:
            self.on_in_message(message)

    def pop_in_message(self):
        """
        Return the first message in the input queue, popping it, if it exists. Return None otherwise.
        Call this to get messages from the client.
        Do **not** call this if the controller doesn't use input message queues.

        Returns:
             str: The value of the first message, or None if there are no messages.
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
        Send a message. This adds it to the queue; Krait will watch the message queue and send it.

        Args:
            message (str): The message to send.
        """
        with self._out_message_lock:
            self._out_message_queue.append(message)

    def pop_out_message(self):
        """
        Return the first message in the output queue, and remove it, if it exists. Return None otherwise.
        Called by Krait to check on messages to send.

        Returns:
            str: The value of the first message, or None if there are no messages.
        """
        with self._out_message_lock:
            if len(self._out_message_queue) == 0:
                return None
            else:
                return self._out_message_queue.pop(0)

    def on_stop(self):
        """
        Set a flag that tells the controller thread to shut down.
        Called by Krait when the WebSockets connection is closing.
        """
        self._exit_event.set()

    def should_stop(self):
        """
        Return True if the shutdown event has been set, or False otherwise.
        Call this periodically from the controller thread to check if you should shut down.
        """
        return self._exit_event.is_set()

    def wait_stopped(self, timeout=None):
        """
        Join the controller thread, with an optional timeout.
        Called by Krait until the thread has shut down.

        Returns:
            bool: True if the thread has shut down, False otherwise.
        """
        if not self._thread.is_alive():
            return True

        self._thread.join(float(timeout))

        return not self._thread.is_alive()
