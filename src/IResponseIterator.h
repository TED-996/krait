#pragma once
#include <boost/utility/string_ref.hpp>

class IResponseIterator
{
public:
	virtual ~IResponseIterator() = default;
	
	virtual boost::string_ref operator*() const = 0;
	virtual IResponseIterator& operator++() = 0;
	virtual bool isTmpRef(const boost::string_ref& ref) const = 0;
};
