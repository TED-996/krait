#pragma once
#include <boost/python/str.hpp>
#include <vector>
#include "IResponseIterator.h"
#include "except.h"

class PyEmitModule
{
	static PyEmitModule* instance;

	std::vector<boost::python::str> pyStrings;
	std::vector<std::string> stdStrings;
	std::vector<boost::string_ref> refs;

	class EmitResponseIterator : public IResponseIterator
	{
		const std::vector<boost::string_ref>& source;
		size_t currentIdx;
	public:
		explicit EmitResponseIterator(const std::vector<boost::string_ref>& source);
		EmitResponseIterator(const EmitResponseIterator&) = default;

		boost::string_ref operator*() const override;
		IResponseIterator& operator++() override;
		bool isTmpRef(const boost::string_ref& ref) const override;
	};

	void addPythonObj(boost::python::str&& strings);
	void addStdString(std::string&& string);

	static PyEmitModule* getInstance();
public:
	PyEmitModule();

	void hideInstance() {
		instance = nullptr;
	}
	void showInstance() {
		instance = this;
	}

	void reset();
	std::unique_ptr<IResponseIterator> getIterator();
	
	static void initialize();
	static void emit(boost::python::object value);
	static void emitRaw(boost::python::object value);
};
