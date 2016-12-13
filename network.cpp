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
#include "except.h"

using namespace std;


int getListenSocket(int port) {
    sockaddr_in serverSockaddr;
    memzero(serverSockaddr);

    serverSockaddr.sin_family = AF_INET;
    serverSockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverSockaddr.sin_port = htons(port);

    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd == -1) {
        throw networkError() << stringInfo("getListenSocket: could not create socket!");
    }

    if (bind(sd, (sockaddr*)&serverSockaddr, sizeof(sockaddr)) != 0) {
        throw networkError() << stringInfo("getListenSocket: could not bind socket");
    }

    if (listen(sd, 1024) == -1) {
        throw networkError() << stringInfo("getListenSocket: could not set socket to listen");
    }

    return sd;
}


int getNewClient(int listenerSocket) {
    sockaddr_in clientSockaddr;
    memzero(clientSockaddr);

    socklen_t sockaddrLen = sizeof(clientSockaddr);
    int client = accept(listenerSocket, (sockaddr*)&clientSockaddr, &sockaddrLen);

    if (client < 0) {
        throw networkError() << stringInfo("getNewClient: could not accept new client");
    }

    return client;
}


void printSocket(int clientSocket) {
    char data[4096];
    memzero(data);
    while(read(clientSocket, data, 4095)) {
        printf("%s\n", data);
    }
}

void respondWith(int clientSocket, const char* response);

void respondRequestHttp10(int clientSocket){
	respondWith(clientSocket, "HTTP/1.0 505 Version Not Supported\nConnection:Close\n\n");	
}

void respondRequest404 (int clientSocket){
	respondWith(clientSocket, "HTTP/1.0 404 Not Found\nConnection:Close\n\n)");
}

void respondWith(int clientSocket, const char* response){
	int len = strlen(response);
	
	if (write(clientSocket, response, len) != len){
		throw networkError() << stringInfo("respondWith: could not send response.");
	}
}

Header::Header ( string name, string value ) {
	this->name = name;
	this->value = value;
}

Request::Request (HttpVerb verb, const string& url, int httpMajor, int httpMinor, const vector< Header >& headers, const string& body) {
	this->verb = verb;
	this->url = url;
	this->httpMajor = httpMajor;
	this->httpMinor = httpMinor;
	this->headers = map<string, string>();
	
	for (auto &it : headers){
		this->headers[it.name] = it.value;
	}
	
	this->body = string(body);
}

const string* Request::getHeader ( const string& name ) {
	auto iterFound = headers.find(name);
	if (iterFound == headers.end()){
		return NULL;
	}
	return &(iterFound->second);
}

