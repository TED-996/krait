#include<python2.7/Python.h>
#include<boost/python.hpp>
#include<boost/format.hpp>
#include<string>
#include"pythonWorker.h"
#include"path.h"
#include"except.h"

using namespace std;
using namespace boost;
using namespace boost::python;
namespace bp = boost::python;

static dict mainGlobal; //Globals I know, but it's the best way without exposing python to main and others.
static object mainModule;
static bool alreadySet = false;


string pyErrAsString();


struct Request_to_python_obj {
	static PyObject* convert(Request const& request){
		object result;
		
		result.attr("http_method") = httpVerbToString(request.getVerb());
		result.attr("url") = bp::str(request.getUrl());
		result.attr("http_version") = bp::str(
			(format("HTTP%1%/%2%") % request.getHttpMajor() % request.getHttpMinor()));
		result.attr("headers") = bp::dict(request.getHeaders());
		result.attr("body") = bp::str(request.getBody());
		
		return incref(result.ptr());
	}
};


void pythonInit (string projectDir) {
	Py_Initialize();
	bp::to_python_converter<Request, Request_to_python_obj>();


	try {
		pythonReset(projectDir);
	}
	catch (pythonError &err){
		if (string const* errorString = get_error_info<stringInfo>(err)){
			BOOST_THROW_EXCEPTION(pythonError() << stringInfo(*errorString));
		}
	}
}


void pythonReset(string projectDir){
	try {
		if (alreadySet){
			mainGlobal.clear();
		}
		
		mainModule = import("__main__");
		mainGlobal = extract<dict>(mainModule.attr("__dict__"));

		pythonSetGlobal ("project_dir", projectDir);
		pythonSetGlobal ("root_dir", getExecRoot().string());


		exec_file(bp::str((getExecRoot() / "py" / "python-setup.py").string()), mainGlobal, mainGlobal);
	}
	catch (error_already_set const*) {
		string errorString = string("Error in pythonReset:\n") + pyErrAsString();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
	
	alreadySet = true;
}


void pythonRun(string command) {
	try {
		exec(bp::str(command), mainGlobal, mainGlobal);
	}
	catch (error_already_set const*) {
		string errorString = pyErrAsString();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
}


string pythonEval(string command) {
	try {
		object result = eval(bp::str(command), mainGlobal, mainGlobal);
		object resultStr = bp::str(result);
		return extract<string>(resultStr);
	}
	catch (error_already_set const*) {
		string errorString = pyErrAsString();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
}

string pyErrAsString() {
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
	object formatted = bp::str("\n").join(formatted_list);
	return extract<string>(formatted);
}


void pythonSetGlobal (string name, object value) {
	try {
		mainModule.attr(name.c_str()) = value;
	}
	catch (error_already_set const*) {
		string errorString = (format("Error in pythonSetGlobal(%1%, python::object):\n%3%") % name % pyErrAsString()).str();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
}


void pythonSetGlobal (string name, string value) {
	pythonSetGlobal(name, bp::str(value));
}


void pythonSetGlobal(string name, map<string, string> value) {
	pythonSetGlobal(name, bp::dict(value));
}


void pythonSetGlobalRequest(string name, Request value){
	pythonSetGlobal(name, bp::object(value));
}

string pythonGetGlobalStr(string name) {
	try{
		object globalObj = mainModule.attr(name.c_str());
		bp::str globalStr(globalObj);
		return extract<string>(globalStr);
	}
	catch (error_already_set const*) {
		string errorString = pyErrAsString();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
}


map<string, string> pythonGetGlobalMap(string name){
	try{
		object globalObj = mainModule.attr(name.c_str());
		dict globalDict = extract<dict>(globalObj);
		bp::list dictKeys = globalDict.keys();
		int nrKeys = len(dictKeys);
		
		map<string, string> result;
		for (int i = 0; i < nrKeys; i++){
			object key = dictKeys[i];
			result[extract<string>(bp::str(key))] = extract<string>(bp::str(globalDict[key]));
		}
		return result;
	}
	catch (error_already_set const*) {
		string errorString = pyErrAsString();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
}
