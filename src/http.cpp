#include<string>
#include"http.h"

using namespace std;

string httpVerbToString(HttpVerb value) {
	const char* verbStrings[] {
		"ANY",
		"GET",
		"HEAD",
		"POST",
		"PUT",
		"DELETE",
		"CONNECT",
		"OPTIONS",
		"TRACE"
	};

	return string(verbStrings[(int)value]);
}
