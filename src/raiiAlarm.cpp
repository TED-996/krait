#include "raiiAlarm.h"
#include <unistd.h>
#include <signal.h>
#include "except.h"


//Some optimizing care has been put into this file, this may be critical code.


RaiiAlarm* RaiiAlarm::instance = nullptr;


RaiiAlarm::RaiiAlarm(int timeoutSeconds, bool setSigInterrupt) {
	finished = false;

	if (RaiiAlarm::instance != nullptr) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Multiple concurrent RaiiAlarms!"));
	}
	RaiiAlarm::instance = this;

	if (BOOST_UNLIKELY(sigaction(SIGALRM, getSigaction(setSigInterrupt), &oldAction) != 0)) {
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("sigaction(): setting RaiiAlarm") << errcodeInfoDef());
	}
	alarm(timeoutSeconds); //This cannot fail.
}

RaiiAlarm::~RaiiAlarm() {
	alarm(0); //This cannot fail.

	if (BOOST_UNLIKELY(sigaction(SIGALRM, &oldAction, nullptr) != 0)) {
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("sigaction(): removing RaiiAlarm") << errcodeInfoDef());
	}

	RaiiAlarm::instance = nullptr;
}

void RaiiAlarm::handler(int signal) {
	if (RaiiAlarm::instance == nullptr) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("RaiiAlarm handler called with destructed instance."));
	}
	RaiiAlarm::instance->finished = true;
}

struct sigaction* RaiiAlarm::getSigaction(bool setSigInterrupt) {
	static bool initialized = false;
	static struct sigaction action;

	//There might be a better way. Not that it actually matters.
	if (BOOST_UNLIKELY(!initialized)) {
		memset(&action, 0, sizeof(action));
		action.sa_handler = RaiiAlarm::handler;
		action.sa_flags = 0; //Not explicitly necessary, though.
		sigemptyset(&action.sa_mask);

		initialized = true;
	}

	action.sa_flags = setSigInterrupt ? 0 : SA_RESTART;

	return &action;
}

