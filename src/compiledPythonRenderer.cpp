#include "compiledPythonRenderer.h"
#include "except.h"
#include "pyCompileModule.h"
#include "pythonModule.h"


boost::python::object CompiledPythonRenderer::run(boost::string_ref name) {
    PythonModule module;
    try {
        module.open(name);
    } catch (const pythonError& err) {
        throw;
    }
    module.addToCache();

    size_t tryIdx = 0;
    while (tryIdx++ < 3) {
        try {
            return module.callGlobal("run");
        } catch (const pythonError& err) {
            auto* excType = boost::get_error_info<pyExcTypeInfo>(err);
            if (excType != nullptr && PyCompileModule::getExceptionType().ptr() == (*excType)().ptr()) {
                // Is reload
                // The PythonModule should already be reloaded.
                continue;
            } else {
                throw;
            }
        }
    }

    BOOST_THROW_EXCEPTION(compileError() << stringInfoFromFormat("Too many reloads for Python module %1%", name));
}
