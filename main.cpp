#include <stdio.h>
#include <unistd.h>

#include "network.h"
#include "request.h"
#include "logger.h"
#include "server.h"


int main(int argc, char* argv[]) {
	/*printf("Waiting on port 8080: ");

	int server = getServerSocket(8080, true);

	printf("Waiting.\n");

	int client = getNewClient(server, -1);

	printf("Request got!\n");

	Request r = getRequestFromSocket(client);

	respondRequest200(client);
	closeSocket(client);

	printf("Done.");*/
	
	int errPipe[2];
	int infoPipe[2];
	
	if (pipe(errPipe) != 0 || pipe(infoPipe) != 0){
		fprintf(stderr, "Error creating logging pipes.\n");
		exit(10);
	}
	
	printf("Forking\n");
	pid_t pid = fork();
	
	if (pid == -1){
		fprintf(stderr, "Error fork()ing for the logger\n");
		exit(10);
	}
	
	if (pid == 0){
		close(errPipe[1]);
		close(infoPipe[1]);

		LoggerOut iLogger(infoPipe[0], "info.log");
		LoggerOut eLogger(errPipe[0], "err.log");
		
		autoLogger(iLogger, eLogger);
		exit(0);
	}
	
	close(errPipe[0]);
	close(infoPipe[0]);
	
	Server server(".", 8080, LoggerIn(infoPipe[1]), LoggerIn(errPipe[1]));
	server.runServer();
	
	return 0;
}
