import krait.cookie
import krait.mvc
import krait.websockets
from krait.http_request import Request
from krait.http_response import Response, ResponseNotFound, ResponseBadRequest, ResponseRedirect

import os

__all__ = ["cookie", "krait_utils", "mvc", "websockets"]


# Globals are dumped here for API ease of use.

"""
The site_root (directly from the krait argument)
"""
site_root = ""


def get_full_path(filename):
    """
    Converts a filename relative to the site-root to its full path
    Note that this is not necessarily absolute, but is derived from the krait argument.
    :param filename: a filename relative to the site-root (the krait argument)
    :return: os.path.join(site_root, filename)
    """
    global site_root
    print "site root is ", site_root
    return os.path.join(site_root, filename)


# An object of type krait.Request, set before any responses are handled
request = None
# An object of type krait.Response (or its subclasses); leave None for usual behavior, set to override.
response = None

# If this is None, get from source extension.
# If this is "ext/.{extension}", read from mime.types;
#   (since this may change, use krait.set_content_type(ext="your-ext")
# otherwise this is the content-type to send to the client.
content_type = None

# Extra response headers to set without overriding the entire response, as a tuple list
extra_headers = None


def set_content_type(raw=None, ext=None):
    """
    Sets the content-type to a custom value
    Use when the routable page's extension is not the extension of the final content.
    :param raw: full content-type (e.g 'application/json')
    :param ext: extension from which to derive the content-type (e.g. 'json')
    """
    global content_type
    if raw is not None:
        content_type = raw
    elif ext is not None:
        content_type = "ext/{}".format(ext)
    else:
        content_type = None
