#include <boost/algorithm/string/case_conv.hpp>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include<sys/wait.h>
#include <fcntl.h>
#include"commander.h"
#include"path.h"
#include"except.h"

using namespace std;
namespace fs = boost::filesystem;

void printUsage();
void startKrait(int argc, char* argv[]);
void watchKrait();

int main(int argc, char* argv[]){
	bool argsOk = false;

	if (argc == 2){
		if (string(argv[1]) == "stop"){
			argsOk = true;
			sendCommandClose();
			printf("Stop sent.\n");
		}
		else if (string(argv[1]) == "kill"){
			argsOk = true;
			sendCommandKill();
			printf("Kill sent.\n");
		}
		else if (string(argv[1]) == "watch"){
			argsOk = true;
			watchKrait();
		}
	}
	if (argc >= 2 && string(argv[1]) == "start"){
		argsOk = true;
		startKrait(argc - 2, argv + 2);
	}
	if (!argsOk) {
		printUsage();
		return 1;
	}

	return 0;
}

string getStdoutPath(){
	string dotKrait = getCreateDotKrait();
	return (fs::path(dotKrait) / "stdout").string();
}

string getStderrPath(){
	string dotKrait = getCreateDotKrait();
	return (fs::path(dotKrait) / "stderr").string();
}



void startKrait(int argc, char* argv[]){
	fs::path kraitPath = getExecRoot() / "krait";

	//The daemon way.
	pid_t child1Pid = fork();
	if (child1Pid == -1){
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("fork(): first child failed to start.") << errcodeInfoDef());
	}
	if (child1Pid == 0){
		child1Pid = getpid();
		
		int stdoutFd = open(getStdoutPath().c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
		int stderrFd = open(getStderrPath().c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);

		if (stdoutFd == -1 || stderrFd == -1){
			BOOST_THROW_EXCEPTION(syscallError() << stringInfo("open(): creating krait output.") << errcodeInfoDef());
		}

		if (dup2(stdoutFd, 1) == -1 || dup2(stderrFd, 2) == -1){
			BOOST_THROW_EXCEPTION(syscallError() << stringInfo("dup2(): failed to set stdout/stderr output.") << errcodeInfoDef());
		}
		close(0);
		close(stdoutFd);
		close(stderrFd);

		pid_t child2Pid = fork();
		if (child2Pid == -1){
			BOOST_THROW_EXCEPTION(syscallError() << stringInfo("fork(): second child failed to start.") << errcodeInfoDef());
		}
		if (child2Pid == 0){
			string kraitPath = (getExecRoot() / "krait").string();
			const char** kraitArgs = new const char*[argc + 2];
			kraitArgs[0] = kraitPath.c_str();
			for (int i = 0; i < argc; i++){
				kraitArgs[i + 1] = argv[i];
			}
			kraitArgs[argc + 1] = NULL;
			
			if (execv(kraitPath.c_str(), (char* const*)kraitArgs) == -1){
				BOOST_THROW_EXCEPTION(syscallError() << stringInfo("exec(): could not start Krait"));
			}
			exit(0); //not really useful, just good practice...
		}
		else{
			exit(0);
		}
	}
	else{
		if (waitpid(child1Pid, NULL, 0) == -1){
			BOOST_THROW_EXCEPTION(syscallError() << stringInfo("waitpid: waiting for child1 to exit.") << errcodeInfoDef());
		}
		printf("Krait started.\nWatch stdout in %s and stderr in %s\nor run krait-cmdr watch\n", getStdoutPath().c_str(), getStderrPath().c_str());	
	}
}


void watchKrait(){
	string stdoutPath = getStdoutPath();
	string stderrPath = getStderrPath();
	if (!fs::exists(stdoutPath) || !fs::exists(stderrPath)){
		printf("At least one of the stdout/stderr files don't exist.");
		return;
	}

	const char** args = new const char*[6];
	args[0] = "tail";
	args[1] = "-f";
	args[2] = stdoutPath.c_str();
	args[3] = "-f";
	args[4] = stderrPath.c_str();
	args[5] = NULL;

	if (execvp("tail", (char* const*)args) == -1){
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("exec(): could not start tail."));
	}
}

void printUsage(){
	printf("Usage:\n");
	printf("krait-cmdr start {args}: start krait with given arguments.\n");
	printf("krait-cmdr stop: sends graceful close signal to krait.\n");
	printf("krait-cmdr kill: sends force close signal to krait.\n");
	printf("krait-cmdr watch: watch krait logs (equivalent to \"tail -f {stdout-path} -f {stderr_path}\")\n");
}
