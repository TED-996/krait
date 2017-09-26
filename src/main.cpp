#include <unistd.h>
#include <iostream>
#include <string>
#include <boost/program_options.hpp>
#include "logger.h"
#include "server.h"
#include "commander.h"
#include "signalManager.h"
#include "except.h"

#include "dbg.h"

namespace bpo = boost::program_options;

void startSetLoggers(std::string outFilename, std::string errFilename);
bool splitPorts(const std::string& portSpecifier, boost::optional<u_int16_t>* httpPort, boost::optional<u_int16_t>* httpsPort);


int main(int argc, char* argv[]) {
	std::string stdoutName;
	std::string stderrName;
	std::string portSpecifier;
	std::string siteRoot;

	DBG("Krait debug messaging is active.");

	bpo::options_description genericDesc("Generic options");
	genericDesc.add_options()
			("help,h", "Print help information");

	bpo::options_description mainDesc("Krait options");
	mainDesc.add_options()
			("port,p", bpo::value<std::string>(&portSpecifier)->required(),
				"Set ports for server to run on, as <http>/<https> (for example 80/443); one of them may be missing.")
			("stdout", bpo::value<std::string>(&stdoutName)->default_value("stdout"),
				"output logs (default is \"stdout\" for standard output)")
			("stderr", bpo::value<std::string>(&stderrName)->default_value("stderr"),
				"error logs (default is \"stderr\" for standard error output)");

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
			exit(0);
		}

		bpo::notify(parsedArgs);
	}
	catch (boost::program_options::required_option& e) {
		std::cerr << "Invalid arguments: missing option " << e.get_option_name()
			<< std::endl << "Run 'krait --help' for more information" << std::endl;
		exit(10);
	}
	catch (boost::program_options::error& e) {
		std::cerr << "Invalid arguments: " << e.what() << std::endl << "Run 'krait --help' for more information" << std::endl;
		exit(10);
	}

	if ((stdoutName != "stdout" ? 1 : 0) + (stderrName != "stderr" ? 1 : 0 == 1)) {
		std::cerr << "Invalid arguments: specifying options stdout or stderr to non-default values requires both to be specified."
			<< std::endl << "Run 'krait --help' for more information" << std::endl;
		exit(10);
	}

	if (stdoutName != "stdout" && stderrName != "stderr") {
		startSetLoggers(stdoutName, stderrName);
	}


	startCommanderProcess();
	
	SignalManager::registerSignal(std::make_unique<ShtudownSignalHandler>());
	SignalManager::registerSignal(std::make_unique<StopSignalHandler>());
	SignalManager::registerSignal(std::make_unique<KillSignalHandler>());

	boost::optional<u_int16_t> httpPort;
	boost::optional<u_int16_t> httpsPort;

	if (!splitPorts(portSpecifier, &httpPort, &httpsPort)) {
		std::cerr << "Invalid port specifier. Format is <http>/<https>, (for example 80/443); one of them may be missing. "
			"These must be valid ports.";
		exit(10);
	}

	try {
		ArgvConfig config(siteRoot, httpPort, httpsPort);
		Server server(config);

		server.runServer();
	}
	catch(const rootException &ex) {
		Loggers::logErr(ex.what());
		Loggers::logErr("Server terminated.");
		exit(1);
	}

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

	pid_t pid = fork();

	if (pid == -1) {
		std::cerr << "Error foring for the logger\n";
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

bool splitPorts(const std::string& portSpecifier, boost::optional<u_int16_t>* httpPort, boost::optional<u_int16_t>* httpsPort) {
	const char separator = '/';
	size_t sepIdx = portSpecifier.find(separator);

	if (sepIdx == std::string::npos) {
		return false;
	}

	if (sepIdx == 0) {
		*httpPort = boost::none;
	}
	else {
		size_t idx = 0;
		auto result = std::stoul(portSpecifier, &idx);

		//The number should extend all the way to the separator.
		if (idx != sepIdx) {
			return false;
		}
		//The number should fit inside a u_int16_t
		if (result > (1 << 16) - 1) {
			return false;
		}
		*httpPort = result;
	}

	if (sepIdx == portSpecifier.length() - 1) {
		*httpsPort = boost::none;
	}
	else {
		size_t idx = sepIdx + 1;
		auto result = std::stoul(portSpecifier.substr(sepIdx + 1), &idx);
		
		//The number should extend all the way to the end.
		if (idx != portSpecifier.size() - (sepIdx + 1)) {
			return false;
		}
		//The number should fit inside a u_int16_t
		if (result > (1 << 16) - 1) {
			return false;
		}
		*httpsPort = result;
	}

	return true;
}
