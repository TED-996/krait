#pragma once
#include<stack>
#include <boost/utility/string_ref.hpp>
#include "pymlFile.h"
#include "IResponseIterator.h"

class PymlIterator : public IResponseIterator
{
private:
	std::stack<const IPymlItem*> items;
	std::string tmpStr;
	const std::string* lastValuePtr;
public:
	PymlIterator(const IPymlItem* rootItem);
	PymlIterator(const PymlIterator& other);
	PymlIterator(PymlIterator&& other) noexcept;

	boost::string_ref operator*() const override;

	PymlIterator& operator++() override;

	bool isTmpRef(const boost::string_ref& ref) const override;
};
