"""
This module is used to wrap the usage of cookies in websites.
This prevents the need to manipulate the HTTP headers manually.

Usage
=====

1. Get request cookies:
    Use :obj:`cookie.get_cookie` or :obj:`cookie.get_cookies` to get all
2. Set cookies:
    Create a new :class:`cookie.Cookie` object, optionally add attributes to it,
    then use :obj:`cookie.set_cookie` to make it be sent with the HTTP response.
3. Get response cookies:
    Use :obj:`cookie.get_response_cookies`.

Reference
=========
"""


import krait
import krait_utils


class Cookie:
    """
    Represents an HTTP cookie.

    Args:
        name (str): The name of the cookie
        value (str): The value of the cookie.
        attributes (list of :class:`CookieAttribute`, optional): A (possibly empty) list of attributes.
            Can be updated later.

    Attributes:
        name (str): The name of the cookie.
        value (str): The value of the cookie.
        attributes (list of :class:`CookieAttribute`): A (possibly empty) list of attributes.
            Update only with instance methods.
    """

    def __init__(self, name, value, attributes=None):
        self.name = name
        self.value = value
        self.attributes = attributes or []

    def __str__(self):
        """
        Convert the Cookie to the Set-Cookie syntax

        Returns
            str: The cookie in standard HTTP ``Set-Cookie`` syntax.
        """
        if not self.name:
            main_name = self.value
        else:
            main_name = self.name + "=" + str(self.value)

        return "; ".join([main_name] + [str(a) for a in self.attributes])

    def __repr__(self):
        """
        Convert the Cookie to its representation.

        Returns
            str: The cookie in its representation form.
        """
        # TODO: make it re-initializable code.
        return "krait.cookie.Cookie("\
            + repr(self.name) + ", "\
            + repr(self.value)\
            + ("" if not self.attributes else ", attributes=[...]")\
            + ")"

    def add_attribute(self, attribute):
        """
        Add an attribute to the cookie.

        Args:
            attribute :class:`CookieAttribute`: The attribute to add.
        """

        self.remove_attribute(attribute.name)
        self.attributes.append(attribute)

    def remove_attribute(self, name):
        """
        Remove an attribute from the cookie.

        Args:
            name (str): The name of the attribute to remove (e.g. ``Expires``).
        """

        name = name.lower()
        for attribute in self.attributes[:]:
            if attribute.name.lower() == name:
                self.attributes.remove(attribute)

    def set_expires(self, expires_datetime):
        """
        Set or remove the *Expires* attribute on the cookie.
        This makes the cookie delete itself after a certain time.

        Args:
            expires_datetime (:obj:`datetime.datetime`, optional):
                The **UTC**/timezoned time of expiration, or None to remove.
        """

        if expires_datetime is None:
            self.remove_attribute("Expires")
        else:
            self.add_attribute(CookieExpiresAttribute(expires_datetime))

    def set_max_age(self, max_age):
        """
        Set or remove the *Max-Age* attribute on the cookie.
        This makes the cookie delete itself after a certain number of seconds.

        Warnings:
            This attribute is not supported by all browsers. Notably, Internet Explorer does not respect it.
        Args:
            max_age (int, optional): The maximum time that a cookie can be kept, in seconds, or None to remove.
        """

        if max_age is None:
            self.remove_attribute("Max-Age")
        else:
            self.add_attribute(CookieMaxAgeAttribute(max_age))

    def set_path(self, path):
        """
        Set or remove the *Path* attribute on the cookie.
        This restricts the cookie only to one URL and its descendants.

        Args:
            path (str, optional): The path, or None to remove.
        """

        if path is None:
            self.remove_attribute("Path")
        else:
            self.add_attribute(CookiePathAttribute(path))

    def set_domain(self, domain):
        """
        Set or remove the *Domain* attribute on the cookie.
        This restricts the domain on which the cookie can be sent by the client.

        Args:
            domain (str, optional): The domain, or None to remove.
        """

        if domain is None:
            self.remove_attribute("Domain")
        else:
            self.add_attribute(CookieDomainAttribute(domain))

    def set_secure(self, is_secure):
        """
        Set or remove the *Secure* attribute on the cookie.
        This causes the cookie to only be sent over HTTPS (not yet supported by Krait).

        Args:
            is_secure (bool): True to set the attribute, False to remove it.
        """

        if is_secure:
            self.add_attribute(CookieSecureAttribute())
        else:
            self.remove_attribute("Secure")

    def set_http_only(self, is_http_only):
        """
        Set or remove the *HTTPOnly* attribute on the cookie.
        This causes the cookie to be inaccessible from Javascript.

        Args:
            is_http_only (bool): True to set the attribute, False to remove it.
        """

        if is_http_only:
            self.add_attribute(CookieHttpOnlyAttribute())
        else:
            self.remove_attribute("HttpOnly")


