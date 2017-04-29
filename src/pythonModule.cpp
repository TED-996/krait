#include "pythonModule.h"
#include<python2.7/Python.h>
#include<boost/python.hpp>
#include<boost/format.hpp>
#include<string>
#include"path.h"
#include"except.h"

#define DBG_DISABLE
#include"dbg.h"

using namespace std;
using namespace boost;
using namespace boost::python;
namespace bp = boost::python;

PythonModule PythonModule::main("__main__");
PythonModule PythonModule::krait("krait");
PythonModule PythonModule::mvc("mvc");


bool PythonModule::initialized = false;
bp::object PythonModule::requestType;


static PyObject* PythonModule::StringMapToPythonObjectConverter::convert(map<string, string> const& map) {
	DBG("in map<string, string>->PyObject()");

	dict result;
	for (auto it : map) {
		result[bp::str(it.first)] = bp::str(it.second);
	}
	return incref(result.ptr());
}

static PyObject* PythonModule::requestToPythonObjectConverter::convert(Request const& request) {
	DBG("in Request->PyObject()");

	object result = PythonModule::requestType(
			bp::str(httpVerbToString(request.getVerb())),
			bp::str(request.getUrl()),
			bp::str(request.getQueryString()),
			bp::str((format("HTTP/%1%.%2%") % request.getHttpMajor() % request.getHttpMinor()).str()),
			bp::dict(request.getHeaders()),
			bp::str(request.getBody())
	);

	return incref(result.ptr());
}


void PythonModule::initPython(string projectDir) {
	DBG("in initPython()");

	Py_Initialize();
	bp::to_python_converter<Request, requestToPythonObjectConverter>();
	bp::to_python_converter<map<string, string>, StringMapToPythonObjectConverter>();


	try {
		pythonReset(projectDir);
	}
	catch (pythonError& err) {
		DBG("Python error in initPython()!");
		if (string const* errorString = get_error_info<stringInfo>(err)) {
			BOOST_THROW_EXCEPTION(pythonError() << stringInfo(*errorString));
		}
	}
}


void PythonModule::resetPython(string projectDir) {
	DBG("in pythonReset()");
	try {
		if (initialized) {
			PythonModule::main.clear();
			PythonModule::krait.clear();
			PythonModule::mvc.clear();
		}

		PythonModule::main.setGlobal("project_dir", projectDir);
		PythonModule::main.setGlobal("root_dir", getExecRoot().string());

		PythonModule::main.execfile((getExecRoot() / "py" / "python-setup.py").string());

		requestType = PythonModule::krait.moduleGlobals["Request"];
	}
	catch (error_already_set const&) {
		DBG("Python error in resetPython()!");

		string errorString = string("Error in resetPython:\n") + errAsString();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}

	initialized = true;
}



PythonModule::PythonModule(std::string name) {
	this->name = name;
	moduleObject = import(bp::str(name));
	moduleGlobals = extract<dict>(moduleObject.attr("__dict__"));
}

void PythonModule::clear() {
	moduleGlobals.clear();
}

void PythonModule::run(string command) {
	DBG("in run()");
	try {
		exec(bp::str(command), moduleGlobals, moduleGlobals);
	}
	catch (error_already_set const&) {
		DBG("Python error in pythonRun()!");

		string errorString = errAsString();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString) << pyCodeInfo(command));
	}
}

void PythonModule::execfile(std::string filename) {
	DBG("in run()");
	try {
		exec(bp::str(filename), moduleGlobals, moduleGlobals);
	}
	catch (error_already_set const&) {
		DBG("Python error in pythonRun()!");

		string errorString = errAsString();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError()
				                      << stringInfo(errorString)
				                      << pyCodeInfo(formatString("<see %1%>", filename)));
	}
}


string PythonModule::eval(string command) {
	DBG("in pythonEval()");
	try {
		object result = eval(bp::str(command), moduleGlobals, moduleGlobals);
		bp::str resultStr(result);
		return extract<string>(resultStr);
	}
	catch (error_already_set const&) {
		DBG("Python error in pythonEval()!");

		string errorString = errAsString();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString) << pyCodeInfo(command));
	}
}


bool PythonModule::test(std::string condition){
	try{
		object result = eval(bp::str(condition), moduleGlobals, moduleGlobals);
		return (bool)result;
	}
	catch(error_already_set const&){
		DBG("Python error in pythonTest()!");

		string errorString = errAsString();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString) << pyCodeInfo(condition));
	}
}


string PythonModule::errAsString() {
	DBG("in errAsString()");
	try {
		PyObject* exception, *value, *traceback;
		object formatted_list;
		PyErr_Fetch(&exception, &value, &traceback);

		handle<> handleException(exception);
		handle<> handleValue(allow_null(value));
		handle<> handleTraceback(allow_null(traceback));

		object moduleTraceback(import("traceback"));

		DBG("In errAsString: gathered info");

		if (!traceback) {
			object format_exception_only(moduleTraceback.attr("format_exception_only"));
			formatted_list = format_exception_only(handleException, handleValue);
		}
		else {
			object format_exception(moduleTraceback.attr("format_exception"));
			formatted_list = format_exception(handleException, handleValue, handleTraceback);
		}

		DBG("In errAsString: gathered list");

		object formatted = bp::str("\n").join(formatted_list);

		DBG("In errAsString: got formatted str");

		return extract<string>(formatted);
	}
	catch (error_already_set const&) {
		return "We tried to get some Python error info, but we failed.";
	}
}


