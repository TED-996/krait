#include<string>
#include<stdlib.h>
#include<unistd.h>
#include<boost/filesystem.hpp>
#include<sys/stat.h>
#include<poll.h>
#include<signal.h>
#include"except.h"
#include"commander.h"

using namespace std;
using namespace boost;

void commanderStart(pid_t mainPid, const string& fifoName);
void commanderStart(pid_t mainPid, int fifoFd);

void startCommanderProcess(){
	pid_t mainPid = getpid();

	filesystem::path fifoNameBase = filesystem::path(getenv("HOME")) / ".krait";
	const char* fifoEnd = "krait_cmdr";

	if (!filesystem::exists(fifoNameBase)){
		if (!filesystem::create_directory(fifoNameBase)){
			BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("mkdir(): creating .krait directory (%1%). Check that you have appropriate rights.",
				fifoNameBase.str())
				<< errcodeInfoDef());
		}
	}

	pid_t childPid = fork();
	if (childPid == -1){
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("fork(): starting commander process.") << errcodeInfoDef());
	}
	if (childPid == 0){
		while(getppid() != mainPid){
			commanderStart(mainPid, (fifoNameBase / fifoEnd).str());
		}
		exit(0);
	}
	else{
		return;
	}
}

void commanderStart(pid_t mainPid, const string& fifoName){
	if (!filesystem::exists(fifoName)){
		if (!mkfifo(fifoName.c_str(), 0666)){
			BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("mkfifo(): creating command file (%1%)", fifoName) << errcodeInfoDef());
		}
	}
	//TODO: IMPORTANT: maybe parent exits while we try to open this... maybe do this on a timeout? maybe open nonblocking?
	int fifoFd = open(fifoName, O_RDONLY | O_NONBLOCK);
	if (fifoFd < 0){
		BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("open(): opening command file (%1%)", fifoName) << errcodeInfoDef());		
	}

	commanderStart(mainPid, fifoFd);
}

bool processCommand(const char* cmd, int cmdLen);

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
			int bytesRead = read(fifoFd, cmdBuffer);
			if (bytesRead == -1){
				BOOST_THROW_EXCEPTION(syscallError() << stringInfo("read(): reading command fifo.") << errcodeInfoDef());				
			}
			else if (bytesRead == 0){
				//FIFO closed; will reopen.
				return;
			}
			else {
				if (!processCommand(cmdBuffer, bytesRead)){
					BOOST_THROW_EXCEPTION(serverError() << stringInfoFromFormat("Execution of command %1% failed.", cmdBuffer));
				}
			}
		}
	}
}

bool processCommand(const char* cmd, int cmdLen){
	if (cmdLen == 0){
		return true;
	}
	if (cmdLen >= 2 && cmd[0] == '^' && cmd[1] == 'X'){
		kill(mainPid, SIGUSR1);
		return processCommand(cmd + 2, cmdLen - 2);
	}
	return false;
}