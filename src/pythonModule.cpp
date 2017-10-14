#include "pythonModule.h"
#include <boost/python.hpp>
#include <boost/format.hpp>
#include <string>
#include <cstdlib>
#include "path.h"
#include "except.h"
#include "routes.h"
#include "response.h"
#include "pyEmitModule.h"
#include "pythonCodeEscapingFsm.h"

#define DBG_DISABLE
#include "dbg.h"

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
		//First initialize the Emit module.
		PyEmitModule::initialize();

		//Afterwards initialize Python.
		Py_Initialize();
		DBG("PyInitialize done");
		bp::to_python_converter<Request, requestToPythonObjectConverter>();
		bp::to_python_converter<std::map<std::string, std::string>, StringMapToPythonObjectConverter>();
		bp::to_python_converter<std::multimap<std::string, std::string>, StringMultimapToPythonObjectConverter>();
		bp::converter::registry::push_back(
			&PythonObjectToRouteConverter::convertible,
			&PythonObjectToRouteConverter::construct,
			boost::python::type_id<Route>());
		bp::converter::registry::push_back(
			&PythonObjectToResponsePtrConverter::convertible,
			&PythonObjectToResponsePtrConverter::construct,
			boost::python::type_id<PtrWrapper<Response>>());

		DBG("converters done");

		bp::object mainModule = bp::import("__main__");
		DBG("import main done");

		bp::dict mainDict = bp::extract<bp::dict>(mainModule.attr("__dict__"));
		DBG("imports done");

		mainModule.attr("root_dir") = bp::str(getShareRoot().string());
		DBG("executing file");

		bp::exec_file(bp::str((getShareRoot() / "py" / "krait" / "_internal" / "python-setup.py").string()), mainDict, mainDict);
	}
	catch (const bp::error_already_set&) {
		DBG("Python error in initPython().");

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
		catch (const bp::error_already_set&) {
			DBG("Python error in finishPython().");

			BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("finishPython()"));
		}
		pythonInitialized = false;
	}
}

void PythonModule::initModules(const std::string& projectDir) {
	DBG("in initModules()");
	initPython();

	try {
		resetModules(projectDir);
	}
	catch (pythonError& err) {
		DBG("Python error in initModules().");
		BOOST_THROW_EXCEPTION(pythonError() << stringInfo(err.what()) << originCallInfo("initModules()"));
	}
}

void PythonModule::resetModules(const std::string& projectDir) {
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
	catch (const bp::error_already_set&) {
		DBG("Python error in resetModules().");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("resetModules()"));
	}

	modulesInitialized = true;
}

PythonModule::PythonModule(const std::string& name) {
	DBG_FMT("PythonModule(%1%)", name);
	initPython();

	try {
		this->name = name;
		moduleObject = bp::import(bp::str(name));
		moduleGlobals = bp::extract<bp::dict>(moduleObject.attr("__dict__"));
	}
	catch (const bp::error_already_set&) {
		DBG_FMT("Python error in PythonModule(%1%).", name);

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("PythonModule()"));
	}
}

PythonModule::PythonModule(boost::python::object moduleObject) {
	DBG("PythonModule(moduleObject)");
	// No initPython() necessary, everything is already loaded, since we have an object.

	try {
		this->moduleObject = moduleObject;
		moduleGlobals = bp::extract<bp::dict>(moduleObject.attr("__dict__"));
		name = bp::extract<std::string>(moduleObject.attr("__name__"));
	}
	catch (const bp::error_already_set&) {
		DBG("Python error in PythonModule(moduleObject).");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("PythonModule(moduleObject)"));
	}
}

void PythonModule::clear() {
	moduleGlobals.clear();
}


void PythonModule::run(const std::string& command) {
	DBG("in run()");
	try {
		bp::exec(bp::str(command), moduleGlobals, moduleGlobals);
	}
	catch (const bp::error_already_set&) {
		DBG("Python error in run().");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("run()") << pyCodeInfo(command));
	}
}


