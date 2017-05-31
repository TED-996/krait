#pragma once
#include<boost/python.hpp>
#include<string>
#include<map>
#include<memory>
#include"request.h"

class PythonModule {
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

private:
	static bool pythonInitialized;
	static bool modulesInitialized;
	static boost::python::object requestType;

//Methods
public:
	PythonModule(std::string name);
	void clear();

	void run(std::string command);
	void execfile(std::string filename);
	std::string eval(std::string code);
	boost::python::object evalToObject(std::string code);
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

private:
	void setGlobal(std::string name, boost::python::object value);

//Statics
public:
	static std::string prepareStr(std::string pyCode);
	static std::string errAsString();

	static void initPython();
	static void initModules(std::string projectDir);
	static void finishPython();
private:
	static void resetModules(std::string projectDir);

//Structs
private:
	struct StringMapToPythonObjectConverter{
		static PyObject* convert(std::map<std::string, std::string> const& map);
	};

	struct requestToPythonObjectConverter {
		static PyObject* convert(Request const& request);
	};

	struct StringMultimapToPythonObjectConverter {
		static PyObject* convert(std::multimap<std::string, std::string> const& map);
	};

};
