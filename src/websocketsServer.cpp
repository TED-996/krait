#include "websocketsServer.h"
#include "formatHelper.h"
#include "logger.h"
#include "pythonModule.h"
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/uuid/sha1.hpp>

//#define DBG_DISABLE
#include "dbg.h"

WebsocketsServer::WebsocketsServer(IManagedSocket& clientSocket) : clientSocket(clientSocket) {
    this->closed = false;
}

boost::optional<WebsocketsMessage> WebsocketsServer::read(int timeoutMs) {
    boost::optional<WebsocketsFrame> frameOptional = clientSocket.getWebsocketsFrameTimeout(timeoutMs);
    if (!frameOptional) {
        return boost::none;
    }

    WebsocketsFrame frame = frameOptional.get();

    WebsocketsMessage message;
    message.opcode = frame.opcode;
    message.message = frame.message;

    if (frame.opcode == WebsocketsOpcode::Text || frame.opcode == WebsocketsOpcode::Binary) {
        bool messageFinished = frame.isFin;

        while (!messageFinished) {
            frame = clientSocket.getWebsocketsFrame();
            if (frame.opcode == WebsocketsOpcode::Ping) {
                handlePing(frame);
            } else if (frame.opcode == WebsocketsOpcode::Pong) {
                handlePong(frame);
            } else if (frame.opcode == WebsocketsOpcode::Close) {
                handleClose(frame);
                messageFinished = true;
            } else {
                messageFinished = frame.isFin; // Don't finish the message just because of an interjected control frame.
                message.message.append(frame.message); // Don't pollute the message with the ping message.
            }
        }
    } else if (frame.opcode == WebsocketsOpcode::Ping) {
        handlePing(frame);
    } else if (frame.opcode == WebsocketsOpcode::Pong) {
        handlePong(frame);
    } else if (frame.opcode == WebsocketsOpcode::Close) {
        handleClose(frame);
    }

    return message;
}

void WebsocketsServer::write(WebsocketsMessage message) {
    WebsocketsFrame frame;
    frame.opcode = message.opcode;
    frame.isFin = false;

    size_t maxSize = 128 * 1024; // 128KB, totally arbitrary.
    size_t msgIdx = 0;

    do {
        size_t frameLen = maxSize;
        if (msgIdx + maxSize > message.message.length()) {
            frame.isFin = true;
            frameLen = message.message.length() - msgIdx;
        } else {
            frame.isFin = false;
        }
        frame.message.assign(message.message, msgIdx, frameLen);

        clientSocket.sendWebsocketsFrame(frame);

        msgIdx += frameLen;
        frame.opcode = Continuation;
    } while (!frame.isFin);
}

void WebsocketsServer::sendPing() {
    WebsocketsFrame pingFrame;
    pingFrame.opcode = WebsocketsOpcode::Ping;
    pingFrame.isFin = true;
    pingFrame.message = "";

    clientSocket.sendWebsocketsFrame(pingFrame);
}

void WebsocketsServer::sendClose() {
    WebsocketsFrame closeFrame;
    closeFrame.opcode = WebsocketsOpcode::Close;
    closeFrame.isFin = true;
    closeFrame.message = "";

    clientSocket.sendWebsocketsFrame(closeFrame);
}

void WebsocketsServer::handlePing(WebsocketsFrame ping) {
    WebsocketsFrame pongFrame;
    pongFrame.opcode = WebsocketsOpcode::Pong;
    pongFrame.isFin = true;
    pongFrame.message = ping.message;
    clientSocket.sendWebsocketsFrame(pongFrame);
}

void WebsocketsServer::handleClose(WebsocketsFrame close) {
    if (!closed) {
        closed = true;
        sendClose();
    }
}

void WebsocketsServer::handlePong(WebsocketsFrame pong) {
}