void PythonModule::execfile(const std::string& filename) {
	DBG("in execfile()");
	try {
		bp::exec_file(bp::str(filename), moduleGlobals, moduleGlobals);
	}
	catch (const bp::error_already_set&) {
		DBG("Python error in execfile().");

		BOOST_THROW_EXCEPTION(pythonError()
			<< getPyErrorInfo() << originCallInfo("execfile()")
			<< pyCodeInfo(formatString("<see %1%>", filename)));
	}
}

std::string PythonModule::eval(const std::string& code) {
	DBG("in pythonEval()");
	try {
		return toStdString(bp::eval(bp::str(code), moduleGlobals, moduleGlobals));
	}
	catch (const bp::error_already_set&) {
		DBG("Python error in pythonEval().");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("eval()") << pyCodeInfo(code));
	}
}


bp::object PythonModule::evalToObject(const std::string& code) {
	DBG("in pythonEvalToObject()");
	try {
		bp::object result = bp::eval(bp::str(code), moduleGlobals, moduleGlobals);
		return result;
	}
	catch (const bp::error_already_set&) {
		DBG("Python error in pythonEval().");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("evalToObject()") << pyCodeInfo(code));
	}
}


bool PythonModule::test(const std::string& condition) {
	try {
		bp::object result = bp::eval(bp::str(condition), moduleGlobals, moduleGlobals);
		return (bool)result;
	}
	catch (const bp::error_already_set&) {
		DBG("Python error in test().");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("test(condition)") << pyCodeInfo(condition));
	}
}

bp::object PythonModule::callObject(const bp::object& obj) {
	DBG("in callObject()");
	try {
		return obj();
	}
	catch (const bp::error_already_set&) {
		DBG("Python error in callObject().");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("callObject(obj)"));
	}
}

bp::object PythonModule::callObject(const bp::object& obj, const bp::object& arg) {
	DBG("in callObject(arg)");
	try {
		return obj(arg);
	}
	catch (const bp::error_already_set&) {
		DBG("Python error in callObject(arg).");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("callObject(obj, arg)"));
	}
}

boost::python::object PythonModule::callGlobal(const std::string& name) {
	DBG("in callGlobal()");
	try {
		return moduleObject.attr(name.c_str())();
	}
	catch (const bp::error_already_set&) {
		DBG("Python error in callGlobal(name).");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("callGlobal(name)") << pyCodeInfo(name));
	}
}

bool PythonModule::checkIsNone(const std::string& name) {
	DBG("in pythonVarIsNone(name)");
	try {
		return bp::object(moduleObject.attr(name.c_str())).is_none();
	}
	catch (const bp::error_already_set&) {
		DBG("Python error in checkIsNone(name).");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("checkIsNone(name)") << pyCodeInfo(name));
	}
}

void PythonModule::setGlobal(const std::string& name, const bp::object& value) {
	DBG("in setGlobal()");
	try {
		moduleObject.attr(name.c_str()) = value;
	}
	catch (const bp::error_already_set&) {
		DBG("Python error in setGlobal().");

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
	catch (const bp::error_already_set&) {
		DBG("Python error in setGlobalRequest(), converting Request to PyObject");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo(formatString("setGlobalRequest(%1%, <Request>)", name)));
	}
}

void PythonModule::setGlobalNone(const std::string& name) {
	setGlobal(name, bp::object());
}

void PythonModule::setGlobalEmptyList(const std::string& name) {
	setGlobal(name, bp::list());
}

std::string PythonModule::getGlobalStr(const std::string& name) {
	DBG("in pythonGetGlobalStr()");
	try {
		return toStdString(moduleObject.attr(name.c_str()));
	}
	catch (const bp::error_already_set&) {
		DBG("Python error in getGlobalStr().");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo(formatString("getGlobalStr(%1%)", name)));
	}
}