class CookieAttribute(object):
    """
    A generic cookie attribute.

    Args:
        name (str): The name of the attribute.
        value (str, optional): The value of the attribute.

    Attributes:
        name (str): The name of the attribute.
        value (str): The value of the attribute.

    """
    def __init__(self, name, value):
        self.name = name
        self.value = value

    def __str__(self):
        if self.value is None:
            return self.name
        else:
            return self.name + "=" + self.value

    def __repr__(self):
        return "CookieAttribute([}, {})".format(repr(self.name), repr(self.value))


class CookieExpiresAttribute(CookieAttribute):
    """
    Sets the *Expires* attribute on the cookie.
    This makes the cookie delete itself after a certain time.

    Args:
        expire_datetime (:obj:`datetime.datetime`): the moment that the cookie expires at

    Attributes:
        expire_datetime (:obj:`datetime.datetime`): the moment that the cookie expires at
    """

    def __init__(self, expire_datetime):
        super(CookieExpiresAttribute, self).__init__("expires", krait_utils.date_to_gmt_string(expire_datetime))


class CookieMaxAgeAttribute(CookieAttribute):
    """
    Sets the *Max-Age* attribute on the cookie.
    This makes the cookie delete itself after a certain number of seconds.

    Warnings:
        This attribute is not supported by all browsers. Notably, Internet Explorer does not respect it.

    Args:
        max_age (int): The lifetime of the cookie, in seconds.

    Attributes:
        max_age (int): The lifetime of the cookie, in seconds.
    """

    def __init__(self, max_age):
        super(CookieMaxAgeAttribute, self).__init__("max-age", str(max_age))


class CookiePathAttribute(CookieAttribute):
    """
    Sets the *Path* attribute on the cookie.
    This restricts the cookie only to one URL and its descendants.

    Args:
        path (str): The URL to which to restrict the cookie.

    Attributes:
        path (str): The URL to which to restrict the cookie.
    """

    def __init__(self, path):
        super(CookiePathAttribute, self).__init__("path", path)


class CookieDomainAttribute(CookieAttribute):
    """
    Sets the *Domain* attribute on the cookie.
    This restricts the domain on which the cookie can be sent by the client.

    Args:
        domain (str): The domain on which the cookie is restricted.

    Attributes:
        domain (str): The domain on which the cookie is restricted.
    """

    def __init__(self, domain):
        super(CookieDomainAttribute, self).__init__("domain", domain)


class CookieHttpOnlyAttribute(CookieAttribute):
    """
    Sets the *HttpOnly* attribute on the cookie.
    This causes the cookie to be inaccessible from Javascript.
    """

    def __init__(self):
        super(CookieHttpOnlyAttribute, self).__init__("httponly", None)  # TODO: must be uppercase?


class CookieSecureAttribute(CookieAttribute):
    """
    Sets the *Secure* attribute on the cookie.
    This causes the cookie to only be sent over HTTPS (not yet supported by Krait).
    """

    def __init__(self):
        super(CookieSecureAttribute, self).__init__("secure", None)


_cookies_cpt = None
"""
list of :class:`Cookie`:
Computed request cookies. Since it would be of no use to keep recomputing them, these are cached.
"""


def get_cookies():
    """
    Get all the cookies sent by the client.

    Returns:
        list of :class:`Cookie`
    """
    global _cookies_cpt
    if _cookies_cpt is not None:
        return _cookies_cpt

    cookie_header = krait.request.headers.get("cookie")
    if cookie_header is None:
        _cookies_cpt = []
        return []
    else:
        parts = cookie_header.split("; ")

        results = []
        for part in parts:
            name, value = _split_equals(part)
            results.append(Cookie(name, value, []))

        _cookies_cpt = results
        return results


def get_cookie(name, default=None):
    """
    Get the value of a single cookie, by name.

    Args:
        name (str): The name of the cookie to be returned.
        default (str, optional): The value to be used if the cookie cannot be found.
    Returns:
        The value of the cookie, or the second argument, if it doesn't exist.
    """
    for cookie in get_cookies():
        if cookie.name == name:
            return cookie.value

    return default


def _split_equals(item):
    """
    Split a ``name=value`` string in a ``(name, value) tuple.

    Args:
        item (str): the string to be split, as ``'name=value'``.

    Returns:
        (str, str): the ``(name, value)`` tuple.
    """
    sep_idx = item.find("=")
    if sep_idx == -1:
        return "", item

    return item[:sep_idx], item[sep_idx + 1:]


def get_response_cookies():
    """
    Get cookies already set with :obj:`cookie.setCookie()` or direct header manipulation

    Returns:
        list of :class:`cookie.Cookie`: the response cookies already set.
    """

    cookie_values = [value for name, value in krait.extra_headers if name == 'set-cookie']
    results = []

    for value in cookie_values:
        parts = value.split(", ")
        parts_split = [_split_equals(part) for part in parts]
        results.append(Cookie(parts_split[0][0], parts_split[0][1], parts_split[1:]))

    return results


def set_cookie(cookie):
    """
    Set a new (or updated) cooke.

    Args:
        cookie (:class:`cookie.Cookie`): the cookie item.
    """

    krait.extra_headers.append(("set-cookie", str(cookie)))
