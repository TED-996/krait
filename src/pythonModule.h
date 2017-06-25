#pragma once

#include<Python.h>
#include<boost/python.hpp>
#include<boost/optional.hpp>
#include<string>
#include<map>
#include"request.h"
#include"except.h"

class PythonModule
{
private:
	std::string name;
	boost::python::object moduleObject;
	boost::python::dict moduleGlobals;

	//Statics
public:
	static PythonModule main;
	static PythonModule krait;
	static PythonModule mvc;
	static PythonModule websockets;
	static PythonModule config;

private:
	static bool pythonInitialized;
	static bool modulesInitialized;
	static boost::python::object requestType;

	//Methods
public:
	explicit PythonModule(std::string name);
	void clear();

	void run(std::string command);
	void execfile(std::string filename);
	std::string eval(std::string code);
	boost::python::object evalToObject(std::string code);

	template<typename T>
	boost::optional<T> evalToOptional(std::string code) {
		return extractOptional<T>(this->evalToObject(code));
	}

	bool test(std::string condition);
	bool checkIsNone(std::string name);

	boost::python::object callObject(boost::python::object obj, boost::python::object arg);
	boost::python::object callObject(boost::python::object obj);

	void setGlobal(std::string name, std::string value);
	void setGlobal(std::string name, std::map<std::string, std::string> value);
	void setGlobal(std::string name, std::multimap<std::string, std::string> value);
	void setGlobalRequest(std::string name, Request value);

	std::string getGlobalStr(std::string name);
	std::map<std::string, std::string> getGlobalMap(std::string name);
	std::multimap<std::string, std::string> getGlobalTupleList(std::string name);
	boost::python::object getGlobalVariable(std::string name);

	template<typename T>
	boost::optional<T> getGlobalOptional(std::string name) {
		return extractOptional<T>(this->getGlobalVariable(name));
	}

private:
	void setGlobal(std::string name, boost::python::object value);

	//Statics
public:
	static std::string prepareStr(std::string pyCode);

	template<typename T>
	static boost::optional<T> extractOptional(boost::python::object value){
		if (value.is_none()) {
			return boost::none;
		}
		try {
			return boost::optional<T>(boost::python::extract<T>(value));
		}
		catch(boost::python::error_already_set const&) {
			BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("extractOptional()"));
		}
	}

	static void initPython();
	static void initModules(std::string projectDir);
	static void finishPython();
private:
	static void resetModules(std::string projectDir);

	//Structs
private:
	struct StringMapToPythonObjectConverter
	{
		static PyObject* convert(std::map<std::string, std::string> const& map);
	};

	struct requestToPythonObjectConverter
	{
		static PyObject* convert(Request const& request);
	};

	struct StringMultimapToPythonObjectConverter
	{
		static PyObject* convert(std::multimap<std::string, std::string> const& map);
	};

	struct PythonObjectToRouteConverter
	{
		static void* convertible(PyObject* objPtr);
		static void construct(PyObject* objPtr, boost::python::converter::rvalue_from_python_stage1_data* data);
	};
};
