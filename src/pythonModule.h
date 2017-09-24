#pragma once

#include <boost/python/detail/wrap_python.hpp>
#include <boost/python.hpp>
#include <boost/optional.hpp>
#include <string>
#include <map>
#include "request.h"
#include "response.h"
#include "except.h"

class PythonModule
{
private:
	std::string name;
	boost::python::object moduleObject;
	boost::python::dict moduleGlobals;

	//Statics
public:
	static PythonModule& main() {
		static PythonModule mod("__main__");
		return mod;
	}
	static PythonModule& krait() {
		static PythonModule mod("krait");
		return mod;
	}
	static PythonModule& mvc() {
		static PythonModule mod("krait");
		return mod;
	}
	static PythonModule& websockets() {
		static PythonModule mod("krait.websockets");
		return mod;
	}
	static PythonModule& config() {
		static PythonModule mod("krait.config");
		return mod;
	}
	static PythonModule& cookie() {
		static PythonModule mod("krait.cookie");
		return mod;
	}

private:
	static bool pythonInitialized;
	static bool modulesInitialized;
	static boost::python::object requestType;

	//Methods
public:
	explicit PythonModule(const std::string& name);
	void clear();

	void run(const std::string& command);
	void execfile(const std::string& filename);
	std::string eval(const std::string& code);
	boost::python::object evalToObject(const std::string& code);

	template<typename T>
	boost::optional<T> evalToOptional(const std::string& code) {
		return extractOptional<T>(this->evalToObject(code));
	}

	bool test(const std::string& condition);
	bool checkIsNone(const std::string& name);

	boost::python::object callObject(const boost::python::object& obj, const boost::python::object& arg);
	boost::python::object callObject(const boost::python::object& obj);

	void setGlobal(const std::string& name, const std::string& value);
	void setGlobal(const std::string& name, const std::map<std::string, std::string>& value);
	void setGlobal(const std::string& name, const std::multimap<std::string, std::string>& value);
	void setGlobal(const std::string& name, const boost::python::object& value);
	void setGlobalRequest(const std::string&, const Request& value);
	void setGlobalNone(const std::string& name);
	void setGlobalEmptyList(const std::string& name);

	std::string getGlobalStr(const std::string& name);
	std::map<std::string, std::string> getGlobalMap(const std::string& name);
	std::multimap<std::string, std::string> getGlobalTupleList(const std::string& name);
	Response* getGlobalResponsePtr(const std::string& name);

	boost::python::object getGlobalVariable(const std::string& name);

	template<typename T>
	boost::optional<T> getGlobalOptional(const std::string& name) {
		return extractOptional<T>(this->getGlobalVariable(name));
	}

	//Statics
public:
	static std::string prepareStr(const std::string& pyCode);

	template<typename T>
	static boost::optional<T> extractOptional(boost::python::object value){
		if (value.is_none()) {
			return boost::none;
		}
		try {
			return boost::python::extract<T>(value)();
		}
		catch(boost::python::error_already_set const&) {
			BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("extractOptional()"));
		}
	}

	static boost::optional<boost::python::object> extractOptional(boost::python::object value) {
		if (value.is_none()) {
			return boost::none;
		}
		try {
			return boost::optional<boost::python::object>(value);
		}
		catch (boost::python::error_already_set const&) {
			BOOST_THROW_EXCEPTION(pythonError() << getPyErrorInfo() << originCallInfo("extractOptional()"));
		}
	}

	static void initPython();
	static void initModules(const std::string& projectDir);
	static void finishPython();
private:
	static void resetModules(const std::string& projectDir);

	//Structs
private:
	template<typename T>
	struct PtrWrapper
	{
		T* value;
		PtrWrapper() noexcept : value(nullptr){}
		PtrWrapper(T* value) noexcept : value(value){}

		PtrWrapper(const PtrWrapper& other) noexcept = default;
		PtrWrapper(PtrWrapper&& other) noexcept = default;
		PtrWrapper& operator=(const PtrWrapper& other) noexcept = default;
		PtrWrapper& operator=(PtrWrapper&& other) noexcept = default;

		operator T*() const noexcept { return value; }
		T* operator()() const noexcept { return value; }
	};

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

	struct PythonObjectToResponsePtrConverter
	{
		static void* convertible(PyObject* objPtr);
		static void construct(PyObject* objPtr, boost::python::converter::rvalue_from_python_stage1_data* data);
	};
};
