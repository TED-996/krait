#include "pyEmitModule.h"
#include <boost/python/extract.hpp>
#include "except.h"


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
	if (instance == nullptr) {
		BOOST_THROW_EXCEPTION(serverError() << stringInfo("Multiple PyEmitModule instances."));
	}
	instance = this;

	reset();
}

void PyEmitModule::reset() {
	pyStrings.clear();
}

std::unique_ptr<IResponseIterator> PyEmitModule::getIterator() {
	return std::make_unique<EmitResponseIterator>(refs);
}

void PyEmitModule::initialize() {
	TODO;
}

void PyEmitModule::emit(boost::python::object value) {
	if (throwIfNoInstance()) {
		return;
	}
	
}

void PyEmitModule::emitRaw(boost::python::object value) {
	if (throwIfNoInstance()) {
		return;
	}
}

bool PyEmitModule::throwIfNoInstance() {
	if (instance == nullptr) {
		PyErr_SetString(PyExc_ValueError, "Emit not available, not processing a response.");
		return true;
	}
	return false;
}
