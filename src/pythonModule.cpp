#include "pythonModule.h"
#include<boost/python.hpp>
#include<boost/format.hpp>
#include<string>
#include<cstdlib>
#include"path.h"
#include"except.h"

#define DBG_DISABLE
#include"dbg.h"
#include "routes.h"

namespace b = boost;
namespace bp = boost::python;


bool PythonModule::pythonInitialized = false;
bool PythonModule::modulesInitialized = false;
bp::object PythonModule::requestType;


void PythonModule::initPython() {
	DBG("initializing python");
	if (pythonInitialized) {
		return;
	}
	try {
		Py_Initialize();
		DBG("PyInitialize done");
		bp::to_python_converter<Request, requestToPythonObjectConverter>();
		bp::to_python_converter<std::map<std::string, std::string>, StringMapToPythonObjectConverter>();
		bp::to_python_converter<std::multimap<std::string, std::string>, StringMultimapToPythonObjectConverter>();
		bp::converter::registry::push_back(
			&PythonObjectToRouteConverter::convertible,
			&PythonObjectToRouteConverter::construct,
			boost::python::type_id<Route>());
		DBG("converters done");

		bp::object mainModule = bp::import("__main__");
		DBG("import main done");

		bp::dict mainDict = bp::extract<bp::dict>(mainModule.attr("__dict__"));
		DBG("imports done");

		mainModule.attr("root_dir") = bp::str(getShareRoot().string());
		DBG("executing file");

		bp::exec_file(bp::str((getShareRoot() / "py" / "krait" / "_internal" / "python-setup.py").string()), mainDict, mainDict);
	}
	catch (bp::error_already_set const&) {
		DBG("Python error in initPython()!");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("initPython()"));
	}

	DBG("pre_atexit");
	if (atexit(PythonModule::finishPython) != 0) {
		BOOST_THROW_EXCEPTION(syscallError() << stringInfo("atexit: setting finishPython at exit") << errcodeInfoDef());
	}

	pythonInitialized = true;
	DBG("python initialized");
}


void PythonModule::finishPython() {
	if (pythonInitialized) {
		try {
			Py_Finalize();
		}
		catch (bp::error_already_set const&) {
			DBG("Python error in finishPython()!");

			BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("finishPython()"));
		}
	}
}

void PythonModule::initModules(std::string projectDir) {
	DBG("in initModules()");
	initPython();

	try {
		resetModules(projectDir);
	}
	catch (pythonError& err) {
		DBG("Python error in initModules()!");
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(err.what()) << originCallInfo("initModules()"));
	}
}

void PythonModule::resetModules(std::string projectDir) {
	DBG("in pythonReset()");
	try {
		if (modulesInitialized) {
			PythonModule::main().clear();
			PythonModule::krait().clear();
			PythonModule::mvc().clear();
			PythonModule::websockets().clear();
			PythonModule::config().clear();
		}
		PythonModule::main().setGlobal("project_dir", projectDir);

		PythonModule::main().execfile((getShareRoot() / "py" / "krait" / "_internal" / "site-setup.py").string());

		requestType = PythonModule::krait().moduleGlobals["Request"];
	}
	catch (bp::error_already_set const&) {
		DBG("Python error in resetModules()!");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("resetModules()"));
	}

	modulesInitialized = true;
}

PythonModule::PythonModule(std::string name) {
	DBG_FMT("PythonModule(%1%)", name);
	initPython();

	try {
		this->name = name;
		moduleObject = import(bp::str(name));
		moduleGlobals = bp::extract<bp::dict>(moduleObject.attr("__dict__"));
	}
	catch (bp::error_already_set const&) {
		DBG_FMT("Python error in PythonModule(%1%)!", name);

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("PythonModule()"));
	}
}


void PythonModule::clear() {
	moduleGlobals.clear();
}


void PythonModule::run(std::string command) {
	DBG("in run()");
	try {
		bp::exec(bp::str(command), moduleGlobals, moduleGlobals);
	}
	catch (bp::error_already_set const&) {
		DBG("Python error in run()!");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("run()") << pyCodeInfo(command));
	}
}


void PythonModule::execfile(std::string filename) {
	DBG("in execfile()");
	try {
		bp::exec_file(bp::str(filename), moduleGlobals, moduleGlobals);
	}
	catch (bp::error_already_set const&) {
		DBG("Python error in execfile()!");

		BOOST_THROW_EXCEPTION(pythonError()
			<< getPyErrorInfo() << originCallInfo("execfile()")
			<< pyCodeInfo(formatString("<see %1%>", filename)));
	}
}

std::string PythonModule::eval(std::string code) {
	DBG("in pythonEval()");
	try {
		bp::object result = bp::eval(bp::str(code), moduleGlobals, moduleGlobals);
		bp::str resultStr(result);
		return bp::extract<std::string>(resultStr);
	}
	catch (bp::error_already_set const&) {
		DBG("Python error in pythonEval()!");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("eval()") << pyCodeInfo(code));
	}
}


