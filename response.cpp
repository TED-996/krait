#include "response.h"

using namespace std;


Response::Response(int httpMajor, int httpMinor, int httpCode, unordered_map<string, string> headers, string body) {

}


void Response::addHeader(string name, string value) {
	auto headerIt = headers.find(name);

	if (headerIt == headers.end()) {
		headers.insert(make_pair(name, value));
	}
	else {
		headers[name] = headers[name] + "," + value;
	}
}
