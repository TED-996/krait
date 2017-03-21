#pragma once
#include <stack>
#include <string>
#include <boost/filesystem.hpp>
#include "IPymlCache.h"
#include "IPymlFile.h"
#include "pymlItems.h"


class PymlFile : public IPymlFile {
public:
	struct CacheInfo {
		IPymlCache& cache;
		boost::filesystem::path embedRoot;
	};
	struct SrcInfo {
		const std::string& source;
		bool isRaw;
	};

private:
	const PymlItem* rootItem;
	
	int state;
	std::string workingBackBuffer;
	char workingStr[65536];
	unsigned int workingIdx;
	unsigned int absIdx;
	unsigned int saveIdx;
	
	std::stack<PymlWorkingItem> itemStack;
	PymlItemPool pool;
	boost::object_pool<PymlWorkingItem> workingItemPool;
	std::string tmpStr;

	CacheInfo cacheInfo;
	
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
	
	const PymlItem* parseFromSource(const std::string& source);
	bool consumeOne(char chr);
	
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
	
public:
	PymlFile(PymlFile::SrcInfo srcInfo, PymlFile::CacheInfo cacheInfo);
	
	PymlFile(PymlFile&) = delete;
	PymlFile(PymlFile const&) = delete;
	
	bool isDynamic() const;
	std::string runPyml() const;

	const IPymlItem* getRootItem() const{
		return (IPymlItem*) rootItem;
	}
};
