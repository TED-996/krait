"""
Signifies a route that incoming requests can take.
"""
class Route(object):
    """
    The route verbs to choose from (and their mapping to C++ enumerations)
    """
    route_verbs = {
        "GET": 1,
        "POST": 2,
        "PUT": 3,
        "DELETE": 4,
        "CONNECT": 5,
        "OPTIONS": 6,
        "TRACE": 7,
        "ANY": 8,
        "WEBSOCKET": 9
    }

    def __init__(self, verb=None, url=None, regex=None, target=None):

        self.verb = verb or "GET"
        self.url = url
        self.regex = regex
        self.target = target

        if self.verb not in Route.route_verbs:
            raise ValueError("Route verb {} not recognized\n"
                             "See krait.config.Route.route_verbs for options".format(self.verb))


routes = None


cache_no_store = []
cache_private = []
cache_public = []
cache_long_term = []

cache_max_age_default = 300
cache_max_age_long_term = 864000