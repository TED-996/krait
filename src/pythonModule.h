#pragma once

#include "except.h"
#include "request.h"
#include "response.h"
#include <boost/optional.hpp>
#include <boost/python.hpp>
#include <boost/python/detail/wrap_python.hpp>
#include <map>
#include <string>

class PythonModule {
private:
    std::string name;
    boost::python::object moduleObject;
    boost::python::dict moduleGlobals;

    // Statics
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

    // TODO: maybe add uncached variants
    static void reload(const std::string& moduleName);
    static void reload(PythonModule& module);

    static PythonModule* getCached(const std::string& moduleName);

    static boost::python::str toPythonStr(const boost::python::object& obj) {
        if (PyString_Check(obj.ptr())) {
            return boost::python::extract<boost::python::str>(obj);
        } else {
            return boost::python::str(obj);
        }
    }
    static std::string toStdString(const boost::python::object& obj) {
        return boost::python::extract<std::string>(toPythonStr(obj));
    }

private:
    static bool pythonInitialized;
    static bool modulesInitialized;
    static boost::python::object requestType;

    // Every module that is kept MUST live in the module cache.
    static std::unordered_multimap<std::string, PythonModule&> moduleCache;

    // Methods
public:
    explicit PythonModule(boost::string_ref name);
    PythonModule(PythonModule& other) = default;

    // Late opening
    PythonModule();
    void open(boost::string_ref name);

    ~PythonModule();

    void clear();
    void addToCache();
    void removeFromCache();

    const std::string& getName() const {
        return name;
    }

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

    boost::python::object callGlobal(const std::string& name);
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

    // Statics
public:
    static std::string prepareStr(std::string&& pyCode);

    template<typename T>
    static boost::optional<T> extractOptional(boost::python::object value) {
        if (value.is_none()) {
            return boost::none;
        }
        try {
            return boost::python::extract<T>(value)();
        } catch (boost::python::error_already_set const&) {
            BOOST_THROW_EXCEPTION(getPythonError() << originCallInfo("extractOptional()"));
        }
    }

    static boost::optional<boost::python::object> extractOptional(boost::python::object value) {
        if (value.is_none()) {
            return boost::none;
        }
        try {
            return boost::optional<boost::python::object>(value);
        } catch (boost::python::error_already_set const&) {
            BOOST_THROW_EXCEPTION(getPythonError() << originCallInfo("extractOptional()"));
        }
    }

    static void initPython();
    static void initModules(const std::string& projectDir);
    static void finishPython();

protected:
    explicit PythonModule(boost::python::object moduleObject);

private:
    static void resetModules(const std::string& projectDir);

    // Structs
private:
    // Converter heplers

    template<typename T>
    struct PtrWrapper {
        T* value;
        PtrWrapper() noexcept : value(nullptr) {
        }
        PtrWrapper(T* value) noexcept : value(value) {
        }

        PtrWrapper(const PtrWrapper& other) noexcept = default;
        PtrWrapper(PtrWrapper&& other) noexcept = default;
        PtrWrapper& operator=(const PtrWrapper& other) noexcept = default;
        PtrWrapper& operator=(PtrWrapper&& other) noexcept = default;

        operator T*() const noexcept {
            return value;
        }
        T* operator()() const noexcept {
            return value;
        }
    };


    // Converters
    struct StringMapToPythonObjectConverter {
        static PyObject* convert(std::map<std::string, std::string> const& map);
    };

    struct requestToPythonObjectConverter {
        static PyObject* convert(Request const& request);
    };

    struct StringMultimapToPythonObjectConverter {
        static PyObject* convert(std::multimap<std::string, std::string> const& map);
    };

    struct StringRefToPythonObjectConverter {
        static PyObject* convert(boost::string_ref const& ref);
    };


    struct PythonObjectToRouteConverter {
        static void* convertible(PyObject* objPtr);
        static void construct(PyObject* objPtr, boost::python::converter::rvalue_from_python_stage1_data* data);
    };

    struct PythonObjectToResponsePtrConverter {
        static void* convertible(PyObject* objPtr);
        static void construct(PyObject* objPtr, boost::python::converter::rvalue_from_python_stage1_data* data);
    };

    struct PythonObjectToStringRefConverter {
        static void* convertible(PyObject* objPtr);
        static void construct(PyObject* objPtr, boost::python::converter::rvalue_from_python_stage1_data* data);
    };
};
