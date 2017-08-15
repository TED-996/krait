#include "except.h"
#include "signalManager.h"
#include <sys/wait.h>

#include "dbg.h"

std::vector<std::unique_ptr<SignalHandler>> SignalManager::signals;
std::map<int, const struct sigaction> SignalManager::oldActions;
struct sigaction SignalManager::signalAction = SignalManager::getSigaction();
std::list<int> SignalManager::childPids;


void SignalManager::registerSignal(std::unique_ptr<SignalHandler>&& signal) {
	std::vector<int> newSignalNumbers = signal->getSignals();

	for (int signalNumber : newSignalNumbers) {
		if (oldActions.find(signalNumber) != oldActions.end()) {
			BOOST_THROW_EXCEPTION(serverError() << stringInfoFromFormat("Signal %1% registered twice.", signalNumber));
		}

		struct sigaction oldAction;
		if (sigaction(signalNumber, &signalAction, &oldAction) != 0) {
			BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("sigaction(): setting signal %1%", signalNumber));
		}

		oldActions.insert(std::make_pair(signalNumber, oldAction));
	}

	signals.push_back(std::move(signal));
}

void SignalManager::unregisterSignal(const std::unique_ptr<SignalHandler>& signal) {
	for (int signalNumber : signal->getSignals()) {
		if (sigaction(signalNumber, &oldActions[signalNumber], nullptr) != 0) {
			BOOST_THROW_EXCEPTION(syscallError() << stringInfoFromFormat("sigaction(): resetting signal %1%", signalNumber));
		}

		oldActions.erase(signalNumber);
	}
}

void SignalManager::unregisterAll() {
	for (const auto& it : signals) {
		unregisterSignal(it);
	}
}

void SignalManager::signalHandler(int signal, siginfo_t* info, void* ucontext) {
	for (auto& it : signals) {
		if (it->handlesSignal(signal)) {
			it->setOldAction(oldActions[signal]);
			it->handler(signal, info, ucontext);
			return; //Uniqueness guaranteed by insertion. Don't even think about double-registering.
		}
	}
}

void SignalManager::removePid(int pid) {
	for (auto it = childPids.begin(); it != childPids.end();) {
		if (*it == pid) {
			it = childPids.erase(it);
		}
		else {
			++it;
		}
	}
}

void SignalManager::signalChildren(int signal) {
	for (const auto pid : childPids) {
		kill(pid, signal);
	}
}

int SignalManager::waitStoppedChildren() {
	if (childPids.size() == 0) {
		return 0;
	}
	int childrenExited = 0;

	int status;
	pid_t pid = -1;
	while (childPids.size() != 0 && (pid = waitpid(-1, &status, WNOHANG)) != 0) {
		if (pid == -1) {
			BOOST_THROW_EXCEPTION(syscallError() << stringInfo("waitpid: waiting for child") << errcodeInfoDef());
		}
		if (WIFEXITED(status) || WIFSIGNALED(status)) {
			DBG_FMT("[Parent] Child %1% exited with status %2%", pid, WEXITSTATUS(status));

			removePid(pid);
			childrenExited++;
		}
	}

	return childrenExited;
}

void SignalManager::waitChildrenBlocking() {
	while (childPids.size() != 0) {
		waitStoppedChildren();
		
		sleep(1);
	}
}

void SignalManager::waitChild(int pid) {
	if (waitpid(pid, NULL, 0) != pid) {
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("waitpid(): waiting for request responder process") << errcodeInfoDef());
	}
	removePid(pid);
}


struct sigaction SignalManager::getSigaction() {
	struct sigaction result;
	memset(&result, 0, sizeof(result));

	result.sa_flags = SA_SIGINFO;
	result.sa_sigaction = &SignalManager::signalHandler;

	return result;
}
