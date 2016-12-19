#pragma once
#include<vector>
#include<string>
#include<map>

#include"request.h"
#include"response.h"


int getServerSocket(int port, bool setListen);
void setSocketListen(int sd);
int getNewClient(int listenerSocket, int timeoutMs);
void closeSocket(int clientSocket);

void printSocket(int clientSocket);
Request getRequestFromSocket(int clientSocket);

void respondRequestHttp10(int clientSocket);
void respondRequest404(int clientSocket);
void respondRequest200(int clientSocket);
void respondWithObject(int clientSocket, Response response);

