#pragma once
#include <stack>
#include <string>
#include <boost/filesystem.hpp>
#include "pymlItems.h"
#include "IPymlCache.h"
#include "IPymlParser.h"


class V2PymlParser : public IPymlParser {
private:
	PymlItem* rootItem;

	std::stack<PymlWorkingItem> itemStack;
	PymlItemPool pool;
	boost::object_pool<PymlWorkingItem> workingItemPool;

	IPymlCache& cache;
	boost::filesystem::path embedRoot;

	int krItIndex;

	template<typename T>
	bool stackTopIsType(){
		return !itemStack.empty() && boost::get<T>(&itemStack.top().data) != nullptr;
	}

	template<typename T>
	T& getStackTop(){
		return boost::get<T>(itemStack.top().data);
	}

	template<typename T>
	T& popStackTop(){
		T& result = boost::get<T>(itemStack.top().data);
		itemStack.pop();
		return result;
	}

	void addPymlWorkingStr(const std::string& str);
	void addPymlWorkingPyCode(PymlWorkingItem::Type type, const std::string& code);
	void addPymlWorkingEmbed(const std::string& filename);
	void pushPymlWorkingIf(const std::string& condition);
	bool addSeqToPymlWorkingIf(bool isElse);
	void pushPymlWorkingFor();
	void addCodeToPymlWorkingFor(int where, const std::string& code);
	bool addSeqToPymlWorkingFor();
	void pushPymlWorkingForIn(std::string entry, std::string collection);
	void pushPymlWorkingSeq();
	void addPymlStackTop();

	
	bool consumeOne(char ch);

public:
	V2PymlParser(IPymlCache& cache, boost::filesystem::path embedRoot);

	void consume(std::string::iterator start, std::string::iterator end);
	IPymlItem* getParsed();
};
