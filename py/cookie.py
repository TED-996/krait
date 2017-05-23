import krait
import collections
import datetime
import time
import krait_utils

"""
Represents a parsed cookie.
"""
#Cookie = collections.namedtuple("Cookie", ["name", "value", "attributes"])


class Cookie:
    """
    Represents a cookie. Has a name, a value and zero or more attributes.
    """
    def __init__(self, name, value, attributes=None):
        """
        :param name: the name of the cookie
        :param value: the value of the cookie
        :param attributes: a list of attributes (of type cookie.CookieAttribute), can be updated with instance methods.
        """
        self.name = name
        self.value = value
        self.attributes = attributes or []

    def __str__(self):
        """
        Convert the Cookie to the Set-Cookie syntax
        """
        return "; ".join(["{}={}".format(self.name, self.value)] + [str(a) for a in self.attributes])

    def add_attribute(self, attribute):
        """
        Adds an attribute to the cookie.
        :param attribute: the attribute to add 
        """
        self.remove_attribute(attribute.name)
        self.attributes.append(attribute)

    def remove_attribute(self, name):
        """
        Removes all attributes with a certain name from the cookie.
        :param name: the name of the attributes to remove
        """
        name = name.lower()
        for attribute in self.attributes[:]:
            if attribute.name.lower() == name:
                self.attributes.remove(attribute)

    def set_expires(self, expires_datetime):
        """
        Sets or removes the Expires attribute on the cookie. 
        This makes the cookie delete itself at a certain time.
        :param expires_datetime: the **UTC**/timezoned time of expiration as a datetime.datetime, or None to remove.
        """
        if expires_datetime is None:
            self.remove_attribute("Expires")
        else:
            self.add_attribute(CookieExpiresAttribute(expires_datetime))

    def set_max_age(self, max_age):
        """
        Sets or removes the Max-Age attribute on the cookie. Warning, this is not supported by all browsers.
        Notably, Internet Explorer does not respect this.
        This makes the cookie delete itself after a certain numbe rof seconds
        :param max_age: the maximum time that the cookie can be kept, in seconds, or None to remove
        """

        if max_age is None:
            self.remove_attribute("Max-Age")
        else:
            self.add_attribute(CookieMaxAgeAttribute(max_age))

    def set_path(self, path):
        """
        Sets or removes the path attribute on the cookie.
        This restricts the cookie to only one URL (and its descendants)
        :param path: the path, or None to remove
        """

        if path is None:
            self.remove_attribute("Path")
        else:
            self.add_attribute(CookiePathAttribute(path))

    def set_domain(self, domain):
        """
        Sets or removes the domain attribute on the cookie.
        This changes the domain on which the cookie can be sent.
        :param domain: the domain, or None to remove
        """

        if domain is None:
            self.remove_attribute("Domain")
        else:
            self.add_attribute(CookieDomainAttribute(domain))

    def set_secure(self, is_secure):
        """
        Sets or removes the Secure attribute on the cookie.
        This causes the cookie to only be sent over HTTPS (not yet supported by Krait).
        :param is_secure: True to set the attribute, False to remove it
        """

        if is_secure:
            self.add_attribute(CookieSecureAtribute())
        else:
            self.remove_attribute("Secure")

    def set_http_only(self, is_http_only):
        """
        Sets or removes the HttpOnly attribute on the cookie.
        This causes the cookie to be unaccessible from Javascript.
        :param is_http_only: True to set the attribute, False to remove it
        """

        if is_http_only:
            self.add_attribute(CookieHttpOnlyAttribute())
        else:
            self.remove_attribute("HttpOnly")


class CookieAttribute(object):
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
    def __init__(self, expire_datetime):
        """
        :param expire_datetime: the date that the cookie expires on, as a datetime.datetime
        """
        super(CookieExpiresAttribute, self).__init__("expires", krait_utils.date_to_gmt_string(expire_datetime))


class CookieMaxAgeAttribute(CookieAttribute):
    def __init__(self, max_age):
        super(CookieMaxAgeAttribute, self).__init__("max-age", str(max_age))


class CookiePathAttribute(CookieAttribute):
    def __init__(self, path):
        super(CookiePathAttribute, self).__init__("path", path)


class CookieDomainAttribute(CookieAttribute):
    def __init__(self, domain):
        super(CookieDomainAttribute, self).__init__("domain", domain)


class CookieHttpOnlyAttribute(CookieAttribute):
    def __init__(self):
        super(CookieHttpOnlyAttribute, self).__init__("httponly", None)  # TODO: must be uppercase?


class CookieSecureAtribute(CookieAttribute):
    def __init__(self):
        super(CookieSecureAtribute, self).__init__("secure", None)

"""
Computed request cookies. Since it would be of no use to keep recomputing them, these is cached.
"""
_cookies_cpt = None


def get_cookies():
    """
    Gets all request cookies, as a list of cookie.Cookie.
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
    Gets the value of a single cookie by name.
    :param name: the name of the cookie to be returned.
    :param default: value to be used if the cookie cannot be found.
    """
    for cookie in get_cookies():
        if cookie.name == name:
            return cookie.value

    return default


def _split_equals(item):
    """
    Split a 'name=value' string in a (name, value) tuple.
    :param item: the string to be split, in a 'name=value' format.
    :return: a (name, value) tuple
    """
    sep_idx = item.index("=")
    return item[:sep_idx], item[sep_idx + 1:]


def get_response_cookies():
    """
    Gets cookies already set with cookie.setCookie or direct header manipulation, as a list of cookie.Cookie items.
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
    Set a new (or updated) cookie.
    :param cookie: the cookie item, as a cookie.Cookie named tuple.
    """
    krait.extra_headers.append(("set-cookie", str(cookie)))