std::map<std::string, std::string> PythonModule::getGlobalMap(const std::string& name) {
	DBG("in getGlobalMap()");
	try {
		bp::object globalObj = moduleObject.attr(name.c_str());
		bp::dict globalDict = bp::extract<bp::dict>(globalObj);
		bp::list dictKeys = globalDict.keys();
		int nrKeys = len(dictKeys);

		std::map<std::string, std::string> result;
		for (int i = 0; i < nrKeys; i++) {
			bp::object key = dictKeys[i];
			result[toStdString(key)] = toStdString(globalDict[key]);
		}
		return result;
	}
	catch (const bp::error_already_set&) {
		DBG("Python error in getGlobalMap().");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo(formatString("getGlobalMap(%1%)", name)));
	}
}

bp::object PythonModule::getGlobalVariable(const std::string& name) {
	DBG("in getGlobalVariable()");
	try {
		bp::object globalObj = moduleObject.attr(name.c_str());
		return globalObj;
	}
	catch (const bp::error_already_set&) {
		DBG("Python error in getGlobalVariable().");
		
		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo(formatString("getGlobalVariable(%1%)", name)));
	}
}

std::multimap<std::string, std::string> PythonModule::getGlobalTupleList(const std::string& name) {
	DBG("in getGlobalTupleList()");
	try {
		bp::object globalObj = moduleObject.attr(name.c_str());
		int listLen = bp::len(globalObj);

		std::multimap<std::string, std::string> result;
		for (int i = 0; i < listLen; i++) {
			std::string key = toStdString(globalObj[i][0]);
			std::string value = toStdString(globalObj[i][1]);
			result.insert(make_pair(std::move(key), std::move(value)));
		}
		return result;
	}
	catch (const bp::error_already_set&) {
		DBG("Python error in getGlobalTupleList().");
		
		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo(formatString("getGlobalTupleList(%1%)", name)));
	}
}

Response* PythonModule::getGlobalResponsePtr(const std::string& name) {
	DBG("in getGlobalTupleList()");
	try {
		return bp::extract<PtrWrapper<Response>>(moduleObject.attr(name.c_str()))()();
	}
	catch (const bp::error_already_set&) {
		DBG("Python error in getGlobalResponsePtr().");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo(formatString("getGlobalResponsePtr(%1%)", name)));
	}
}

// TODO: provide some useful error messages (since this can absolutely fail)
std::string dedentSimple(std::string&& str, char dedentCharacter) {
	static PythonCodeEscapingFsm escapingFsm;

	if (str.length() == 0 || !std::isspace(str[0])) {
		return std::move(str);
	}

	size_t indentSize = str.size();

	// First, calculate indent size.
	size_t indentStart = 0;
	for (size_t i = 0; i < str.size(); i++) {
		const char ch = str[i];

		if (ch == '\r' || ch == '\n') {
			// Reset indent
			indentStart = i + 1;
		}
		else if (ch == dedentCharacter) {
			// Continue indent
		}
		else {
			// Indent is over
			indentSize = i - indentStart;
			break;
		}
	}
	
	// Trivial cases
	if (indentSize == 0) {
		return std::move(str);
	}
	if (indentSize == str.size()) {
		str.clear();
		return std::move(str);
	}

	size_t copyOffset = 0;

	escapingFsm.reset();

	//Then, actually dedent.
	size_t indentLeft = indentSize;
	for (size_t i = 0; i < str.size(); i++) {
		const unsigned char ch = static_cast<const unsigned char>(str[i]);

		if (escapingFsm.isEscaped()) {
			// Just keep. Remove any expected indentation.
			indentLeft = 0;
		}
		escapingFsm.consumeOne(ch);

		if (ch == '\r' || ch == '\n') {
			// Keep the newlines, reset the indent.
			str[i - copyOffset] = str[i];
			indentLeft = indentSize;
		}
		else if (indentLeft > 0) {
			if (ch == dedentCharacter) {
				// All good. Skip it.
				copyOffset++;
			}
			else {
				// Bad, something else inside indent!
				// TODO: Provide line info for error.

				// Handle non-printable characters.
				if (ch < 0x20 || ch >= 0x80) {
					BOOST_THROW_EXCEPTION(siteError() << stringInfoFromFormat(
						"Unexpected control character with code %d inside indent of '%c' (code %d)",
						(unsigned int)ch, dedentCharacter, (unsigned int)dedentCharacter));
				}
				// Can print regular characters.
				else {
					BOOST_THROW_EXCEPTION(siteError() << stringInfoFromFormat(
						"Unexpected character '%c' (code %d) inside indent of '%c' (code %d)",
						ch, (unsigned int)ch, dedentCharacter, (unsigned int)dedentCharacter));
				}
			}
			indentLeft--;
		}
		else {
			// Outside indent. Just keep everything
			str[i - copyOffset] = str[i];
		}
	}

	str.erase(str.size() - copyOffset);
	return std::move(str);	
}

