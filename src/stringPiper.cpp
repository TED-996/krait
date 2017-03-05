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
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("pipe(): creating pipe for StringPiper") << errcodeInfoDef());
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
	string writeData = string((char*)&len, sizeof(len)) + data;
	
	write(writeHead, writeData.c_str(), writeData.length());
}

string StringPiper::pipeRead() {
	size_t length;
	if (read(readHead, &length, sizeof(length)) != sizeof(length)){
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("read(): reading length in StringPiper::pipeRead") << errcodeInfoDef());
	}
	
	char* data = new char[length + 1];
	data[length] = '\0';
	
	if (read(readHead, data, length) != (int)length){
		delete[] data;
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("read(): reading bulk in StringPiper::pipeRead") << errcodeInfoDef());
	}
	
	string result = string(data, length);
	delete[] data;
	
	return result;
}


bool StringPiper::pipeAvailable() {
	pollfd pfd;
	pfd.fd = readHead;
	pfd.events = POLLIN;
	pfd.revents = 0;
	
	int pollResult = poll(&pfd, 1, 0);
	if (pollResult == -1) {
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("poll(): getting StringPiper::pipeAvailable") << errcodeInfoDef());
	}
	if (pollResult == 0) {
		return false;
	}
	return true;
}


