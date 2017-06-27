"""
This module contains functions used in more than one place in Krait's Python codebase.
Generally not for user usage.
"""


_days = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"]
_months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]


def date_to_gmt_string(value_datetime):
    """
    Convert a :obj:`datetime.datetime` object to a string as defined in Section 7.1.1.1 of RFC7231.
    The argument should either be a naive GMT/UTC datetime, or timezone-aware.

    Args:
        value_datetime :obj:`datetime.datetime`: The datetime to convert.

    Returns:
        str: The representation of this datetime in the proper format.
    """

    utc_datetime = _datetime_to_utc(value_datetime)
    return "{}, {} {} {} {}:{}:{} GMT".format(_days[utc_datetime.weekday()],
                                              utc_datetime.day,
                                              _months[utc_datetime.month - 1],
                                              utc_datetime.year,
                                              utc_datetime.hour,
                                              utc_datetime.minute,
                                              utc_datetime.second)


def _datetime_to_utc(dt):
    if dt.utcoffset() is None:
        return dt
    else:
        return (dt - dt.utcoffset()).replace(tzinfo=None)

