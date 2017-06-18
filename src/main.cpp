#include <unistd.h>
#include <iostream>
#include <string>
#include <boost/program_options.hpp>

#include "network.h"
#include "logger.h"
#include "server.h"
#include "commander.h"

#define DBG_DISABLE
#include "dbg.h"
#include "signalManager.h"

namespace bpo = boost::program_options;

void startSetLoggers(std::string outFilename, std::string errFilename);
//TODO: get the stdout/stderr logger through a pipe too

int main(int argc, char* argv[]) {
	std::string stdoutName;
	std::string stderrName;
	int port;
	std::string siteRoot;

	bpo::options_description genericDesc("Generic options");
	genericDesc.add_options()
			("help,h", "Print help information");

	bpo::options_description mainDesc("Krait options");
	mainDesc.add_options()
			("port,p", bpo::value<int>(&port)->required(), "Set port for server to run on")
			("stdout", bpo::value<std::string>(&stdoutName)->default_value("stdout"), "output logs (default is \"stdout\" for standard output)")
			("stderr", bpo::value<std::string>(&stderrName)->default_value("stderr"), "error logs (default is \"stderr\" for standard error output)");

	bpo::options_description hiddenDesc("Hidden options");
	hiddenDesc.add_options()
			("site-root,r", bpo::value<std::string>(&siteRoot)->required(), "Set website root directory");

	bpo::positional_options_description positionalDesc;
	positionalDesc.add("site-root", 1);

	bpo::options_description cmdlineOptions;
	cmdlineOptions.add(genericDesc).add(mainDesc).add(hiddenDesc);

	bpo::options_description visibleOptions("Krait options");
	visibleOptions.add(genericDesc).add(mainDesc);


	bpo::variables_map parsedArgs;

	try {
		bpo::store(bpo::command_line_parser(argc, argv).options(cmdlineOptions).positional(positionalDesc).run(), parsedArgs);

		if (parsedArgs.count("help") != 0) {
			std::cout << visibleOptions << '\n';
			return 0;
		}

		bpo::notify(parsedArgs);
	}
	catch (boost::program_options::required_option& e) {
		std::cerr << "Invalid arguments: missing option " << e.get_option_name() << std::endl << "Run 'krait --help' for more information" << std::endl;
		return 10;
	}
	catch (boost::program_options::error& e) {
		std::cerr << "Invalid arguments: " << e.what() << std::endl << "Run 'krait --help' for more information" << std::endl;
		return 10;
	}

	if ((stdoutName != "stdout" ? 1 : 0) + (stderrName != "stderr" ? 1 : 0 == 1)) {
		std::cerr << "Invalid arguments: specifying options stdout or stderr to non-default values requires both to be specified." << std::endl <<
				"Run 'krait --help' for more information" << std::endl;
		return 10;
	}

	if (stdoutName != "stdout" && stderrName != "stderr") {
		startSetLoggers(stdoutName, stderrName);
	}


	startCommanderProcess();
	
	SignalManager::registerSignal(std::move(std::unique_ptr<ShtudownSignalHandler>(new ShtudownSignalHandler())));
	SignalManager::registerSignal(std::move(std::unique_ptr<StopSignalHandler>(new StopSignalHandler())));
	SignalManager::registerSignal(std::move(std::unique_ptr<KillSignalHandler>(new KillSignalHandler())));

	Server server(siteRoot, port);
	
	server.runServer();

	Loggers::logInfo("Waiting for children to shut down.");
	SignalManager::waitChildrenBlocking();

	SignalManager::unregisterAll();

	Loggers::logInfo("Krait shutting down. Goodbye.");

	return 0;
}

void startSetLoggers(std::string outFilename, std::string errFilename) {
	int errPipe[2];
	int infoPipe[2];

	if (pipe(errPipe) != 0 || pipe(infoPipe) != 0) {
		std::cerr << "Error creating logging pipes.\n";
		exit(10);
	}

	//DBG("Forking");
	pid_t pid = fork();

	if (pid == -1) {
		std::cerr << "Error fork()ing for the logger\n";
		exit(10);
	}

	if (pid == 0) {
		close(errPipe[1]);
		close(infoPipe[1]);

		LoggerOut iLogger(infoPipe[0], outFilename);
		LoggerOut eLogger(errPipe[0], errFilename);

		loopTick2Loggers(iLogger, eLogger);
		exit(0);
	}

	close(errPipe[0]);
	close(infoPipe[0]);

	Loggers::setLoggers(infoPipe[1], errPipe[1]);

	close(infoPipe[1]);
	close(errPipe[1]);
}
