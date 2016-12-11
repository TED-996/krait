#pragma once

int getListenSocket(int port);
int getNewClient(int listenerSocket);

void printSocket(int clientSocket);