class Route(object):
    """
    Signifies a route that incoming requests can take.
    """

    """
    The route verbs to choose from (and their mapping to C++ enumerations)
    """
    route_verbs = {
        "GET": 1,  #: Match GET or HEAD requests
        "POST": 2,  #: Match POST requests.
        "PUT": 3,  #: Match PUT requests.
        "DELETE": 4,  #: Match DELETE requests
        "CONNECT": 5,  #: Match CONNECT requests.
        "OPTIONS": 6,  #: Match OPTIONS requests.
        "TRACE": 7,  #: Match TRACE requests.
        "ANY": 8,  #: Match any request.
        "WEBSOCKET": 9  #: Match Websocket upgrade requests.
    }

    def __init__(self, verb=None, url=None, regex=None, target=None):
        """
        :param verb: The routing verb (most HTTP verbs, plus ANY and WEBSOCKET).
            See Route.route_verbs for options; GET by default
        :param url: The URL to match, or None to skip
        :param regex: The regex to match, or None to skip
        :param target: The target of the route, or None to keep default target (extracted from URL)
        """
        self.verb = verb or "GET"
        self.url = url
        self.regex = regex
        self.target = target

        if self.verb not in Route.route_verbs:
            raise ValueError("Route verb {} not recognized\n"
                             "See krait.config.Route.route_verbs for options".format(self.verb))


#: The list of routes to be read by Krait. A list of krait.config.Route, or None for default
#: The default is all GETs to default targets, deny anything else.
routes = None


#: The list of filename regexes that the clients shouldn't cache.
cache_no_store = []

#: The list or filename regexes that only private (client) caches should keep.
#: This usually means that browser caches can keep the resource, but not shared caches.
cache_private = []

#: The list of filename regexes that can be kept in any caches.
cache_public = []

#: The list of filename regexes that can be kept for a longer time.
#: This duration is configured with ``krait.config.cache_max_age_long_term``
#: This is a modifier, so it can be applied to both private or public cache directives.
cache_long_term = []

#: The number of seconds that clients should not re-request the resource.
#: This corresponds to the Max-Age of the HTTP response, for public or private (and not long-term) cached resources.
cache_max_age_default = 300

#: The number of seconds that clients should not re-request the resource, for long-term cached resources.
#: This corresponds to the Max-Age of the HTTP responses, for long-term, public or private, cached resources.
cache_max_age_long_term = 864000
