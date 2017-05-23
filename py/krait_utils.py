days = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"]
months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]


def date_to_gmt_string(expire_datetime):
    utc_datetime = datetime_to_utc(expire_datetime)
    return "{}, {} {} {} {}:{}:{} GMT".format(days[utc_datetime.weekday()],
                                              utc_datetime.day,
                                              months[utc_datetime.month - 1],
                                              utc_datetime.year,
                                              utc_datetime.hour,
                                              utc_datetime.minute,
                                              utc_datetime.second)


def datetime_to_utc(dt):
    if dt.utcoffset() is None:
        return dt
    else:
        return (dt - dt.utcoffset()).replace(tzinfo=None)

