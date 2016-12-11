#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include "network.h"
#include "utils.h"


int getListenSocket(int port){
    sockaddr_in serverSockaddr;
    memzero(serverSockaddr);

    serverSockaddr.sin_family = AF_INET;
    serverSockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverSockaddr.sin_port = htons(port);

    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd == -1){
        throw "getListenSocket: could not create socket";
    }

    if (bind(sd, (sockaddr*)&serverSockaddr, sizeof(sockaddr)) != 0){
        throw "getListenSocket: could not bind socket";
    }

    if (listen(sd, 1024) == -1){
        throw "getListenSocket: could not set socket to listen";
    }

    return sd;
}


int getNewClient(int listenerSocket){
    sockaddr_in clientSockaddr;
    memzero(clientSockaddr);

    socklen_t sockaddrLen = sizeof(clientSockaddr);
    int client = accept(listenerSocket, (sockaddr*)&clientSockaddr, &sockaddrLen);

    if (client < 0){
        throw "getNewClient: could not accept new client";
    }

    return client;
}


void printSocket(int clientSocket){
    char data[4096];
    memzero(data);
    while(read(clientSocket, data, 4095)){
        printf("%s\n", data);
    }
}