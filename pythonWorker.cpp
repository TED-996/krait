#include<python2.7/Python.h>
#include<boost/python.hpp>
#include<string>
#include"python_worker.h"
#include"path.h"
#include"except.h"

using namespace boost;
using namespace boost::python;

static object mainGlobal;


std::string pyErrAsString();


void initPython(std::string projectDir) {
    Py_Initialize();

    try {
        object main = import("__main__");
        mainGlobal = main.attr("__dict__");

        main.attr("project_dir") = str(projectDir);
        main.attr("root_dir") = str(getExecRoot().string());

        exec_file(str((getExecRoot() / "py" / "python-setup.py").string()), mainGlobal, mainGlobal);
    }

    catch(error_already_set const *) {
        std::string errorString = std::string("Error in initPython:\n") + pyErrAsString();
        PyErr_Clear();
        throw pythonError() << stringInfo(errorString);
    }
}


void pythonRun(std::string command) {
    try {
        exec(str(command), mainGlobal, mainGlobal);
    }
    catch(error_already_set const *) {
        std::string errorString = pyErrAsString();
        PyErr_Clear();
        throw pythonError() << stringInfo(errorString);
    }
}

std::string pyErrAsString() {
    PyObject *exception, *value, *traceback;
    object formatted_list;
    PyErr_Fetch(&exception, &value, &traceback);

    handle<> handleException(exception);
    handle<> handleValue(allow_null(value));
    handle<> handleTraceback(allow_null(traceback));

    object moduleTraceback(import("traceback"));

    if (!traceback) {
        object format_exception_only(moduleTraceback.attr("format_exception_only"));
        formatted_list = format_exception_only(handleException, handleValue);
    }
    else {
        object format_exception(moduleTraceback.attr("format_exception"));
        formatted_list = format_exception(handleException, handleValue, handleTraceback);
    }
    object formatted = str("\n").join(formatted_list);
    return extract<std::string>(formatted);
}

std::string pythonEval(std::string command) {
    try {
        object result = eval(str(command), mainGlobal, mainGlobal);
        object resultStr = str(result);
        return extract<std::string>(resultStr);
    }
    catch(error_already_set const *) {
        std::string errorString = pyErrAsString();
        PyErr_Clear();
        throw pythonError() << stringInfo(errorString);
    }
}
