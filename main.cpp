#include<stdio.h>

#include "network.h"
#include "request.h"

int main(int argc, char* argv[]){
    printf("Waiting on port 8080: ");

    int server = getListenSocket(8080);

    printf("Waiting.\n");
    
    int client = getNewClient(server);
	
	printf("Request got!\n");
    
    Request r = getRequestFromSocket(client);
	
	respondRequest200(client);
	closeSocket(client);

    printf("Done.");
    return 0;
}