bool PythonModule::checkIsNone(string name){
	DBG("in pythonVarIsNone(name)");
	try {
		return object(moduleObject.attr(name.c_str())).is_none();
	}
	catch (error_already_set const&) {
		DBG("Python error in checkIsNone(name)!");

		string errorString = (format("Error in checkIsNone(%1%):\n%2%")  % name % errAsString()).str();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
}

void PythonModule::setGlobal(string name, object value) {
	DBG("in setGlobal()");
	try {
		moduleObject.attr(name.c_str()) = value;
	}
	catch (error_already_set const&) {
		DBG("Python error in setGlobal()!");

		string errorString = (format("Error in setGlobal(%1%, python::object):\n%2%") % name % errAsString()).str();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
}


void PythonModule::setGlobal(string name, string value) {
	setGlobal(name, bp::str(value));
}


void PythonModule::setGlobal(string name, map<string, string> value) {
	setGlobal(name, bp::dict(value));
}


void PythonModule::setGlobalRequest(string name, Request value) {
	try {
		setGlobal(name, bp::object(value));
	}
	catch (error_already_set const&) {
		DBG("Python error in setGlobalRequest(), converting Request to PyObject");

		string errorString = (format("Error in setGlobalRequest(%1%, python::object):\n%2%") % name %
		                      errAsString()).str();

		DBG_FMT("In catch: got errorstring %1%", errorString);

		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
}


string PythonModule::getGlobalStr(string name) {
	DBG("in pythonGetGlobalStr()");
	try {
		object globalObj = moduleObject.attr(name.c_str());
		bp::str globalStr(globalObj);
		return extract<string>(globalStr);
	}
	catch (error_already_set const&) {
		DBG("Python error in getGlobalStr()!");

		string errorString = errAsString();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
}

map<string, string> PythonModule::getGlobalMap(string name) {
	DBG("in getGlobalMap()");
	try {
		object globalObj = moduleObject.attr(name.c_str());
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
		DBG("Python error in getGlobalMap()!");

		string errorString = errAsString();
		PyErr_Clear();
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(errorString));
	}
}


//Dedent string
string PythonModule::prepareStr(string pyCode) {
	//DBG_FMT("pythonPrepareStr(%1%)", pyCode);
	if (pyCode.length() == 0 || !isspace(pyCode[0])){
		return pyCode;
	}

	//First remove first line if whitespace.

	const size_t commonIndentSentinel = pyCode.length() + 1;
	size_t commonIndent = commonIndentSentinel;
	size_t outSize = pyCode.length();
	size_t lineIndent = 0;
	int codeLines = 0;
	char indentChr = '\0';
	bool lineWs = true;
	bool inIndent = true;

	for (size_t i = 0; i < pyCode.length(); i++){
		char chr = pyCode[i];
		if (chr == ' ' || chr == '\t'){
			indentChr = chr;
			break;
		}
		else if (chr == '\r' || chr == '\n'){
			continue;
		}
		else{
			break;
		}
	}
	if (indentChr == '\0'){
		return pyCode;
	}

	for (size_t i = 0; i < pyCode.length(); i++){
		char chr = pyCode[i];
		if (chr == '\r' || chr == '\n'){
			if (!lineWs){
				codeLines++;

				if (commonIndent == commonIndentSentinel){
					commonIndent = lineIndent;
				}
				else if (lineIndent < commonIndent) {
					size_t indentDiff = commonIndent - lineIndent;
					outSize += (codeLines - 1) * indentDiff;
					commonIndent = lineIndent;
				}
			}
			outSize -= (lineIndent < commonIndent ? lineIndent : commonIndent);
			lineIndent = 0;
			lineWs = true;
			inIndent = true;
		}
		else if (chr == indentChr && inIndent){
			lineIndent++;
			//DBG("inIndentUp");
		}
		else{
			inIndent = false;
			if (chr != ' ' && chr != '\t'){
				lineWs = false;
			}
		}
	}

	if (!lineWs) {
		codeLines++;

		if (commonIndent == commonIndentSentinel) {
			commonIndent = lineIndent;
		} else if (lineIndent < commonIndent) {
			size_t indentDiff = commonIndent - lineIndent;
			outSize += (codeLines - 1) * indentDiff;
			commonIndent = lineIndent;
		}
	}

	//DBG_FMT("common indent for %1%: %2% of '%3%'", pyCode, commonIndent, indentChr);
	string result;
	result.resize(outSize); //outSize may be a bit larger, this is fine.

	size_t offset = 0;
	lineIndent = 0;
	for (size_t i = 0; i < pyCode.length(); i++){
		char chr = pyCode[i];
		if (chr == '\r' || chr == '\n'){
			lineIndent = 0;
			result[i - offset] = pyCode[i];
		}
		else{
			if (lineIndent < commonIndent){
				lineIndent++;
				offset++;
			}
			else {
				if (offset != 0) {
					result[i - offset] = pyCode[i];
				}
			}
		}
	}

	result.resize(pyCode.length() - offset);

	//DBG_FMT("result: %1%", result);

	return result;
}
