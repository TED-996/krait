#pragma once
#include<string>
#include<vector>
#include<stack>
#include<boost/pool/object_pool.hpp>
#include<boost/variant.hpp>


class PymlItem {
public:
	virtual std::string runPyml() const {
		return "";
	}
	
	virtual bool isDynamic() const {
		return false;
	}
};


class PymlItemStr : public PymlItem {
	std::string str;
public:
	PymlItemStr(const std::string& str){
		this->str = str;
	}
	
	std::string runPyml() const override {
		return str;
	}
	
	bool isDynamic() const override {
		return false;
	}
};


class PymlItemSeq : public PymlItem {
	std::vector<const PymlItem*> items;
public:
	PymlItemSeq(const std::vector<const PymlItem*>& items){
		this->items = items;
	}
	
	std::string runPyml() const override;
	bool isDynamic() const override;
	
	const PymlItem* tryCollapse() const;
};


class PymlItemPyEval : public PymlItem {
	std::string code;
public:
	PymlItemPyEval(const std::string& code){
		this->code = code;
	}
	std::string runPyml() const override;
	
	bool isDynamic() const override {
		return true;
	}
};


class PymlItemPyEvalRaw : public PymlItem {
	std::string code;
public:
	PymlItemPyEvalRaw(const std::string& code){
		this->code = code;
	}
	std::string runPyml() const override;
	
	bool isDynamic() const override {
		return true;
	}
};


class PymlItemPyExec : public PymlItem {
	std::string code;
public:
	PymlItemPyExec(const std::string& code){
		this->code = code;
	}
	std::string runPyml() const override;
	
	bool isDynamic() const override {
		return true;
	}
};


class PymlItemIf : public PymlItem {
	std::string conditionCode;
	const PymlItem* itemIfTrue;
	const PymlItem* itemIfFalse;
	
public:
	PymlItemIf(const std::string& conditionCode, const PymlItem* itemIfTrue, const PymlItem* itemIfFalse){
		this->conditionCode = conditionCode;
		this->itemIfTrue = itemIfTrue;
		this->itemIfFalse = itemIfFalse;
	}
	
	std::string runPyml() const override;
	
	bool isDynamic() const override {
		return true;
	}
};


class PymlItemFor : public PymlItem {
	std::string initCode;
	std::string conditionCode;
	std::string updateCode;
	
	const PymlItem* loopItem;
	
public:
	PymlItemFor(const std::string& initCode, const std::string& conditionCode, const std::string& updateCode, const PymlItem* loopItem){
		this->initCode = initCode;
		this->conditionCode = conditionCode;
		this->updateCode = updateCode;
		this->loopItem = loopItem;
	}
	
	std::string runPyml() const override;
	
	bool isDynamic() const override {
		return true;
	}
};


struct PymlItemPool {
	boost::object_pool<PymlItem> itemPool;
	boost::object_pool<PymlItemStr> strPool;
	boost::object_pool<PymlItemSeq> seqPool;
	boost::object_pool<PymlItemPyEval> pyEvalPool;
	boost::object_pool<PymlItemPyEvalRaw> pyEvalRawPool;
	boost::object_pool<PymlItemPyExec> pyExecPool;
	boost::object_pool<PymlItemIf> ifExecPool;
	boost::object_pool<PymlItemFor> forExecPool;
};


struct PymlWorkingItem {
	enum Type { None, Str, Seq, PyEval, PyEvalRaw, PyExec, If, For };
	struct NoneData {
	};
	struct StrData {
		std::string str;
	};
	struct SeqData {
		std::vector<PymlWorkingItem*> items;
	};
	struct PyCodeData {
		Type type;
		std::string code;
	};
	struct IfData {
		std::string condition;
		PymlWorkingItem* itemIfTrue;
		PymlWorkingItem* itemIfFalse;
	};
	struct ForData {
		std::string initCode;
		std::string conditionCode;
		std::string updateCode;
		PymlWorkingItem* loopItem;
	};
	
	boost::variant<NoneData, StrData, SeqData, PyCodeData, IfData, ForData> data;
	
	Type type; //TODO: set
	PymlWorkingItem(Type type);
	
	template<typename T>
	T* getData(){
		return boost::get<T>(&data);
	}
	
	const PymlItem* getItem(PymlItemPool& pool) const;
	
};


class PymlFile {
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
	void pushPymlWorkingIf(const std::string& condition);
	bool addSeqToPymlWorkingIf(bool isElse);
	void pushPymlWorkingFor();
	void addCodeToPymlWorkingFor(int where, const std::string& code);
	bool addSeqToPymlWorkingFor();
	void pushPymlWorkingForIn(std::string entry, std::string collection);
	void pushPymlWorkingSeq();
	void addPymlStackTop();
	
public:
	PymlFile(const std::string& pymlSource){
		rootItem = parseFromSource(pymlSource);
	}
	
	PymlFile(PymlFile&) = delete;
	PymlFile(PymlFile const&) = delete;
	
	bool isDynamic() const;
	std::string runPyml() const;
};
