#include<unistd.h>
#include<stdio.h>
#include<poll.h>
#include"stringPiper.h"
#include"except.h"
#include"limits.h"

using namespace std;

StringPiper::StringPiper(){
	int pipes[2];
	if (pipe(pipes) == -1){
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("Erorr creating pipe for StringPiper") << errcodeInfo(errno));
	}
	readHead = pipes[0];
	writeHead = pipes[1];
}

StringPiper::~StringPiper() {
	closePipes();
}


void StringPiper::closePipes(){
	closeRead();
	closeWrite();
}

void StringPiper::closeRead(){
	if (readHead != -1){
		close(readHead);
		readHead = -1;
	}
}

void StringPiper::closeWrite(){
	if (writeHead != -1){
		close(writeHead);
		writeHead = -1;
	}
}

void StringPiper::pipeWrite(string data) {
	if (data.length() > PIPE_BUF){
		fprintf(stderr, "[WARN] StringPiper: tried to write more than PIPE_BUF at a time; not atomic so weird errors may occur!");
	}
	size_t len = data.length();
	write(writeHead, &len, sizeof(size_t));
	
	write(writeHead, data.c_str(), len);
}

string StringPiper::pipeRead() {
	size_t length;
	if (read(readHead, &length, sizeof(length)) != sizeof(length)){
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("StringPiper::pipeRead: length read() failed") << errcodeInfo(errno));
	}
	
	char* data = new char[length + 1];
	data[length] = '\0';
	
	if (read(readHead, data, length) != (int)length){
		delete data;
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("StringPiper::pipeRead: bulk read() failed") << errcodeInfo(errno));
	}
	
	string result = string(data, length);
	delete data;
	
	return result;
}


bool StringPiper::pipeAvailable() {
	pollfd pfd;
	pfd.fd = readHead;
	pfd.events = POLLIN;
	pfd.revents = 0;
	
	int pollResult = poll(&pfd, 1, 0);
	if (pollResult == -1) {
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("StringPiper::pipeAvailable: poll() failed") << errcodeInfo(errno));
	}
	if (pollResult == 0) {
		return false;
	}
	return true;
}



