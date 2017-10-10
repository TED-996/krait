#pragma once
#include "fsmV2.h"

class PythonCodeEscapingFsm : FsmV2
{
public:
	enum class PythonCodeEscapingState {
		Statement,
		String,
		MultilineString,
		Comment
	};

	PythonCodeEscapingFsm();

	void init();
	PythonCodeEscapingState getCodeState();
};
