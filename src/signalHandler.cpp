#include "signalHandler.h"
#include "logger.h"
#include "server.h"
#include "signalManager.h"
#include <cstring>

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

void SignalHandler::callOldHandler(int signal, siginfo_t* info, void* ucontext) const {
    if (oldAction.sa_flags & SA_SIGINFO) {
        oldAction.sa_sigaction(signal, info, ucontext);
    } else {
        if (oldAction.sa_handler == SIG_IGN) {
        } else if (oldAction.sa_handler == SIG_DFL) {
            handleDefaultSignal(signal, oldAction);
        } else {
            oldAction.sa_handler(signal);
        }
    }
}

void SignalHandler::handleDefaultSignal(int signal, struct sigaction oldAction) {
    if (signal == SIGTERM || signal == SIGINT || signal == SIGQUIT || signal == SIGKILL || signal == SIGHUP) {
        Loggers::logErr(
            formatString("[SIGNAL]: Received signal %1%, no traceback info available at the moment.", signal));
    }
    struct sigaction currentAction;
    if (sigaction(signal, &oldAction, &currentAction) != 0) {
        BOOST_THROW_EXCEPTION(
            syscallError() << stringInfoFromFormat("sigaction(): before default call, setting handler to default"));
    }
    raise(signal);
    if (sigaction(signal, &currentAction, nullptr) != 0) {
        BOOST_THROW_EXCEPTION(syscallError()
            << stringInfoFromFormat("sigaction(): after default call, resetting handler to user-defined"));
    }
}

void ShtudownSignalHandler::handler(int signal, siginfo_t* info, void* ucontext) {
    if (Server::getInstance() != nullptr) {
        Server::getInstance()->requestShutdown();
    }

    SignalManager::signalChildren(signal);

    Loggers::logInfo(formatString("Process %1% shutting down...", getpid()));
}

void StopSignalHandler::handler(int signal, siginfo_t* info, void* ucontext) {
    Loggers::logInfo(formatString("Stop requested for process %1%", getpid()));

    if (Server::getInstance() != nullptr) {
        Server::getInstance()->cleanup();
        Server::getInstance()->~Server();
    }

    SignalManager::signalChildren(signal);
    SignalManager::waitChildrenBlocking();

    Loggers::logInfo(formatString("Process %1% stopped.", getpid()));

    exit(0);
}

void KillSignalHandler::handler(int signal, siginfo_t* info, void* ucontext) {
    Loggers::logInfo(formatString("Force stop requested for process %1%", getpid()));

    if (Server::getInstance() != nullptr) {
        Server::getInstance()->cleanup();
        Server::getInstance()->~Server();
    }

    SignalManager::signalChildren(signal);

    Loggers::logInfo(formatString("Process %1% force stopped.", getpid()));

    exit(0);
}

void SigPipeLogHandler::handler(int signal, siginfo_t* info, void* ucontext) {
    Loggers::logInfo("Client disconnected during write.");
}
