#include <stdio.h>
#include <unistd.h>
#include <string>

#include "network.h"
#include "logger.h"
#include "server.h"
#include "commander.h"
#include "dbg.h"

using namespace std;

int main(int argc, char* argv[]) {
	int errPipe[2];
	int infoPipe[2];

	if (pipe(errPipe) != 0 || pipe(infoPipe) != 0) {
		fprintf(stderr, "Error creating logging pipes.\n");
		exit(10);
	}

	startCommanderProcess();

	//DBG("Forking");
	pid_t pid = fork();

	if (pid == -1) {
		fprintf(stderr, "Error fork()ing for the logger\n");
		exit(10);
	}

	if (pid == 0) {
		close(errPipe[1]);
		close(infoPipe[1]);

		LoggerOut iLogger(infoPipe[0], "info.log");
		LoggerOut eLogger(errPipe[0], "err.log");

		loopTick2Loggers(iLogger, eLogger);
		exit(0);
	}

	close(errPipe[0]);
	close(infoPipe[0]);

	DBG("Pre server ctor");
	const char* serverRoot = "testserver";
	int port = 8080;
	if (argc >= 2){
		serverRoot = argv[1];
	}
	if (argc >= 3){
		size_t end = 0;
		port = stoi(string(argv[2]), &end);
		if (end != string(argv[2]).length()){
			BOOST_THROW_EXCEPTION(serverError() << stringInfo("Server port is not an integer."));
		}
	}
	if (argc >= 4){
		printf("Too many arguments.\nUsage:\nkrait root_directory [port]\n");
		exit(10);
	}
	Server server(serverRoot, port, LoggerIn(infoPipe[1]), LoggerIn(errPipe[1]));
	DBG("post server ctor");
	server.runServer();

	return 0;
}