bp::object PythonModule::evalToObject(std::string code) { //TODO: references!
	DBG("in pythonEvalToObject()");
	try {
		bp::object result = bp::eval(bp::str(code), moduleGlobals, moduleGlobals);
		return result;
	}
	catch (bp::error_already_set const&) {
		DBG("Python error in pythonEval()!");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("evalToObject()") << pyCodeInfo(code));
	}
}


bool PythonModule::test(std::string condition) {
	try {
		bp::object result = bp::eval(bp::str(condition), moduleGlobals, moduleGlobals);
		return (bool)result;
	}
	catch (bp::error_already_set const&) {
		DBG("Python error in test()!");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("test()") << pyCodeInfo(condition));
	}
}

bp::object PythonModule::callObject(bp::object obj) {
	DBG("in callObject()");
	try {
		return obj();
	}
	catch (bp::error_already_set const&) {
		DBG("Python error in callObject()!");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("callObject(obj)"));
	}
}

bp::object PythonModule::callObject(bp::object obj, bp::object arg) {
	DBG("in callObject(arg)");
	try {
		return obj(arg);
	}
	catch (bp::error_already_set const&) {
		DBG("Python error in callObject(arg)!");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("callObject(obj, arg)"));
	}
}


bool PythonModule::checkIsNone(std::string name) {
	DBG("in pythonVarIsNone(name)");
	try {
		return bp::object(moduleObject.attr(name.c_str())).is_none();
	}
	catch (bp::error_already_set const&) {
		DBG("Python error in checkIsNone(name)!");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("name()") << pyCodeInfo(name));
	}
}

void PythonModule::setGlobal(const std::string& name, const bp::object& value) {
	DBG("in setGlobal()");
	try {
		moduleObject.attr(name.c_str()) = value;
	}
	catch (bp::error_already_set const&) {
		DBG("Python error in setGlobal()!");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo(formatString("setGlobal(%1%, obj)", name)));
	}
}


void PythonModule::setGlobal(const std::string& name, const std::string& value) {
	setGlobal(name, bp::str(value));
}

void PythonModule::setGlobal(const std::string& name, const std::map<std::string, std::string>& value) {
	setGlobal(name, bp::dict(value));
}

void PythonModule::setGlobal(const std::string& name, const std::multimap<std::string, std::string>& value) {
	setGlobal(name, bp::list(value));
}

void PythonModule::setGlobalRequest(const std::string& name, const Request& value) {
	try {
		setGlobal(name, bp::object(value));
	}
	catch (bp::error_already_set const&) {
		DBG("Python error in setGlobalRequest(), converting Request to PyObject");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo(formatString("setGlobalRequest(%1%, <Request>)", name)));
	}
}

std::string PythonModule::getGlobalStr(std::string name) {
	DBG("in pythonGetGlobalStr()");
	try {
		bp::object globalObj = moduleObject.attr(name.c_str());
		bp::str globalStr(globalObj);
		return bp::extract<std::string>(globalStr);
	}
	catch (bp::error_already_set const&) {
		DBG("Python error in getGlobalStr()!");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo(formatString("getGlobalStr(%1%)", name)));
	}
}

std::map<std::string, std::string> PythonModule::getGlobalMap(std::string name) {
	DBG("in getGlobalMap()");
	try {
		bp::object globalObj = moduleObject.attr(name.c_str());
		bp::dict globalDict = bp::extract<bp::dict>(globalObj);
		bp::list dictKeys = globalDict.keys();
		int nrKeys = len(dictKeys);

		std::map<std::string, std::string> result;
		for (int i = 0; i < nrKeys; i++) {
			bp::object key = dictKeys[i];
			result[bp::extract<std::string>(bp::str(key))] = bp::extract<std::string>(bp::str(globalDict[key]));
		}
		return result;
	}
	catch (bp::error_already_set const&) {
		DBG("Python error in getGlobalMap()!");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo(formatString("getGlobalMap(%1%)", name)));
	}
}

bp::object PythonModule::getGlobalVariable(std::string name) {
	DBG("in getGlobalVariable()");
	try {
		bp::object globalObj = moduleObject.attr(name.c_str());
		return globalObj;
	}
	catch (bp::error_already_set const&) {
		DBG("Python error in getGlobalVariable()!");
		
		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo(formatString("getGlobalVariable(%1%)", name)));
	}
}

std::multimap<std::string, std::string> PythonModule::getGlobalTupleList(std::string name) {
	DBG("in getGlobalTupleList()");
	try {
		bp::object globalObj = moduleObject.attr(name.c_str());
		int listLen = bp::len(globalObj);

		std::multimap<std::string, std::string> result;
		for (int i = 0; i < listLen; i++) {
			std::string key = bp::extract<std::string>(bp::str(globalObj[i][0]));
			std::string value = bp::extract<std::string>(bp::str(globalObj[i][1]));
			result.insert(make_pair(key, value));
		}
		return result;
	}
	catch (bp::error_already_set const&) {
		DBG("Python error in getGlobalTupleList()!");
		
		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo(formatString("getGlobalTupleList(%1%)", name)));
	}
};

