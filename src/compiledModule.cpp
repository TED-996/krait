#include "compiledModule.h"


void CompiledModule::reload() {
    BOOST_THROW_EXCEPTION(notImplementedError() << stringInfo("CompiledModule::reload"));
}
