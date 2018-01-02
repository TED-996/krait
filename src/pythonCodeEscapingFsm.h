#pragma once
#include "fsmV2.h"

// In this context, "Escaped" means that characters aren't to be interpreted as literal Python statements
class PythonCodeEscapingFsm : public FsmV2 {
public:
    enum class PythonCodeEscapingState { Statement, String, MultilineString, Comment };

    PythonCodeEscapingFsm();

    void init();

    PythonCodeEscapingState getCodeState();
    bool isEscaped() {
        return getCodeState() != PythonCodeEscapingState::Statement;
    }
    bool isMultilineString() {
        return getCodeState() == PythonCodeEscapingState::MultilineString;
    }
    char getMultilineQuoteChar();
};