bool WebsocketsServer::start(Request& upgradeRequest) {
    if (!handleUpgradeRequest(upgradeRequest)) {
        return false;
    }
    // Request is now upgraded to a WebSockets request.
    boost::python::object ctrlStartMethod = PythonModule::websockets().evalToObject("response.controller.on_start");
    boost::python::object inMsgEvent
        = PythonModule::websockets().evalToObject("response.controller.on_in_message_internal");
    boost::python::object outMsgGetter = PythonModule::websockets().evalToObject("response.controller.pop_out_message");
    boost::python::object ctrlStopMethod = PythonModule::websockets().evalToObject("response.controller.on_stop");
    boost::python::object ctrlWaitStopMethod
        = PythonModule::websockets().evalToObject("response.controller.wait_stopped");

    PythonModule::websockets().callObject(ctrlStartMethod);

    while (!closed) {
        boost::optional<WebsocketsMessage> msgInOpt;
        Py_BEGIN_ALLOW_THREADS msgInOpt = this->read(8);
        Py_END_ALLOW_THREADS if (msgInOpt != boost::none) {
            WebsocketsMessage& msgIn = msgInOpt.get();
            if (msgIn.opcode == WebsocketsOpcode::Text || msgIn.opcode == WebsocketsOpcode::Binary) {
                PythonModule::websockets().callObject(inMsgEvent, boost::python::str(msgIn.message));
            }
        }

        boost::python::object outMsg = PythonModule::websockets().callObject(outMsgGetter);
        if (!outMsg.is_none()) {
            std::string msgString = PythonModule::toStdString(outMsg);

            Py_BEGIN_ALLOW_THREADS WebsocketsMessage msg = {WebsocketsOpcode::Text, msgString};
            this->write(msg);
            Py_END_ALLOW_THREADS
        }
    }

    PythonModule::websockets().callObject(ctrlStopMethod);
    int attemptsLeft = 12; // give it 12 x 5s = 1 minute. A bit much, true.
    while (attemptsLeft > 0
        && !boost::python::extract<bool>(
               PythonModule::websockets().callObject(ctrlWaitStopMethod, boost::python::object(5)))) {
        Loggers::errLogger.log(
            formatString("[WARNING]: Websockets controller on url %1% not shutting down, "
                         "retrying until timeout...",
                upgradeRequest.getUrl()));
        attemptsLeft--;
    }

    if (!boost::python::extract<bool>(
            PythonModule::websockets().callObject(ctrlWaitStopMethod, boost::python::object(1)))) {
        Loggers::errLogger.log(
            formatString("[SERIOUS WARNING]: Websockets controller on url %1% failed to shut down "
                         "within the timeout. Process closing forcefully.",
                upgradeRequest.getUrl()));
    }

    return true;
}

std::string encode64(const std::string& val);

bool WebsocketsServer::handleUpgradeRequest(Request& request) {
    if (PythonModule::websockets().checkIsNone("response")) {
        clientSocket.respondWithObject(Response(400, "", true));
        return false;
    }

    boost::optional<std::string> key = request.getHeader("Sec-WebSocket-Key");
    boost::optional<std::string> version = request.getHeader("Sec-WebSocket-Version");

    boost::optional<std::string> protocol = boost::none;

    if (PythonModule::websockets().test("response.protocol is not None")) {
        protocol = PythonModule::websockets().eval("response.protocol");
    }

    if (!key) {
        clientSocket.respondWithObject(Response(400, "", true));
        return false;
    }
    if (!version || version.get() != "13") {
        clientSocket.respondWithObject(Response(
            1, 1, 400, std::unordered_multimap<std::string, std::string>{{"Sec-WebSocket-Version", "13"}}, "", true));
        return false;
    }


    std::string magicString = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string hashInput = key.value() + magicString;
    boost::uuids::detail::sha1 hash;
    hash.process_bytes(hashInput.c_str(), hashInput.length());
    unsigned int digest[5];
    hash.get_digest(digest);

    char hashBytes[20];
    const char* tmp = reinterpret_cast<char*>(digest);

    for (int i = 0; i < 5; i++) {
        hashBytes[i * 4] = tmp[i * 4 + 3];
        hashBytes[i * 4 + 1] = tmp[i * 4 + 2];
        hashBytes[i * 4 + 2] = tmp[i * 4 + 1];
        hashBytes[i * 4 + 3] = tmp[i * 4];
    }

    std::string outHash = encode64(std::string(hashBytes, 20));

    std::unordered_multimap<std::string, std::string> outHeaders{
        {"Sec-WebSocket-Accept", outHash}, {"Connection", "Upgrade"}, {"Upgrade", "websocket"}};

    if (protocol != boost::none) {
        outHeaders.insert(make_pair("Sec-WebSocket-Protocol", protocol.get()));
    }

    clientSocket.respondWithObject(Response(1, 1, 101, outHeaders, "", false));

    return true;
}


std::string encode64(const std::string& val) {
    using namespace boost::archive::iterators;
    using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
    auto tmp = std::string(It(std::begin(val)), It(std::end(val)));
    return tmp.append((3 - val.size() % 3) % 3, '=');
}
