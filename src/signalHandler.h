#pragma once
#include <signal.h>
#include <vector>

class SignalHandler
{
private:
	std::vector<int> signals;
	struct sigaction oldAction;

	static void handleDefaultSignal(int signal, struct sigaction oldAction);
protected:
	void callOldHandler(int signal, siginfo_t* info, void* ucontext) const;
public:
	explicit SignalHandler(std::vector<int> signals);
	virtual ~SignalHandler() = default;

	virtual void handler(int signal, siginfo_t *info, void* ucontext) = 0;

	std::vector<int> getSignals() const {
		return signals;
	}

	bool handlesSignal(int signal);

	const struct sigaction& getOldAction() const {
		return oldAction;
	}


	void setOldAction(const struct sigaction& oldAction) {
		this->oldAction = oldAction;
	}

};

class ShtudownSignalHandler : public SignalHandler
{
public:
	ShtudownSignalHandler()
		: SignalHandler(std::vector<int>{SIGUSR2}) {
	}

	void handler(int signal, siginfo_t* info, void* ucontext) override;
};


class StopSignalHandler : public SignalHandler
{
public:
	StopSignalHandler()
		: SignalHandler(std::vector<int>{SIGUSR1, SIGINT}) {
	}

	void handler(int signal, siginfo_t* info, void* ucontext) override;
};

class KillSignalHandler : public SignalHandler
{
public:
	KillSignalHandler()
		: SignalHandler(std::vector<int>{SIGTERM}) {
	}

	void handler(int signal, siginfo_t* info, void* ucontext) override;
};