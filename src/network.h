#pragma once
#include<string>
#include<boost/optional.hpp>

#include"request.h"
#include"response.h"
#include"websocketsTypes.h"


int getServerSocket(int port, bool setListen, bool reuseAddr);
void setSocketListen(int sd);
int getNewClient(int listenerSocket, int timeoutMs);
void closeSocket(int clientSocket);

void printSocket(int clientSocket);
boost::optional<Request> getRequestFromSocket(int clientSocket, int timeoutMs);

WebsocketsFrame getWebsocketsFrame(int clientSocket);
boost::optional<WebsocketsFrame> getWebsocketsFrameTimeout(int clientSocket, int timeoutMs);
void sendWebsocketsFrame(int clientSocket, WebsocketsFrame& frame);

void respondRequestHttp10(int clientSocket);
void respondRequest404(int clientSocket);
void respondRequest200(int clientSocket);
void respondWithObjectRef(int clientSocket, Response& response);
void respondWithObject(int clientSocket, Response response);
