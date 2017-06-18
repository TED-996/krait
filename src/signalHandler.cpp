#include "signalHandler.h"
#include <cstring>
#include "server.h"

SignalHandler::SignalHandler(std::vector<int> signals) {
	this->signals = signals;
	memset(&this->oldAction, 0, sizeof(this->oldAction));
}

bool SignalHandler::handlesSignal(int signal) {
	for (const auto& it : signals) {
		if (signal == it) {
			return true;
		}
	}
	return false;
}

void SignalHandler::callOldHandler(int signal, siginfo_t* info, void* ucontext) {
	if (oldAction.sa_flags & SA_SIGINFO) {
		oldAction.sa_sigaction(signal, info, ucontext);
	}
	else {
		if (oldAction.sa_handler == SIG_IGN) {
		}
		else if (oldAction.sa_handler == SIG_DFL) {
			handleDefaultSignal(signal, oldAction);
		}
		else {
			oldAction.sa_handler(signal);
		}
	}
}

void SignalHandler::handleDefaultSignal(int signal, struct sigaction oldAction) {
	if (signal == SIGTERM || signal == SIGINT || signal == SIGQUIT || signal == SIGKILL || signal == SIGHUP) {
		//TODO: print a traceback or something
	}
	struct sigaction currentAction;
	if (sigaction(signal, &oldAction, &currentAction) != 0) {
		BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("sigaction(): before default call, setting handler to default"));
	}
	raise(signal);
	if (sigaction(signal, &currentAction, NULL) != 0) {
		BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("sigaction(): after default call, resetting handler to user-defined"));
	}
}

void ShtudownSignalHandler::handler(int signal, siginfo_t* info, void* ucontext) {
}

void StopSignalHandler::handler(int signal, siginfo_t* info, void* ucontext) {
	Server::instance.cleanup();
	Server::instance.~Server();

	exit(0);
}

void KillSignalHandler::handler(int signal, siginfo_t* info, void* ucontext) {
	Server::instance.~Server();

	callOldHandler(signal, info, ucontext);
}
