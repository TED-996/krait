#pragma once
#include <string>
#include <boost/utility/string_ref.hpp>

class CompiledPythonRunner
{
public:
	void run(boost::string_ref name);
};