//Dedent std::string
std::string PythonModule::prepareStr(std::string pyCode) {
	//DBG_FMT("pythonPrepareStr(%1%)", pyCode);
	if (pyCode.length() == 0 || !isspace(pyCode[0])) {
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

	for (size_t i = 0; i < pyCode.length(); i++) {
		char chr = pyCode[i];
		if (chr == ' ' || chr == '\t') {
			indentChr = chr;
			break;
		}
		else if (chr == '\r' || chr == '\n') {
			continue;
		}
		else {
			break;
		}
	}
	if (indentChr == '\0') {
		return pyCode;
	}

	for (size_t i = 0; i < pyCode.length(); i++) {
		char chr = pyCode[i];
		if (chr == '\r' || chr == '\n') {
			if (!lineWs) {
				codeLines++;

				if (commonIndent == commonIndentSentinel) {
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
		else if (chr == indentChr && inIndent) {
			lineIndent++;
			//DBG("inIndentUp");
		}
		else {
			inIndent = false;
			if (chr != ' ' && chr != '\t') {
				lineWs = false;
			}
		}
	}

	if (!lineWs) {
		codeLines++;

		if (commonIndent == commonIndentSentinel) {
			commonIndent = lineIndent;
		}
		else if (lineIndent < commonIndent) {
			size_t indentDiff = commonIndent - lineIndent;
			outSize += (codeLines - 1) * indentDiff;
			commonIndent = lineIndent;
		}
	}

	//DBG_FMT("common indent for %1%: %2% of '%3%'", pyCode, commonIndent, indentChr);
	std::string result;
	result.resize(outSize); //outSize may be a bit larger, this is fine.

	size_t offset = 0;
	lineIndent = 0;
	for (size_t i = 0; i < pyCode.length(); i++) {
		char chr = pyCode[i];
		if (chr == '\r' || chr == '\n') {
			lineIndent = 0;
			result[i - offset] = pyCode[i];
		}
		else {
			if (lineIndent < commonIndent) {
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


PyObject* PythonModule::StringMapToPythonObjectConverter::convert(std::map<std::string, std::string> const& map) {
	DBG("in map<std::string, std::string>->PyObject()");

	bp::dict result;
	for (auto it : map) {
		result[bp::str(it.first)] = bp::str(it.second);
	}
	return bp::incref(result.ptr());
}

PyObject* PythonModule::StringMultimapToPythonObjectConverter::convert(
	std::multimap<std::string, std::string> const& map) {

	DBG("in multimap<std::string, std::string>->PyObject()");

	bp::list result;
	for (const auto& it : map) {
		result.append(bp::make_tuple(bp::str(it.first), bp::str(it.second)));
	}

	return bp::incref(result.ptr());
}

PyObject* PythonModule::requestToPythonObjectConverter::convert(Request const& request) {
	DBG("in Request->PyObject()");

	bp::object result = PythonModule::requestType(
		bp::str(httpVerbToString(request.getVerb())),
		bp::str(request.getUrl()),
		bp::str(request.getQueryString()),
		bp::str((b::format("HTTP/%1%.%2%") % request.getHttpMajor() % request.getHttpMinor()).str()),
		bp::dict(request.getHeaders()),
		bp::str(request.getBody())
	);

	return bp::incref(result.ptr());
}

void* PythonModule::PythonObjectToRouteConverter::convertible(PyObject* objPtr) {
	if (PyObject_HasAttrString(objPtr, "verb") &&
		PyObject_HasAttrString(objPtr, "url") &&
		PyObject_HasAttrString(objPtr, "regex") &&
		PyObject_HasAttrString(objPtr, "target")) {
		return objPtr;
	}
	return nullptr;
}

void PythonModule::PythonObjectToRouteConverter::construct(PyObject* objPtr, bp::converter::rvalue_from_python_stage1_data* data) {
	try {
		bp::object obj = bp::object(bp::handle<>(bp::borrowed(objPtr)));
		void* storage = ((bp::converter::rvalue_from_python_storage<Route>*)data)->storage.bytes;

		boost::optional<std::string> regexStr = extractOptional<std::string>(obj.attr("regex"));
		boost::optional<boost::regex> regex;
		if (regexStr) {
			regex = boost::regex(regexStr.get());
		}

		// ReSharper disable CppNonReclaimedResourceAcquisition
		new(storage) Route(
			toRouteVerb(static_cast<std::string>(bp::extract<std::string>(obj.attr("verb")))),
			regex,
			extractOptional<std::string>(obj.attr("url")),
			extractOptional<std::string>(obj.attr("target")),
			extractOptional<bp::object>(obj.attr("ctrl_class"))
		);
		// ReSharper restore CppNonReclaimedResourceAcquisition

		data->convertible = storage;
	}
	catch (bp::error_already_set const&) {
		DBG("Python error in PythonObjectToRouteConverter::construct!");
		
		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("PythonObjectToRouteConverter::construct()"));
	}
}
