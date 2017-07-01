import socket

def check_internet_access(host="8.8.8.8", port=53, timeout=3):
    orig_timeout = socket.getdefaulttimeout()
    try:
        socket.setdefaulttimeout(timeout)
        socket.socket(socket.AF_INET, socket.SOCK_STREAM).connect((host, port))
        return True
    except StandardError:
        return False
    finally:
        socket.setdefaulttimeout(orig_timeout)