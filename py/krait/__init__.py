"""
This is the root of the Krait API. This has two purposes:

* As a package, this holds the different modules that you can use to interact with Krait,
  namely :obj:`krait.config`, :obj:`krait.mvc`, :obj:`krait.cookie` and :obj:`krait.websockets`.
* As a (pseudo-)module, this holds generic data related to the HTTP request, or site configuration.
  While this may not be the best practice, given Python's poor concurrency, the fact that
  any process serves at most one request at a time, this is safe.
  Krait emphasizes ease of use over absolute purity.

Reference
==========
"""


import krait.cookie
import krait.mvc
import krait.websockets
import krait.config
from krait._http_request import Request
from krait._http_response import Response, ResponseNotFound, ResponseBadRequest, ResponseRedirect

try:
    import _krait_emit

    _emit = _krait_emit.emit
    _emit_raw = _krait_emit.emit_raw
except ImportError:
    def _emit(s):
        raise RuntimeError("Not running under Krait, cannot emit.")
    def _emit_raw(s):
        raise RuntimeError("Not running under Krait, cannot emit.")


import os

__all__ = ["cookie", "mvc", "websockets", "config",
           "request", "response", "site_root", "get_full_path", "extra_headers", "set_content_type",
           "Request", "Response", "ResponseNotFound", "ResponseBadRequest", "ResponseRedirect"]


# Globals are dumped here for API ease of use.

site_root = None
"""str: The site root (lifted directly from the krait argument)"""


def get_full_path(filename):
    """
    Convert a filename relative to the site root to its full path.
    Note that this is not necessarily absolute, but is derived from the krait argument.

    Args:
        filename (str): a filename relative to the site root (the krait argument)

    Returns:
        str: ``os.path.join(krait.site_root, filename)``
    """

    global site_root
    return os.path.join(site_root, filename)


request = None
""":class:`krait.Request`: The HTTP request that is being handled right now. Set by Krait."""

response = None
"""
:class:`krait.Response`: Set this to completely change the HTTP response, leave None for usual behaviour.
Useful when you want, for example, to return a *400 Bad Request* or *302 Found*.
If you only want to add or change headers, use :obj:`krait.extra_headers`.
"""

_content_type = None
"""
str: Specifies the *Content-Type* of the response. Only set it with :obj:`krait.set_content_type`.
If None, the server deduces it from the original route target's extension.
"""

extra_headers = []
"""
list of (str, str): Extra response headers to set without overriding the entire response.
"""


def set_content_type(raw=None, ext=None):
    """
    Set the HTTP Content-Type to a custom value.
    Used when the original route target's extension is not relevant to the extension of the final content.

    Args:
        raw (str, optional): full MIME type (e.g. ``'application/json'``)
        ext (str, optional): file extension from which to derive the MIME type (e.g. ``'json'``)
    """

    global _content_type
    if raw is not None:
        _content_type = raw
    elif ext is not None:
        _content_type = "ext/{}".format(ext)
    else:
        _content_type = None
