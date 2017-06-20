import krait
import ctrl_pingpong_ws

if krait.websockets.request is None:
    krait.response = krait.ResponseBadRequest()
else:
    protocols = krait.websockets.request.protocols
    if "pingpong" in protocols:
        krait.websockets.response = krait.websockets.WebsocketsResponse(ctrl_pingpong_ws.WsPingpongController(), "pingpong")
    else:
        krait.response = krait.ResponseBadRequest()
