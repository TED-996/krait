#include<python2.7/Python.h>
#include<boost/python.hpp>
#include<boost/format.hpp>
#include<string>
#include"pythonWorker.h"
#include"path.h"
#include"except.h"

using namespace boost;
using namespace boost::python;

static object mainGlobal; //Globals I know, but it's the best way without exposing python to main and others.
static object mainModule;


std::string pyErrAsString();


void pythonInit (std::string projectDir) {
	Py_Initialize();

	try {
		mainModule = import("__main__");
		mainGlobal = mainModule.attr("__dict__");

		pythonSetGlobal("project_dir", projectDir);
		pythonSetGlobal("root_dir", getExecRoot().string());

		exec_file(boost::python::str((getExecRoot() / "py" / "python-setup.py").string()), mainGlobal, mainGlobal);
	}
	catch (error_already_set const*) {
		std::string errorString = std::string("Error in initPython:\n") + pyErrAsString();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
}


void pythonSetGlobal(std::string name, std::string value) {
	try {
		mainModule.attr(name.c_str()) = boost::python::str(value);
	}
	catch (error_already_set const*) {
		std::string errorString = (format("Error in pythonSetGlobal(%1%, %2%):\n%3%") % name % value % pyErrAsString()).str();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
}


void pythonRun(std::string command) {
	try {
		exec(boost::python::str(command), mainGlobal, mainGlobal);
	}
	catch (error_already_set const*) {
		std::string errorString = pyErrAsString();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
}


std::string pythonEval(std::string command) {
	try {
		object result = eval(boost::python::str(command), mainGlobal, mainGlobal);
		object resultStr = boost::python::str(result);
		return extract<std::string>(resultStr);
	}
	catch (error_already_set const*) {
		std::string errorString = pyErrAsString();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
}

std::string pyErrAsString() {
	PyObject* exception, *value, *traceback;
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
	object formatted = boost::python::str("\n").join(formatted_list);
	return extract<std::string>(formatted);
}
