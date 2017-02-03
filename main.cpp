#include <stdio.h>
#include <unistd.h>

#include "network.h"
#include "logger.h"
#include "server.h"
#include "dbg.h"


int main(int argc, char* argv[]) {
	int errPipe[2];
	int infoPipe[2];

	if (pipe(errPipe) != 0 || pipe(infoPipe) != 0) {
		fprintf(stderr, "Error creating logging pipes.\n");
		exit(10);
	}

	DBG("Forking");
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
	if (argc >= 2){
		serverRoot = argv[1];
	}
	Server server(serverRoot, 8080, LoggerIn(infoPipe[1]), LoggerIn(errPipe[1]));
	DBG("post server ctor");
	server.runServer();

	return 0;
}
