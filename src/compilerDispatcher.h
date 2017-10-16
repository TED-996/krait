#pragma once
#include "moduleCompiler.h"
#include "pymlCache.h"

class CompilerDispatcher
{
	ModuleCompiler compiler;
	PymlCache& pymlCache;
public:

	const ModuleCompiler& getCompiler() const {
		return compiler;
	}

	PymlCache& getPymlCache() const {
		return pymlCache;
	}
};
