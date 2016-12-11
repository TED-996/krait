#include<cstdio>

#include "network.h"


int main(int argc, char* argv[]){
    printf("Waiting on port 8080: ");

    int server = getListenSocket(8080);

    printf("Waiting.\n");
    
    int client = getNewClient(server);
    
    printSocket(client);

    printf("Done.");
    return 0;
}