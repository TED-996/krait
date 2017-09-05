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

namespace b = boost;
namespace bf = boost::filesystem;

void commanderStart(pid_t mainPid, int fifoFd);
int openFifo(int flags, bool create);


void startCommanderProcess() {
	pid_t mainPid = getpid();

	getCreateDotKrait();

	pid_t childPid = fork();
	if (childPid == -1) {
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("fork(): starting commander process.") << errcodeInfoDef());
	}
	if (childPid == 0) {
		while (getppid() == mainPid) {
			int fifoFd = openFifo(O_RDONLY, true);
			if (fifoFd != -1) {
				commanderStart(mainPid, fifoFd);
			}
			sleep(1);
		}
		printf("Commander process shutting down.\n");
		exit(0);
	}
	else {
		return;
	}
}

std::string getCreateDotKrait() {
	bf::path fifoNameBase = bf::path("/run");
	if (!bf::exists(fifoNameBase)) {
		if (!bf::create_directory(fifoNameBase)) {
			BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("mkdir(): creating command directory %1%. Check that you have appropriate rights.",
					fifoNameBase.string())
				<< errcodeInfoDef());
		}
	}

	return fifoNameBase.string();
}


int openFifo(int flags, bool create) {
	bf::path fifoNameBase = bf::path(getCreateDotKrait());
	const char* fifoEnd = "krait_cmdr";
	std::string fifoName = (fifoNameBase / fifoEnd).string();

	if (!bf::exists(fifoName)) {
		if (create) {
			if (!mkfifo(fifoName.c_str(), 0666)) {
				BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("mkfifo(): creating command file (%1%)", fifoName) << errcodeInfoDef());
			}
		}
		else {
			return -1;
		}
	}
	//TODO: IMPORTANT: maybe parent exits while we try to open this... maybe do this on a timeout? maybe open nonblocking?
	int fifoFd = open(fifoName.c_str(), flags | O_NONBLOCK);
	if (fifoFd < 0) {
		if (errno == ENXIO) {
			BOOST_THROW_EXCEPTION(cmdrError() << stringInfoFromFormat("There is no running server listening on command file."));
		}
		else {
			BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("open(): opening command file (%1%)", fifoName) << errcodeInfoDef());
		}
	}
	if (fcntl(fifoFd, F_SETFD, flags) < 0) {
		BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("fcntl(): opening command file (%1%)", fifoName) << errcodeInfoDef());
	}


	return fifoFd;
}

bool processCommand(pid_t mainPid, const char* cmd, int cmdLen);

void commanderStart(pid_t mainPid, int fifoFd) {
	//TODO: handle CTRL-C; X, etc.absIdx
	//TODO: IMPORTANT: exit when parent exits!
	char cmdBuffer[1024];

	struct pollfd pfd;
	memzero(pfd);

	pfd.fd = fifoFd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	//while not detached
	while (getppid() == mainPid) {
		int pollResult = poll(&pfd, 1, 100);
		if (pollResult == -1) {
			BOOST_THROW_EXCEPTION(syscallError() << stringInfo("poll(): reading command fifo.") << errcodeInfoDef());
		}
		if (pollResult == 1) {
			memzero(cmdBuffer);
			int bytesRead = read(fifoFd, cmdBuffer, 1024);
			if (bytesRead == -1) {
				BOOST_THROW_EXCEPTION(syscallError() << stringInfo("read(): reading command fifo.") << errcodeInfoDef());
			}
			else if (bytesRead == 0) {
				//FIFO closed; will reopen.
				close(fifoFd);
				return;
			}
			else {
				if (!processCommand(mainPid, cmdBuffer, bytesRead)) {
					BOOST_THROW_EXCEPTION(cmdrError() << stringInfoFromFormat("Execution of command %1% failed.", std::string(cmdBuffer, bytesRead)));
				}
			}
		}
	}
}

bool processCommand(pid_t mainPid, const char* cmd, int cmdLen) {
	if (cmdLen == 0) {
		return true;
	}
	if (cmdLen >= 2 && cmd[0] == '^' && cmd[1] == 'X') {
		kill(mainPid, SIGINT);
		return processCommand(mainPid, cmd + 2, cmdLen - 2);
	}
	if (cmdLen >= 2 && cmd[0] == '^' && cmd[1] == 'K') {
		kill(mainPid, SIGTERM);
		return processCommand(mainPid, cmd + 2, cmdLen - 2);
	}
	return false;
}
