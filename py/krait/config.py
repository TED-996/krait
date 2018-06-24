"""
This module is used to configure the behaviour of Krait when a request arrives and when its response is sent.
Currently, this module is used to configure routing and the client-side cache.

The members of this module should only be changed from ``.py/init.py``, later changes are ignored.

The routing engine:
===================

Krait uses routing to decide how to start responding to the client's requests.
For this, you can (should) configure a list of routes.
Each route consists of two components: the matching rule and the target.
The matching rule decides if a route is a match for a specific URL,
and the target decides what is the source to use for the response.

Matching rules can be either universal (matching everything), a string or a regular expression.
These must be matched exactly (and are case-sensitive), or else the request will not be answered
by what you would expect, or it might produce a 404.
Route targets can be either the default (the input URL as a file in the site root directory),
a filename (for now static one, in the future you will be able to make it depend on the URL)
or a MVC controller class. Use MVC controllers where possible, as it can improve performance.

The order of the routes in the route list is important, as the first route to match will be used.
It is recommended that the last route is a default route (matching everything, with default targets)
to serve other files in the site root, such as Javascript, CSS or image files.

Usage
=====

1. Configure the routes that Krait will use for your website:
    Create a file named ``init.py`` (no underscores) in the ``.py`` directory, if it doesn't exist.
    In it, import :obj:`krait.config` and assign :obj:`krait.config.routes`
    to a list of :class:`krait.config.Route` objects.


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
    """list of str: The route verb options."""

    def __init__(self, verb=None, url=None, regex=None, target=None, ctrl_class=None):
        self.verb = verb or "GET"
        self.url = url
        self.regex = regex
        self.target = target
        self.ctrl_class = ctrl_class

        if self.verb not in Route.route_verbs:
            raise ConfigError("Route verb {} not recognized\n"
                             "See krait.config.Route.route_verbs for options".format(self.verb))
        if target is not None and ctrl_class is not None:
            raise ConfigError("Both target and controller class specified, invalid route.")
        if ctrl_class is not None and verb == "WEBSOCKET":
            raise ConfigError("WEBSOCKET verb not supported for MVC controllers.")


routes = []
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

ssl_certificate_path = None
"""
str, optional:
The path to the certificate to be used in SSL connections. Must be in PEM format.
If :obj:`None`, SSL will not work.
"""

ssl_private_key_path = None
"""
str, optional:
The path to the private key to be used to decrypt the SSL certificate above. Must be in PEM format.
If :obj:`None`, SSL will not work.
"""

ssl_key_passphrase = None
"""
str, optional:
The passphrase to the SSL certificate's private key. SHOULD NOT be hardcoded, but read from a file
(not in source control) instead.
If :obj:`None`, the private key is assumed to not be protected by a passphrase.
SSL will not work if the key is, however, protected.
"""

class ConfigError(StandardError):
    """
    An error class that signifies a problem with the Krait site configuration.
    Usually means a configuration setting is incorrect.
    """
    pass
