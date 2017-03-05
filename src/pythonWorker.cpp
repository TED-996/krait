#include<python2.7/Python.h>
#include<boost/python.hpp>
#include<boost/format.hpp>
#include<string>
#include"pythonWorker.h"
#include"path.h"
#include"except.h"

#define DBG_DISABLE
#include"dbg.h"

using namespace std;
using namespace boost;
using namespace boost::python;
namespace bp = boost::python;

static dict mainGlobal; //Globals I know, but it's the best way without exposing python to main and others.
static object mainModule;
static object requestType;

static bool alreadySet = false;


string pyErrAsString();

struct String_map_to_python_obj {
	static PyObject* convert(map<string, string> const& map) {
		DBG("in map<string, string>->PyObject()");

		dict result;
		for (auto it : map) {
			result[bp::str(it.first)] = bp::str(it.second);
		}
		return incref(result.ptr());
	}
};

struct Request_to_python_obj {
	static PyObject* convert(Request const& request) {
		DBG("in Request->PyObject()");

		object result = requestType(
		                    bp::str(httpVerbToString(request.getVerb())),
		                    bp::str(request.getUrl()),
							bp::str(request.getQueryString()),
		                    bp::str((format("HTTP/%1%.%2%") % request.getHttpMajor() % request.getHttpMinor()).str()),
		                    bp::dict(request.getHeaders()),
		                    bp::str(request.getBody())
		                );

		return incref(result.ptr());
	}
};


void pythonInit(string projectDir) {
	DBG("in pythonInit()");

	Py_Initialize();
	bp::to_python_converter<Request, Request_to_python_obj>();
	bp::to_python_converter<map<string, string>, String_map_to_python_obj>();


	try {
		pythonReset(projectDir);
	}
	catch (pythonError& err) {
		DBG("Python error in pythonInit()!");
		if (string const* errorString = get_error_info<stringInfo>(err)) {
			BOOST_THROW_EXCEPTION(pythonError() << stringInfo(*errorString));
		}
	}
}


void pythonReset(string projectDir) {
	DBG("in pythonReset()");
	try {
		if (alreadySet) {
			mainGlobal.clear();
		}

		mainModule = import("__main__");
		mainGlobal = extract<dict>(mainModule.attr("__dict__"));

		pythonSetGlobal("project_dir", projectDir);
		pythonSetGlobal("root_dir", getExecRoot().string());

		exec_file(bp::str((getExecRoot() / "py" / "python-setup.py").string()), mainGlobal, mainGlobal);

		requestType = mainGlobal["Request"];
	}
	catch (error_already_set const&) {
		DBG("Python error in pythonReset()!");

		string errorString = string("Error in pythonReset:\n") + pyErrAsString();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}

	alreadySet = true;
}


void pythonRun(string command) {
	DBG("in pythonRun()");
	try {
		exec(bp::str(command), mainGlobal, mainGlobal);
	}
	catch (error_already_set const&) {
		DBG("Python error in pythonRun()!");

		string errorString = pyErrAsString();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
}


string pythonEval(string command) {
	DBG("in pythonEval()");
	try {
		object result = eval(bp::str(command), mainGlobal, mainGlobal);
		bp::str resultStr(result);
		return extract<string>(resultStr);
	}
	catch (error_already_set const&) {
		DBG("Python error in pythonEval()!");

		string errorString = pyErrAsString();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
}


bool pythonTest(std::string condition){
	try{
		object result = eval(bp::str(condition), mainGlobal, mainGlobal);
		return (bool)result;
	}
	catch(error_already_set const&){
		DBG("Python error in pythonTest()!");
		
		string errorString = pyErrAsString();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
}


string pyErrAsString() {
	DBG("in pyErrAsString()");
	try {
		PyObject* exception, *value, *traceback;
		object formatted_list;
		PyErr_Fetch(&exception, &value, &traceback);

		handle<> handleException(exception);
		handle<> handleValue(allow_null(value));
		handle<> handleTraceback(allow_null(traceback));

		object moduleTraceback(import("traceback"));

		DBG("In pyErrAsString: gathered info");

		if (!traceback) {
			object format_exception_only(moduleTraceback.attr("format_exception_only"));
			formatted_list = format_exception_only(handleException, handleValue);
		}
		else {
			object format_exception(moduleTraceback.attr("format_exception"));
			formatted_list = format_exception(handleException, handleValue, handleTraceback);
		}

		DBG("In pyErrAsString: gathered list");

		object formatted = bp::str("\n").join(formatted_list);

		DBG("In pyErrAsString: got formatted str");

		return extract<string>(formatted);
	}
	catch (error_already_set const&) {
		return "We tried to get some Python error info, but we failed.";
	}
}

bool pythonVarIsNone(string name){
	DBG("in pythonVarIsNone()");
	try {
		return object(mainModule.attr(name.c_str())).is_none();
	}
	catch (error_already_set const&) {
		DBG("Python error in pythonSetGlobal()!");

		string errorString = (format("Error in pythonSetGlobal(%1%, python::object):\n%2%") % name % pyErrAsString()).str();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
}


void pythonSetGlobal(string name, object value) {
	DBG("in pythonSetGlobal()");
	try {
		mainModule.attr(name.c_str()) = value;
	}
	catch (error_already_set const&) {
		DBG("Python error in pythonSetGlobal()!");

		string errorString = (format("Error in pythonSetGlobal(%1%, python::object):\n%2%") % name % pyErrAsString()).str();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
}


void pythonSetGlobal(string name, string value) {
	pythonSetGlobal(name, bp::str(value));
}


void pythonSetGlobal(string name, map<string, string> value) {
	pythonSetGlobal(name, bp::dict(value));
}


void pythonSetGlobalRequest(string name, Request value) {
	try {
		pythonSetGlobal(name, bp::object(value));
	}
	catch (error_already_set const&) {
		DBG("Python error in pythonSetGlobalRequest(), converting Request to PyObject");

		string errorString = (format("Error in pythonSetGlobalRequest(%1%, python::object):\n%2%") % name %
		                      pyErrAsString()).str();

		DBG_FMT("In catch: got errorstring %1%", errorString);

		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
}

string pythonGetGlobalStr(string name) {
	DBG("in pythonGetGlobalStr()");
	try {
		object globalObj = mainModule.attr(name.c_str());
		bp::str globalStr(globalObj);
		return extract<string>(globalStr);
	}
	catch (error_already_set const&) {
		DBG("Python error in pythonGetGlobalStr()!");

		string errorString = pyErrAsString();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
}


map<string, string> pythonGetGlobalMap(string name) {
	DBG("in pythonGetGlobalMap()");
	try {
		object globalObj = mainModule.attr(name.c_str());
		dict globalDict = extract<dict>(globalObj);
		bp::list dictKeys = globalDict.keys();
		int nrKeys = len(dictKeys);

		map<string, string> result;
		for (int i = 0; i < nrKeys; i++) {
			object key = dictKeys[i];
			result[extract<string>(bp::str(key))] = extract<string>(bp::str(globalDict[key]));
		}
		return result;
	}
	catch (error_already_set const&) {
		DBG("Python error in pythonGetGlobalMap()!");

		string errorString = pyErrAsString();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
}
