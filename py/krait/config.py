"""
This module is used to configure the behaviour of Krait when a request arrives and when its response is sent.
Currently, this module is used to configure routing and the client-side cache.

The members of this module should only be changed from ``.py/init.py``, later changes are ignored.

Usage
=====



Reference
=========
"""


class Route(object):
    """
    Signifies a route that incoming requests can take.

    Args:
        verb (str, optional): The routing verb (most HTTP verbs, plus ``ANY`` and ``WEBSOCKET``).
            See :obj:`Route.route_verbs` for options; GET by default
        url (str, optional): The URL to match, or None to skip
        regex (str, optional): The regex to match, or None to skip
        target (str, optional): The target of the route, or None to keep default target (extracted from URL)
        ctrl_class (Callable[[], krait.mvc.CtrlBase], optional): The MVC controller class to be used,
            or None to ignore. This is usually a class name, but can be a simple function.

    Attributes:
        verb (str): The routing verb
        url (str, optional): The URL to match, or None to skip
        regex (str, optional): The regex to match, or None to skip
        target (str, optional): The target of the route. None keeps default target (extracted from the URL)
        ctrl_class (Callable[[], krait.mvc.CtrlBase], optional): The MVC controller class,
            or constructor function.
    """

    route_verbs = [
        "GET",  # Match GET or HEAD requests
        "POST",  # Match POST requests.
        "PUT",  # Match PUT requests.
        "DELETE",  # Match DELETE requests
        "CONNECT",  # Match CONNECT requests.
        "OPTIONS",  # Match OPTIONS requests.
        "TRACE",  # Match TRACE requests.
        "ANY",  # Match any request.
        "WEBSOCKET"  # Match Websocket upgrade requests.
    ]
    """The route verb options."""

    def __init__(self, verb=None, url=None, regex=None, target=None, ctrl_class=None):
        self.verb = verb or "GET"
        self.url = url
        self.regex = regex
        self.target = target
        self.ctrl_class = ctrl_class

        if self.verb not in Route.route_verbs:
            raise ValueError("Route verb {} not recognized\n"
                             "See krait.config.Route.route_verbs for options".format(self.verb))
        if target is not None and ctrl_class is not None:
            raise ValueError("Both target and controller class specified, invalid route.")


routes = None
"""
list of :class:`Route`:
The list of routes to be read by Krait. The default is all GETs to default targets, deny anything else.
If you override this, the last route *should* be a default one (``Route()``) to allow GET requests to reach
resources that aren't explicitly routed (like CSS or Javascript files)
"""

cache_no_store = []
"""list of str: The list of filename regexes that the clients shouldn't cache."""

cache_private = []
""":
list of str: 
The list or filename regexes that only private (client) caches should keep.
This usually means that browser caches can keep the resource, but not shared caches.
"""

cache_public = []
"""list of str: The list of filename regexes that can be kept in any caches."""


cache_long_term = []
"""
list of str: 
The list of filename regexes that can be kept for a longer time.
This duration is configured with :obj:`krait.config.cache_max_age_long_term`
This is a modifier, so it can be applied to both private or public cache directives.
"""

cache_max_age_default = 300
"""
int: 
The number of seconds that clients should not re-request the resource.
This corresponds to the Max-Age of the HTTP response, for public or private (and not long-term) cached resources.
"""

cache_max_age_long_term = 864000
"""
int:
The number of seconds that clients should not re-request the resource, for long-term cached resources.
This corresponds to the Max-Age of the HTTP responses, for long-term, public or private, cached resources.
"""
