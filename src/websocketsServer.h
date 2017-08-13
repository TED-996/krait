#pragma once
#include <string>
#include "websocketsTypes.h"
#include "request.h"
#include "IManagedSocket.h"


class WebsocketsServer
{
private:
	IManagedSocket& managedSocket;
	bool closed;

	boost::optional<WebsocketsMessage> read(int timeoutMs);
	void write(WebsocketsMessage message);

	void sendPing();
	void sendClose();

	void handlePing(WebsocketsFrame ping);
	void handlePong(WebsocketsFrame pong);
	void handleClose(WebsocketsFrame close);
	bool handleUpgradeRequest(Request& upgradeRequest);
public:
	WebsocketsServer(int clientSocket);
	bool start(Request& upgradeRequest);
};