// Dedent std::string
std::string PythonModule::prepareStr(std::string&& pyCode) {
	return dedentSimple(dedentSimple(std::move(pyCode), '\t'), ' ');
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

		// ReSharper disable once CppNonReclaimedResourceAcquisition
		new(storage) Route(
			toRouteVerb(toStdString(obj.attr("verb"))),
			regex,
			extractOptional<std::string>(obj.attr("url")),
			extractOptional<std::string>(obj.attr("target")),
			extractOptional<bp::object>(obj.attr("ctrl_class"))
		);
		data->convertible = storage;
	}
	catch (const bp::error_already_set&) {
		DBG("Python error in PythonObjectToRouteConverter::construct.");
		
		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("PythonObjectToRouteConverter::construct()"));
	}
}

void* PythonModule::PythonObjectToResponsePtrConverter::convertible(PyObject* objPtr) {
	DBG("in PythonObjectToResponsePtrConverter:");

	if (PyObject_HasAttrString(objPtr, "http_version") &&
		PyObject_HasAttrString(objPtr, "status_code") &&
		PyObject_HasAttrString(objPtr, "headers") &&
		PyObject_HasAttrString(objPtr, "body")) {
		DBG("OK");
		return objPtr;
	}
	DBG("NOT OK");
	return nullptr;
}

void PythonModule::PythonObjectToResponsePtrConverter::construct(PyObject* objPtr, bp::converter::rvalue_from_python_stage1_data* data) {
	try {
		bp::object obj = bp::object(bp::handle<>(bp::borrowed(objPtr)));
		void* storage = ((bp::converter::rvalue_from_python_storage<Route>*)data)->storage.bytes;

		int statusCode;
		std::unordered_multimap<std::string, std::string> headers;
		std::string body;
		try {
			std::string httpVersion = toStdString(obj.attr("http_version"));
			if (httpVersion != "HTTP/1.1") {
				BOOST_THROW_EXCEPTION(siteError() << stringInfo("HTTP version not HTTP/1.1."));
			}

			statusCode = bp::extract<int>(obj.attr("status_code"));

			bp::object headersObj = obj.attr("headers");
			int listLen = bp::len(headersObj);

			for (int i = 0; i < listLen; i++) {
				std::string key = toStdString(headersObj[i][0]);
				std::string value = toStdString(headersObj[i][1]);
				headers.insert(make_pair(std::move(key), std::move(value)));
			}

			body = toStdString(obj.attr("body"));
		}
		catch(const bp::error_already_set&) {
			BOOST_THROW_EXCEPTION(pythonError() << stringInfo("krait.response malformed (most likely incorrect types)")
				<< getPyErrorInfo() << originCallInfo("PythonObjectToResponsePtrConverter::construct()"));
		}

		// ReSharper disable once CppNonReclaimedResourceAcquisition
		Response* result = new Response(
			1, 1, statusCode, headers, std::move(body), false
		);
		// ReSharper disable once CppNonReclaimedResourceAcquisition
		new(storage) PtrWrapper<Response>(result);

		data->convertible = storage;
	}
	catch (const bp::error_already_set&) {
		DBG("Python error in PythonObjectToResponsePtrConverter::construct.");

		BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("PythonObjectToResponsePtrConverter::construct()"));
	}
}
