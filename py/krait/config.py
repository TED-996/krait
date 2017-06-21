class Route(object):
    def __init__(self, url=None, regex=None, target=None):
        self.url = url
        self.regex = regex
        self.target = target


