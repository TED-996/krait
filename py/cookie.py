import krait
import collections

Cookie = collections.namedtuple("Cookie", ["name", "value", "attributes"])
cookies_cpt = None


def get_cookies():
    global cookies_cpt
    if cookies_cpt is not None:
        return cookies_cpt

    cookie_header = krait.request.headers.get("cookie")
    if cookie_header is None:
        cookies_cpt = []
        return []
    else:
        parts = cookie_header.split("; ")

        results = []
        for part in parts:
            name, value = split_equals(part)
            results.append(Cookie(name, value, []))

        cookies_cpt = results
        return results


def get_cookie(name, default=None):
    for cookie in get_cookies():
        if cookie.name == name:
            return cookie.value

    return default


def split_equals(item):
    sep_idx = item.index("=")
    return item[:sep_idx], item[sep_idx + 1:]


def get_response_cookies():
    cookie_values = [value for name, value in krait.extra_headers if name == 'set-cookie']
    results = []

    for value in cookie_values:
        parts = value.split(", ")
        parts_split = [split_equals(part) for part in parts]
        results.append(Cookie(parts_split[0][0], parts_split[0][1], parts_split[1:]))

    return results


def set_cookie(cookie):
    name, value, attributes = cookie

    header_value = "; ".join(["{}={}".format(name, value)] + ["{}={}".format(n, v) for n, v in attributes])
    krait.extra_headers.append(("set-cookie", header_value))
