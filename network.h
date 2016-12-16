#pragma once
#include<vector>
#include<string>
#include<map>

enum HttpVerb {
	GET,
	HEAD,
	POST,
	PUT,
	DELETE,
	CONNECT,
	OPTIONS,
	TRACE
};

class Request{
private:
	HttpVerb verb;
	std::string url;
	int httpMajor;
	int httpMinor;
	std::map<std::string, std::string> headers;
	std::string body;
	
public:
	Request(HttpVerb verb, const std::string &url, int httpMajor, int httpMinor, const std::map<std::string, std::string> &headers, const std::string &body);
	
	const std::string* getHeader(const std::string &name);
};

int getListenSocket(int port);
int getNewClient(int listenerSocket);
void closeSocket(int clientSocket);

void printSocket(int clientSocket);
Request getRequestFromSocket(int clientSocket);

void respondRequestHttp10(int clientSocket);
void respondRequest404(int clientSocket);
void respondRequest200(int clientSocket);

