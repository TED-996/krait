#pragma once
#include <signal.h>
#include <stack>

class RaiiAlarm {
    static RaiiAlarm* instance;
    static void handler(int signal);
    static struct sigaction* getSigaction(bool setSigInterrupt);

    bool finished;
    struct sigaction oldAction;

public:
    RaiiAlarm(int timeoutSeconds, bool setSigInterrupt);
    ~RaiiAlarm();

    bool isFinished() const {
        return finished;
    }
};
