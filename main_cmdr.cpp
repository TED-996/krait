#include <boost/algorithm/string/case_conv.hpp>
#include <string>
#include"commander.h"

using namespace std;

void printUsage();

int main(int argc, char* argv[]){
	if (argc == 1){
		printUsage();
		return 1;
	}
	else if (argc == 2){
		if (string(argv[1]) == "stop"){
			sendCommandClose();
		}
		else if (string(argv[1]) == "kill"){
			sendCommandKill();
		}
		else {
			printUsage();
			return 1;
		}
	}
	else {
		return 1;
	}

	return 0;
}

void printUsage(){
	printf("Usage:\nkrait-cmdr stop: sends graceful close signal to krait.\nkrait-cmdr kill: sends force close signal to krait.\n");
}
