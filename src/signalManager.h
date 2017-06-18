#pragma once
#include <signal.h>
#include <vector>
#include <map>
#include <list>
#include "signalHandler.h"

class SignalManager
{
private:
	static std::vector<std::unique_ptr<SignalHandler>> signals;
	static std::map<int, const struct sigaction> oldActions;

	static struct sigaction signalAction;

	static std::list<int> childPids;
public:
	static struct sigaction getSigaction();

	static void registerSignal(std::unique_ptr<SignalHandler>&& signal);
	static void unregisterSignal(const std::unique_ptr<SignalHandler>& signal);

	static void unregisterAll();

	static void signalHandler(int signal, siginfo_t *info, void* ucontext);

	static void addPid(int pid) {
		childPids.push_back(pid);
	}

	static void removePid(int pid);

	static void clearPids() {
		childPids.clear();
	}

	static void signalChildren(int signal);
};
