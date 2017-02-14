#include<string>
#include<stdlib.h>
#include<unistd.h>
#include<boost/filesystem.hpp>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<poll.h>
#include<signal.h>
#include"except.h"
#include"commander.h"
#include"utils.h"

using namespace std;
using namespace boost;

void commanderStart(pid_t mainPid, int fifoFd);
int openFifo(int flags, bool create);


void startCommanderProcess(){
	pid_t mainPid = getpid();

	filesystem::path fifoNameBase = filesystem::path(getenv("HOME")) / ".krait";

	if (!filesystem::exists(fifoNameBase)){
		if (!filesystem::create_directory(fifoNameBase)){
			BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("mkdir(): creating .krait directory (%1%). Check that you have appropriate rights.",
				fifoNameBase.string())
				<< errcodeInfoDef());
		}
	}

	pid_t childPid = fork();
	if (childPid == -1){
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("fork(): starting commander process.") << errcodeInfoDef());
	}
	if (childPid == 0){
		while(getppid() != mainPid){
			int fifoFd = openFifo(O_RDONLY, true);
			if (fifoFd != -1){
				commanderStart(mainPid, fifoFd);
			}
			sleep(1);
		}
		exit(0);
	}
	else{
		return;
	}
}


int openFifo(int flags, bool create){
	filesystem::path fifoNameBase = filesystem::path(getenv("HOME")) / ".krait";
	const char* fifoEnd = "krait_cmdr";
	string fifoName = (fifoNameBase / fifoEnd).string();

	if (!filesystem::exists(fifoNameBase)){
		if (!filesystem::create_directory(fifoNameBase)){
			BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("mkdir(): creating .krait directory (%1%). Check that you have appropriate rights.",
				fifoNameBase.string())
				<< errcodeInfoDef());
		}
	}

	if (!filesystem::exists(fifoName)){
		if (create){
			if (!mkfifo(fifoName.c_str(), 0666)){
				BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("mkfifo(): creating command file (%1%)", fifoName) << errcodeInfoDef());
			}
		}
		else{
			return -1;
		}
	}
	//TODO: IMPORTANT: maybe parent exits while we try to open this... maybe do this on a timeout? maybe open nonblocking?
	int fifoFd = open(fifoName.c_str(), flags | O_NONBLOCK);
	fcntl(fifoFd, F_SETFD, flags);
	if (fifoFd < 0){
		BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("open(): opening command file (%1%)", fifoName) << errcodeInfoDef());		
	}

	return fifoFd;
}

bool processCommand(pid_t mainPid, const char* cmd, int cmdLen);

void commanderStart(pid_t mainPid, int fifoFd){
	//TODO: handle CTRL-C; X, etc.absIdx
	//TODO: IMPORTANT: exit when parent exits!
	char cmdBuffer[1024];

	struct pollfd pfd;
	memzero(pfd);

	pfd.fd = fifoFd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	
	//while not detached
	while(getppid() == mainPid){
		int pollResult = poll(&pfd, 1, 100);
		if (pollResult == -1){
			BOOST_THROW_EXCEPTION(syscallError() << stringInfo("poll(): reading command fifo.") << errcodeInfoDef());
		}
		if (pollResult == 1){
			memzero(cmdBuffer);
			int bytesRead = read(fifoFd, cmdBuffer, 1024);
			if (bytesRead == -1){
				BOOST_THROW_EXCEPTION(syscallError() << stringInfo("read(): reading command fifo.") << errcodeInfoDef());				
			}
			else if (bytesRead == 0){
				//FIFO closed; will reopen.
				close(fifoFd);
				return;
			}
			else {
				if (!processCommand(mainPid, cmdBuffer, bytesRead)){
					BOOST_THROW_EXCEPTION(serverError() << stringInfoFromFormat("Execution of command %1% failed.", string(cmdBuffer, bytesRead)));
				}
			}
		}
	}
}

bool processCommand(pid_t mainPid, const char* cmd, int cmdLen){
	if (cmdLen == 0){
		return true;
	}
	if (cmdLen >= 2 && cmd[0] == '^' && cmd[1] == 'X'){
		kill(mainPid, SIGINT);
		//this is a shutdown request, no other commands to process
		exit(0);
	}
	return false;
}

void sendCommandClose(){
	int fifoFd = openFifo(O_WRONLY, true);
	if (fifoFd == -1){
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Coult not open named pipe to request server shutdown."));
	}
	if (write(fifoFd, "^X", 2) != 2){
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("write: writing shutdown command to named pipe") << errcodeInfoDef());
	}
}