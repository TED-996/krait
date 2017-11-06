#include "pyEmitModule.h"
#include "except.h"
#include "pythonModule.h"
#include "utils.h"
#include <boost/python.hpp>

#define DBG_DISABLE
#include "dbg.h"


PyEmitModule* PyEmitModule::instance = nullptr;
bool PyEmitModule::hidden = false;


PyEmitModule::EmitResponseIterator::EmitResponseIterator(const std::vector<boost::string_ref>& source)
        : source(source) {
    currentIdx = 0;
}

boost::string_ref PyEmitModule::EmitResponseIterator::operator*() const {
    if (currentIdx >= source.size()) {
        return boost::string_ref();
    }
    return source[currentIdx];
}

IResponseIterator& PyEmitModule::EmitResponseIterator::operator++() {
    if (currentIdx < source.size()) {
        currentIdx++;
    }
    return *this;
}

bool PyEmitModule::EmitResponseIterator::isTmpRef(const boost::string_ref& ref) const {
    return false;
}

PyEmitModule::PyEmitModule() {
    if (instance != nullptr) {
        BOOST_THROW_EXCEPTION(serverError() << stringInfo("Multiple PyEmitModule instances."));
    }
    instance = this;

    reset();
}

PyEmitModule::PyEmitModule(PyEmitModule&& other) noexcept
        : pyStrings(std::move(other.pyStrings)), stdStrings(std::move(other.stdStrings)), refs(std::move(other.refs)) {
    if (instance == &other) {
        instance = this;
    }
}

PyEmitModule::~PyEmitModule() {
    if (instance == this) {
        instance = nullptr;
    }
}

void PyEmitModule::reset() {
    pyStrings.clear();
    stdStrings.clear();
    refs.clear();
}

std::unique_ptr<IResponseIterator> PyEmitModule::getIterator() {
    return std::make_unique<EmitResponseIterator>(refs);
}


BOOST_PYTHON_MODULE(_krait_emit) {
    boost::python::def("emit", &PyEmitModule::emit);
    boost::python::def("emit_raw", &PyEmitModule::emitRaw);
}


void PyEmitModule::initializeModule() {
    PyImport_AppendInittab("_krait_emit", &init_krait_emit);
}

void PyEmitModule::emit(boost::python::object value) {
    PyEmitModule* instanceLocal = getInstanceOrThrow();

    boost::python::str strValue = PythonModule::toPythonStr(value);

    boost::optional<std::string> escapedStr = htmlEscapeRef(
        boost::string_ref(boost::python::extract<const char*>(strValue)(), boost::python::len(strValue)));

    if (escapedStr == boost::none) {
        instanceLocal->addPythonObj(std::move(strValue));
    } else {
        instanceLocal->addStdString(std::move(escapedStr.get()));
    }
}

void PyEmitModule::emitRaw(boost::python::object value) {
    PyEmitModule* instanceLocal = getInstanceOrThrow();

    boost::python::str strValue = PythonModule::toPythonStr(value);

    instanceLocal->addPythonObj(std::move(strValue));
}

PyEmitModule* PyEmitModule::getInstanceOrThrow() {
    PyEmitModule* instanceLocal = instance;
    if (instanceLocal == nullptr || hidden) {
        PyErr_SetString(PyExc_ValueError, "Krait Emit not available, not processing a (suitable) response.");
        boost::python::throw_error_already_set();
    }
    return instanceLocal;
}

void PyEmitModule::addPythonObj(boost::python::str&& string) {
    pyStrings.emplace_back(std::move(string));
    const boost::python::str& inserted = pyStrings.back();
    refs.emplace_back(boost::string_ref(boost::python::extract<const char*>(inserted), boost::python::len(inserted)));
}

void PyEmitModule::addStdString(std::string&& string) {
    stdStrings.emplace_back(std::move(string));
    refs.emplace_back(stdStrings.back());
}
