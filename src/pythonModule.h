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
	std::string eval(std::string command);
	bool test(std::string condition);
	bool checkIsNone(std::string name);

	void setGlobal(std::string name, std::string value);
	void setGlobal(std::string name, std::map<std::string, std::string> value);
	void setGlobalRequest(std::string name, Request value);

	std::string getGlobalStr(std::string name);
	std::map<std::string, std::string> getGlobalMap(std::string name);

private:
	void setGlobal(std::string name, boost::python::object value);

//Statics
public:
	static std::string prepareStr(std::string pyCode);
	static std::string errAsString();

	static void initPython();
	static void initModules(std::string projectDir);
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
};
