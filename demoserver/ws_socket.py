import websockets
import krait
import ctrl_pingpong_ws

if websockets.request is None:
    krait.response = krait.ResponseBadRequest()
else:
    protocols = websockets.request.protocols
    if "pingpong" in protocols:
        websockets.response = websockets.WebsocketsResponse(ctrl_pingpong_ws.WsPingpongController(), "pingpong")
    else:
        krait.response = krait.ResponseBadRequest()
